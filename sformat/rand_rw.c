/* @(#)rand_rw.c	1.23 06/09/13 Copyright 1993-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)rand_rw.c	1.23 06/09/13 Copyright 1993-2004 J. Schilling";
#endif
/*
 *	Copyright (c) 1993-2004 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/mconfig.h>
#ifdef	HAVE_SYS_PARAM_H
#include <sys/param.h>	/* Include various defs needed with some OS */
#endif
#include <stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/time.h>
#include <signal.h>
#include <schily/schily.h>
#include <scg/scgcmd.h>
#include <scg/scsireg.h>
#include <scg/scsidefs.h>
#include <scg/scsitransp.h>

#include "scsicmds.h"

#include "fmt.h"
#include "rand_rw.h"

#define	min(a, b)	((a) < (b) ? (a) : (b))
#define	MAXBAD		(127)

extern	long	RW;
extern	int	debug;

struct	bb_list	*bad_blocks;

int	rwbad		= 0;
int	rand_rw_intr	= 0;

EXPORT	int	random_rw_test	__PR((SCSI *scgp, long first, long last));
LOCAL	void	sighandler	__PR((int));
LOCAL	int	rand_rw		__PR((SCSI *scgp, long first, long last));
LOCAL	void	pr_percent	__PR((long));
LOCAL	int	check_fault	__PR((SCSI *scgp,
					int (*) (SCSI *scgp, caddr_t bp, long addr, int cnt),
							char *, long, long));
LOCAL	void	add_bad		__PR((long, u_char));
LOCAL	void	display_bad	__PR((void));
LOCAL	void	display_code	__PR((u_char));

#define	rw_verify	(int (*) __PR((SCSI *, caddr_t bp, long addr, int cnt)))verify

EXPORT int
random_rw_test(scgp, first, last)
	SCSI	*scgp;
	long	first;
	long	last;
{
	int	ret = 0;

	if ((bad_blocks = (struct bb_list *) malloc((unsigned)((MAXBAD+1) *
					sizeof (struct bb_list)))) == 0) {
		comerr("No memory for bad block list.\n");
		/* NOTREACHED */
	}
	fillbytes((char *)bad_blocks,
				((MAXBAD+1) * sizeof (struct bb_list)), '\0');

	getstarttime();
	ret = rand_rw(scgp, first, last);
	if (!is_ccs(scgp->dev)) {
		bad_to_def(scgp);
	}
	prstats();
	display_bad();
	return (ret);
}

LOCAL void
sighandler(sig)
	int	sig;
{
	rand_rw_intr++;
}

LOCAL int
rand_rw(scgp, first, last)
	SCSI	*scgp;
	long	first;
	long	last;
{
	void	(*osig) __PR((int));
	long	i;
	long	start = 0;
	long	end = scgp->cap->c_baddr;
	long	amount;
	long	bad_block = -1;
	long	pos[2];
	int	ret = 0;
	char	*buf[2];

	for (i = 0; i < 2; i++) {
		buf[i]	= malloc((unsigned)scgp->cap->c_bsize);
		pos[i] = 0L;
	}

	osig = signal(SIGINT, sighandler);

	start = first;
	end = scgp->cap->c_baddr;
	if (start < 0 || start > end)
		start = 0L;
	if (last > 0 && last < end)
		end = last;
	amount = end - start;
	/*
	 * Do not write to Disk Label or Sinfo Block.
	 */
	if (start == 0)
		start++;
	if (start + amount <= scgp->cap->c_baddr)
		amount++;
	if (debug)
		printf("start: %ld end: %ld amount: %ld last baddr: %ld\n",
				start, end, amount, (long)scgp->cap->c_baddr);

	if (RW == -1)
		RW = amount / 50;
	printf("RAND R/W - %ld loops\n", RW);
	i = RW;

	pos[i%2] = rand() % amount + start;
	while (read_scsi(scgp, buf[i%2], pos[i%2], 1) < 0) {
		ret = check_fault(scgp, read_scsi, buf[i%2],
						pos[i%2], pos[(i+1)%2]);
		add_bad(pos[i%2], ret | READ_FAULT);
		pos[i%2] = rand() % amount + start;
	}
	while (--i >= 0) {
		pos[i%2] = rand() % amount + start;
		while (read_scsi(scgp, buf[i%2], pos[i%2], 1) < 0) {
			ret = check_fault(scgp, read_scsi, buf[i%2],
						pos[i%2], pos[(i+1)%2]);
			add_bad(pos[i%2], ret | READ_FAULT);
			pos[i%2] = rand() % amount + start;
		}

		if (write_scsi(scgp, buf[(i+1)%2], pos[(i+1)%2], 1) < 0) {
			int	j;
			ret = check_fault(scgp, write_scsi, buf[(i+1)%2],
						pos[(i+1)%2], pos[i%2]);
			scgp->silent++;
			for (j = 100; --j >= 0; ) {
				if (write_scsi(scgp, buf[(i+1)%2],
							pos[(i+1)%2], 1) >= 0)
					break;
			}
			scgp->silent--;
			add_bad(pos[(i+1)%2],
				ret | WRITE_FAULT | (j < 0?DATA_LOST:0));
		} else {
			if (verify(scgp, pos[(i+1)%2], 1, &bad_block) < 0) {
				ret = check_fault(scgp, rw_verify, (char *)0,
						pos[(i+1)%2], pos[i%2]);
				add_bad(pos[(i+1)%2], ret | VERIFY_FAULT);
			}
		}
		pr_percent(i);
		if (rand_rw_intr > 0) {
			printf("\n%lu loops done ( %ld%% ).\n",
					RW - i, (((RW - i) * 100) / RW));
			break;
		}
	}
	signal(SIGINT, osig);
	printf("\n");
	return (ret);
}

LOCAL void
pr_percent(loops)
	long	loops;
{
	int	prozent;
static	int	prozent_last = -1;

	prozent = ((RW - loops) * 100) / RW;
	if (prozent > prozent_last) {
		printf("\r %d%% ", prozent);
		flush();
	}
	prozent_last = prozent;
}

LOCAL int
check_fault(scgp, op, buf, cur_pos, last_pos)
	SCSI	*scgp;
	int	(*op) __PR((SCSI *scgp, caddr_t bp, long addr, int cnt));
	char	*buf;
	long	cur_pos;
	long	last_pos;
{
	long	bad_block;
	int	i;

	if (scgp->scmd->error <= SCG_RETRYABLE) {
		error("\rBad Block found at: %ld\n", cur_pos);
		for (i = 0; i < 100; i++) {
			if (op == rw_verify) {
				if (verify(scgp, cur_pos, 1, &bad_block) < 0)
					break;
			} else {
				if ((*op)(scgp, buf, cur_pos, 1) < 0)
					break;
			}
		}
		if (i >= 100) {
			for (i = 0; i < 100; i++) {
				if (seek_scsi(scgp, last_pos) < 0)
					return (SEEK_FAULT);
				if (op == rw_verify) {
					if (verify(scgp, cur_pos, 1,
							&bad_block) < 0)
						break;
				} else {
					if ((*op)(scgp, buf, cur_pos, 1) < 0)
						break;
				}
			}
			if (i < 100) {
				error("SOFT DATA ERROR !!!!\n");
				return (SOFT_FAULT);
			} else {
				error("NO ERROR ????\n");
				return (NO_FAULT);
			}
		}
		error("HARD ERROR !!!!\n");
		return (HARD_FAULT);
	}
	return (DRV_ERR);
}

LOCAL void
#ifdef	__STDC__
add_bad(long bb, u_char err)
#else
add_bad(bb, err)
	long	bb;
	u_char	err;
#endif
{
	register int	i;

	if (get_nbad() >= MAXBAD) {
		display_bad();
		comerrno(EX_BAD, "Too many bad blocks (max is %d)\n", MAXBAD);
		/* NOTREACHED */
	}
	for (i = 0; i < rwbad; i++) {
		if (bad_blocks[i].block == bb && bad_blocks[i].code == err) {
			bad_blocks[i].count++;
			return;
		}
	}
	bad_blocks[rwbad].block = bb;
	bad_blocks[rwbad].code = err;
	bad_blocks[rwbad].count = 1;
	insert_bad(bb);
	rwbad++;
}

LOCAL void
display_bad()
{
	int	i;

	if (rwbad > 0)
		printf("Count	Block		Code\n");
	else
		printf("no errors\n");
	for (i = 0; i < rwbad; i++) {
		printf("%d	%lu		",
			bad_blocks[i].count,
			bad_blocks[i].block);
		display_code(bad_blocks[i].code);
	}
}

LOCAL void
#ifdef	__STDC__
display_code(u_char code)
#else
display_code(code)
	u_char	code;
#endif
{
	if ((code & DRV_ERR) == DRV_ERR) {
		printf("FATAL ERROR.\n");
		return;
	}

	if (code & READ_FAULT)
		printf("READ:");
	if (code & WRITE_FAULT)
		printf("WRITE:");
	if (code & VERIFY_FAULT)
		printf("VERIFY:");
	if (code & SEEK_FAULT)
		printf("SEEK:");

	if (code & DATA_LOST)
		printf(" DATA LOST");
	if (code & HARD_FAULT)
		printf(" constant error");
	if (code & SOFT_FAULT)
		printf(" partial error");
	if (code & NO_FAULT)
		printf(" not reproducable error");

	printf(".\n");
}
