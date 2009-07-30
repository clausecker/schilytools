/* @(#)repair.c	1.21 09/07/11 Copyright 1988-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)repair.c	1.21 09/07/11 Copyright 1988-2009 J. Schilling";
#endif
/*
 *	Repair SCSI disks
 *
 *	Copyright (c) 1988-2009 J. Schilling
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

#include <schily/stdio.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/signal.h>
#include <schily/sigset.h>
#include <schily/schily.h>
#include <schily/libport.h>

#include <scg/scgcmd.h>
#include <scg/scsireg.h>
#include <scg/scsidefs.h>
#include <scg/scsitransp.h>

#include "scsicmds.h"
#include "fmt.h"

#define	min(a, b)	((a) < (b) ? (a) : (b))
#define	max(a, b)	((a) > (b) ? (a) : (b))

extern	char	*Sbuf;
extern	long	Sbufsize;
extern	int	ask;
extern	int	ign_not_found;

LOCAL	int	soft_errs;

extern	int	autoformat;
extern	int	veri;
extern	int	wrveri;
extern	int	format_done;

extern	int	refresh_only;
extern	int	n_test_patterns;
#define	NWVERI	n_test_patterns
extern	int	Nveri;
extern	int	Cveri;
extern	int	CWveri;
extern	long	MAXbad;

EXPORT	int	verify_and_repair_disk	__PR((SCSI *scgp, struct disk *dp));
EXPORT	void	verify_disk		__PR((SCSI *scgp, struct disk *dp, int pass, long first, long last, long maxbad));
LOCAL	int	wr_verify		__PR((SCSI *scgp, struct disk *dp,
						long start, int count, long *bad_block,
						int (*wv_func) (SCSI *, caddr_t, long, int, long *)));


LOCAL	int	read_old_data		__PR((SCSI *scgp, long n));
LOCAL	int	refresh_block		__PR((SCSI *scgp, long n));
LOCAL	void	reassign_bad_block	__PR((SCSI *scgp, long n, int status));
LOCAL	int	check_stable		__PR((SCSI *scgp, long n));
EXPORT	void	ext_reassign_block	__PR((SCSI *scgp, long n));

LOCAL	void	fill_buf		__PR((int n));
LOCAL	void	refill_buf		__PR((void));

#define	MAXVERI	100
int	vlist[MAXVERI];

EXPORT int
verify_and_repair_disk(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	int	ret = 0;
	int	i;
	int	total;
	int	oldi;
	int	odef;
	int	ndef;

	clear_bad();
	odef = 0;

	if (Nveri < 0) {
		Nveri = wrveri ? NWVERI : NVERI;
		if (wrveri)
			Nveri *= 2;
	}
	if (dp->veri_time > 0)
		printf("Estimated time: %ld minutes\n",
				(Nveri * dp->veri_time + 30)/60);
	prdate();
	getstarttime();

	soft_errs = 0;
	ndef = 0;
	oldi = 0;
	total = 0;
newveri:
	for (i = 0; i < Nveri; ) {
		if (i > oldi)
			oldi = i;
		printf("Number of faultless passes: %d (up to now max: %d)\n",
					i, oldi);
		if (!scgp->silent) {
		int j; for (j = 0; j < 10; j++) printf("%d: %d  ", j, vlist[j]);
		printf("\n");
		}
#ifdef	__old__
		verify_disk(scgp, dp, i, 0L, -1L, 32L);	/* XXX Test fuer Sony SMO */
		verify_disk(scgp, dp, i, 0L, -1L, 127L);	/* dies ist das absolute Maximum */
#endif
		verify_disk(scgp, dp, i, 0L, -1L, (MAXbad <= 0L) ? 32L : MAXbad);
		ndef += print_bad();

		if (!is_ccs(scgp->dev)) {
			int	r;

			r = bad_to_def(scgp);
			if (r <= 0)
				clear_bad();
			if (odef + r < ndef)
				ndef = odef + r;
		}
		i++;
		if (ndef > odef)
			break;
	}
	if (i < MAXVERI)
		vlist[i]++;
	total += i;

	printf("Total of %2d defects.\n", ndef);
	printf("Total of %2d soft errors.\n", soft_errs);
	printf("Total of %2d verify loops done.\n", total);
	if (ndef > odef)
		odef = ndef;
	else if (i == Nveri)
		goto done;

	if (!is_ccs(scgp->dev)) {
		clear_bad();
		if (acb_format_disk(scgp, dp, TRUE) >= 0) {
			goto newveri;
		}
	} else {
		reassign_bad(scgp);
		clear_bad();
		if ((!autoformat || ndef <= 10) && reformat_disk(scgp, dp)) {
			goto newveri;
		}
	}

done:
	if (ndef > 10) {
		if (autoformat) {
			error("A C H T U N G\n");
			error("Diese Platte ist nicht brauchbar\n");
			error("Bitte Entwicklung benachrichtigen\n");
		} else {
			error("Excessive bad blocks on disk.\n");
		}
		ret = 1;
	}
	if (soft_errs > total) {
		if (autoformat) {
			error("A C H T U N G\n");
			error("Diese Platte hat zu viele soft errors\n");
			error("Bitte Entwicklung benachrichtigen\n");
		} else {
			error("Excessive soft errors on disk.\n");
		}
		ret = 1;
	}
	return (ret);
}

EXPORT void
verify_disk(scgp, dp, pass, first, last, maxbad)
	SCSI	*scgp;
	struct disk	*dp;
	int	pass;
	long	first;
	long	last;
	long	maxbad;
{
	long	bad_block;
	long	bb2;
	long	start = first;
	long	end;
	int	count;
	long	maxcount;
	int	i;
	long	bads = 0L;
	int	ret;
	BOOL	do_wrveri;
	int	(*wv_func) __PR((SCSI *, caddr_t, long, int, long *));
static	BOOL	force_wrveri = FALSE;

	if (Cveri < 0)
		Cveri = CVERI;
	if (CWveri < 0)
		CWveri = CWVERI;
	maxcount = Cveri;

	if (wrveri && !format_done && !force_wrveri) {
		printf("WARNING: Disk has not been formatted.\n");
		printf("WARNING: Write/verify will destroy disk data.\n");
		force_wrveri = yes("Do you want to force write/verify? ");
		if (force_wrveri) {
			read_sinfo(scgp, dp, FALSE);
			if (dp->formatted < 0) {
				testformat(scgp, dp);
				read_primary_label(scgp, dp);
			}
		}
	}

	if (force_wrveri)
		do_wrveri = (pass & 1) == 0;
	else
		do_wrveri = wrveri && format_done && (pass & 1);

	if (veri) {
		prdate();
		getstarttime();
	}
	if (read_capacity(scgp) < 0) {
		error("Cannot read Capacity\n");
		return;
	}
	end = scgp->cap->c_baddr;
	if (last > 0 && last < end)
		end = last;
	if (start > end)
		start = first = 0L;
	if (do_wrveri) {
		maxcount = Sbufsize/scgp->cap->c_bsize;
		if (maxcount > CWveri)
			maxcount = CWveri;
		maxcount /= 10;
		maxcount *= 10;
		fill_buf(pass>>1);
		printf("Verify pass %d Test Pattern: %lX\n",
							pass, *(long *)Sbuf);
		if (dp->split_wv_cmd > 0)
			wv_func = write_verify_split;
		else
			wv_func = write_verify;

	} else {
/*		printf("no write\n");*/
/*		printf("\n");*/
		wv_func = write_verify;	/* Tell lint: it's initialized */
	}

/*	for(start = 0; start < end; ) {*/
	while (start <= end) {
		printf("\r%ld", start); flush();
		if (scgp->dev == DEV_ACB40X0 ||
		    scgp->dev == DEV_ACB4000 || scgp->dev == DEV_ACB4010 || scgp->dev == DEV_ACB4070)
		xdelay(); /*usleep(10000);*/		/* allow other procs*/
							/* to run if disre  */
							/* not enabled	    */

		count = maxcount;
		count -= start % maxcount;
		count = min(count, end - start + 1);
		wait_unit_ready(scgp, 100);
		if (do_wrveri)
			ret = wr_verify(scgp, dp, start, count, &bad_block, wv_func);
		else
			ret = verify(scgp, start, count, &bad_block);
		if (scgp->debug)
			error("SCG->error: %d\n", scgp->scmd->error);
		if (ret < 0) {
			if (scgp->scmd->error <= SCG_RETRYABLE ||
			    scgp->scmd->error == SCG_TIMEOUT) {
				printf("\rBad Block # %ld found at: %ld\n", bads+1, bad_block);
				if (!scgp->scmd->sense.adr_val) {
					printf("Not valid!!! start: %ld\n",
									start);

					wait_unit_ready(scgp, 100);
					if (verify(scgp, start, 1, &bad_block) >= 0 ||
					    !scgp->scmd->sense.adr_val) {

						/*
						 * Wer weis, was da fuer Schrott
						 * kommt ??? vielleicht 0 !
						 * Darum sicher ist sicher !!
						 */
						start++;
						continue;
					}
				}
				if ((ign_not_found & scg_sense_code(scgp)) == 0x14) {
/*					error("Record not found: %ld\n",*/
					printf("Record not found: %ld\n",
							bad_block);
					if (bad_block < start)
						start++;
					else
						start = bad_block + 1;
					continue;
				}
				/* Wait for settle down */
				usleep(200000);
				wait_unit_ready(scgp, 100);
				if (scgp->scmd->error == SCG_TIMEOUT) {
					error("TIMEOUT, sleeping for drive to calm dowm");
					sleep(20);
				}
				scgp->silent++;
				for (i = 0; i < 16; i++)
					if (verify(scgp, bad_block, 1, &bb2) < 0)
						break;
				scgp->silent--;
				if (scgp->scmd->error == SCG_TIMEOUT) {
					sleep(20);
				}
				/*error("i : %d\n", i);*/
				if (i < 16) {
					insert_bad(bad_block);
#ifdef	OLD
/*XXX*/					if (maxbad > 0 && ++bads >= maxbad)
#else
					bads++;
/*XXX*/					if (maxbad > 0 && bads >= maxbad)
#endif
						return;
				} else {
/*					error("SOFT ERROR !!!! at: %ld\n",*/
					printf("SOFT ERROR !!!! at: %ld\n",
								bad_block);
					soft_errs++;
				}
				if (bad_block < start)
					start++;
				else
					start = bad_block + 1;
				continue;
			}
			scg_printerr(scgp);
			if (scgp->debug) {
				error("SCG->error: %d\n", scgp->scmd->error);
				error("RETURN\n");
			}
			return;
		}
		start += count;
	}
	printf("\r%ld\nVerify done.\n", start);
	if (veri) {
		int	nsec;

		nsec = prstats();
		printf("Verify speed: %.1f kB/sec\n",
			((start - first)/(1024.0/scgp->cap->c_bsize)) / nsec);
	}
}


/*---------------------------------------------------------------------------
|
|	Darf nur aufgerufen werden, wenn die Platte formatiert ist, sonst
|	sind eventuell das Label sowie sinfo nicht initialisiert,
|	sowie wie moeglicherweise wichtige Daten auf der Platte.
|
+---------------------------------------------------------------------------*/
LOCAL int
wr_verify(scgp, dp, start, count, bad_block, wv_func)
	SCSI	*scgp;
	struct disk	*dp;
	long	start;
	int	count;
	long	*bad_block;
	int	(*wv_func) __PR((SCSI *, caddr_t, long, int, long *));
{
	struct scg_cmd	cmdsave;
	int	ret;
	sigset_t	oldmask;

	block_sigs(oldmask);
	ret = (*wv_func)(scgp, (caddr_t)Sbuf, start, count, bad_block);
	if (start == 0 || (start + count) >= scgp->cap->c_baddr) {
		movebytes((caddr_t)scgp->scmd, (caddr_t)&cmdsave, sizeof (cmdsave));
		scgp->silent++;
		write_sinfo(scgp, dp);
		label_disk(scgp, dp);
		convert_def_blk(scgp);
		write_def_blk(scgp, FALSE);
		scgp->silent--;
		movebytes((caddr_t)&cmdsave, (caddr_t)scgp->scmd, sizeof (cmdsave));
		refill_buf();	/* Solange 'Sbuf' vielfach benutzt wird */
	}
	restore_sigs(oldmask);
	return (ret);
}

#define	RECOVERED	0x01
#define	READ_OK		0x02
#define	REFRESHED	0x04
#define	UNSTABLE	0x08

LOCAL int
read_old_data(scgp, n)
	SCSI	*scgp;
	long	n;
{
	int	ret = 0;
	int	i;

	for (i = 0; i < 100; i++) {
		if (read_scsi(scgp, Sbuf, n, 1) >= 0 && scg_getresid(scgp) == 0) {
			ret |= READ_OK;
			break;
		} else {
			if (scg_sense_key(scgp) == SC_RECOVERABLE_ERROR &&
							scg_getresid(scgp) == 0) {
				ret |= RECOVERED;
				break;
			}
		}
	}
	if (i < 100) {
		printf("successful%s (%d).\n", (ret & RECOVERED) ?
			" recovered" : "", i);
		if (ret & RECOVERED) {
			printf("\n");
			scg_printerr(scgp);
			printf("\n");
		}
	} else {
		printf("\nCannot read data from block %ld.\n\n", n);
		scg_printerr(scgp);
		printf("\n");
	}
	return (ret);
}

LOCAL int
refresh_block(scgp, n)
	SCSI	*scgp;
	long	n;
{
	if (ask && !yes("Refresh block %ld ? ", n))
		return (0);

	if (write_scsi(scgp, Sbuf, n, 1) >= 0 && scg_getresid(scgp) == 0) {
		return (REFRESHED);
	} else {
		printf("Cannot refresh block %ld.\n\n", n);
		scg_printerr(scgp);
		printf("\n");
	}
	return (0);
}

LOCAL void
reassign_bad_block(scgp, n, status)
	SCSI	*scgp;
	long	n;
	int	status;
{
	struct scsi_def_list d;
	sigset_t	oldmask;

	block_sigs(oldmask);

	i_to_4_byte(d.def_list.list_block[0], n);

	if (reassign_block(scgp, &d, 1) < 0) {
		printf("Cannot reassign block %ld\n\n", n);
		scg_printerr(scgp);
		printf("\n");
	} else {
		if (!(status & (READ_OK|RECOVERED)))
			fillbytes((caddr_t) Sbuf, (int)(2 * scgp->cap->c_bsize), '\0');

		if (write_scsi(scgp, Sbuf, n, 1) < 0 || scg_getresid(scgp) != 0) {
			printf("Alternate sector (%ld) not usable.\n\n", n);
			scg_printerr(scgp);
			goto out;
		}
		printf("Block is reassigned %s",
			(status & (READ_OK|RECOVERED)) ?
			"and old data is written on alternate sector.\n" :
			"and old data is lost.\n");

		printf("Verifying data... ");
		flush();
		if (read_scsi(scgp, Sbuf + MAX_SECSIZE, n, 1) < 0 || scg_getresid(scgp) != 0) {
			printf("Alternate sector (%ld) not usable.\n\n", n);
			scg_printerr(scgp);
			goto out;
		}
		if (cmpbytes(Sbuf, Sbuf + MAX_SECSIZE, scgp->cap->c_bsize) <
								scgp->cap->c_bsize)
			printf("could not read back same data from disk\n");
		else {
			printf("OK\n");
			restore_sigs(oldmask);

			(void) check_stable(scgp, n);
		}
	}
out:
	restore_sigs(oldmask);
}

LOCAL int
check_stable(scgp, n)
	SCSI	*scgp;
	long	n;
{
	int	i;
	long	altaddr;
	long	dummy;

#define	UNSTABLE	0x08

	altaddr = (n - 1000L) > 0L ? n - 1000L : n + 1000L;

	for (i = 0; i < 1000; i++) {
		if (verify(scgp, n, 1, &dummy) < 0) {
			printf("Block is unstable (%d).\n\n", i);
			scg_printerr(scgp);
			printf("\n");
			return (UNSTABLE);
		}
		if (i % 200 == 0)
			read_scsi(scgp, &((char *)Sbuf)[MAX_SECSIZE], altaddr, 1);
	}
	return (0);
}

EXPORT void
ext_reassign_block(scgp, n)
	SCSI	*scgp;
	long	n;
{
	int	status = 0;

	scgp->silent++;
	printf("Trying to read old data... ");
	flush();
	status |= read_old_data(scgp, n);
	printf("Trying to refresh block...\n");
	status |= refresh_block(scgp, n);
	if (status & REFRESHED) {
		printf("Block %ld is refreshed %s\n", n,
			((status & READ_OK) || (status & RECOVERED)) ?
			"and old data is written on it." :
			"and old data is lost.");
		status |= check_stable(scgp, n);
		if (!(status & UNSTABLE) &&
				(refresh_only ||
				!yes("Do you still want to reassign block? "))) {
			scgp->silent--;
			return;
		}
	}
	if (refresh_only) {
		printf("Block %ld is %s, but will not be reassigned!\n", n,
				(status&UNSTABLE)?"unstable":"defect");
		scgp->silent--;
		return;
	}
	if (ask && !yes("Reassign block %ld ? ", n)) {
		scgp->silent--;
		return;
	}
	reassign_bad_block(scgp, n, status);
	scgp->silent--;
}

unsigned long	test_patterns[] = {
	0xc6dec6de,
	0x6db6db6d,
	0x00000000,
	0xffffffff,
	0xaaaaaaaa,
	0x55555555,
	0x4A536368,
};
int	n_test_patterns = sizeof (test_patterns)/sizeof (test_patterns[0]);
int	last_pattern = 0;	/* Solange 'Sbuf' vielfach benutzt wird */

LOCAL void
fill_buf(n)
	int	n;
{
	register int i = Sbufsize/sizeof (unsigned long);
	register unsigned long pattern = test_patterns[n%n_test_patterns];
	register unsigned long *lp = (unsigned long *)Sbuf;

	last_pattern = n;	/* Solange 'Sbuf' vielfach benutzt wird */

	while (--i >= 0)
		*lp++ = pattern;
}

/* Solange 'Sbuf' vielfach benutzt wird */
LOCAL void
refill_buf()
{
	fill_buf(last_pattern);
}
