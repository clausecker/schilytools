/* @(#)badblock.c	1.21 09/10/16 Copyright 1988-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)badblock.c	1.21 09/10/16 Copyright 1988-2009 J. Schilling";
#endif
/*
 *	Handle defects (SCSI level)
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
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/stdio.h>
#include <schily/types.h>
#include <schily/stdlib.h>
#include <schily/standard.h>
#include <schily/schily.h>

#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "scsicmds.h"
#include "defect.h"
#include <schily/intcvt.h>
#include "fmt.h"

extern	int	reassign;

extern	int	format_done;

LOCAL	long	bad[1024];
LOCAL	int	maxbad	= (sizeof (bad)-sizeof (struct scsi_def_header))/sizeof (bad[0]);
LOCAL	int	num_bad;

EXPORT	void	clear_bad		__PR((void));
EXPORT	int	get_nbad		__PR((void));
EXPORT	void	insert_bad		__PR((long baddr));
EXPORT	int	print_bad		__PR((void));
EXPORT	int	bad_to_def		__PR((SCSI *scgp));
EXPORT	void	reassign_bad		__PR((SCSI *scgp));
LOCAL	void	reassign_found_blocks	__PR((SCSI *scgp, int nbad));
EXPORT	void	repair_found_blocks	__PR((SCSI *scgp, int nbad));
EXPORT	void	reassign_one		__PR((SCSI *scgp));
EXPORT	int	reassign_one_block	__PR((SCSI *scgp, long n));
EXPORT	void	print_defect_list	__PR((struct scsi_def_list *l));
LOCAL	void	print_def_block		__PR((struct scsi_def_list *l));
EXPORT	void	print_def_bfi		__PR((struct scsi_def_list *l));
LOCAL	void	print_def_phys		__PR((struct scsi_def_list *l));
LOCAL	void	print_def_head		__PR((struct scsi_def_list *l, int ndef));

EXPORT void
clear_bad()
{
	fillbytes((caddr_t)bad, sizeof (bad), '\0');
	num_bad = 0;
}

EXPORT int
get_nbad()
{
	return (num_bad);
}

EXPORT void
insert_bad(baddr)
	register long	baddr;
{
	register int	i;
	register long	x;

	for (i = 1; i <= num_bad; i++) {
		x = a_to_4_byte(&bad[i]);
		if (x == baddr)
			return;
	}
	if (++num_bad > maxbad)
		comerrno(EX_BAD, "Too many bad blocks (max is %d)\n", maxbad);
	i_to_4_byte(&bad[num_bad], baddr);
}

#ifdef	used
cmp_bad(ap, bp)
	long	*ap;
	long	*bp;
{
	long	la;
	long	lb;

	la = a_to_4_byte(ap);
	lb = a_to_4_byte(bp);
	return (la - lb);
}

sort_bad()
{
	qsort((caddr_t)&bad[1], num_bad, sizeof (bad[0]), cmp_bad);
}
#endif

EXPORT int
print_bad()
{
	register int	i;

	printf("Found %d defects:\n", num_bad);
	for (i = 1; i <= num_bad; i++)
		printf("%ld\n", a_to_4_byte(&bad[i]));
	return (num_bad);
}

EXPORT int
bad_to_def(scgp)
	SCSI	*scgp;
{
	register int	err	= 0;
	register int	n	= 0;
	register int	i;
	register int	j;
	struct scsi_def_list defl;

	fillbytes((caddr_t)&defl, sizeof (defl), '\0');
	i = sizeof (struct scsi_def_bfi);	/* Ein Defekt */
	i_to_2_byte(defl.hd.length, i);

	for (i = 1; i <= num_bad; i++) {
		if (translate(scgp, &defl.def_bfi[0],
						a_to_4_byte(&bad[i])) < 0) {
			scgp->silent++;
			for (j = 0; j < 16; j++) {
				if (translate(scgp, &defl.def_bfi[0],
						a_to_4_byte(&bad[i])) >= 0)
					break;
			}
			scgp->silent--;
			if (j == 16) {
				err++;
				error("Repair Block %ld by hand.\n\n",
						a_to_4_byte(&bad[i]));
				continue;
			}
		}
		n++;
		print_def_bfi(&defl);
		add_def_bfi(&defl.def_bfi[0]);
	}
	if (err) {
		printf("%d Block%s could not be translated.\n",
						err, err > 1 ? "s":"");
	}
	return (n);
}

EXPORT void
reassign_bad(scgp)
	SCSI	*scgp;
{
	if (format_done) {
#ifdef	this_really_is_correct
		/*
		 * Some disks (e.g. IBM) have problems when getting
		 * a list that contains more than one entry.
		 */
		(void) reassign_block((struct scsi_def_list *)bad, num_bad);
		(void) rezero_unit();
#else
		reassign_found_blocks(scgp, num_bad);
#endif
	} else if (yes("Reassign found bad blocks? ")) {
		printf("The disk has not been modified now.\n");
		printf("If the data may get entirely lost and you don't want to wait a long time\n");
		printf("for trying to retrieve old data, you may choose the quick way.\n");
		printf("NOTE: if the disk is not formatted after that, the blocks may remain bad.\n");
		if (yes("Quick reassign of bad blocks ?"))
			reassign_found_blocks(scgp, num_bad);
		else
			repair_found_blocks(scgp, num_bad);
	}
}

#ifdef	used
copy_def_block()
{
	register struct scsi_def_list *l;
	register int	ndef;
	register int	n;
	register int	i;
	register long	x;

	l = (struct scsi_def_list *)Sbuf;
	i = a_to_u_2_byte(l->hd.length);
	ndef = i/sizeof (l->def_phys[0]);

	for (n = 1, i = 0; i < ndef; i++) {
		x = a_to_4_byte(l->def_block[i]);
		if (x == -1)
			continue;
		i_to_4_byte(&bad[n++], x);
	}
	num_bad = --n;
}
#endif

LOCAL void
reassign_found_blocks(scgp, nbad)
	SCSI	*scgp;
	int	nbad;
{
	int	i;

	for (i = 1; i <= nbad; i++) {
		(void) reassign_one_block(scgp, a_to_4_byte(&bad[i]));
		(void) rezero_unit(scgp);
	}
}

EXPORT void
repair_found_blocks(scgp, nbad)
	SCSI	*scgp;
	int	nbad;
{
	int	i;

	for (i = 1; i <= nbad; i++) {
		(void) ext_reassign_block(scgp, a_to_4_byte(&bad[i]));
		(void) rezero_unit(scgp);
	}
}

EXPORT void
reassign_one(scgp)
	SCSI	*scgp;
{
	long	b = -1L;

	if (!reassign) {
		error("WARNING: If you are no guru hit ^C and try -reassign option\n");
		getlong("Block Address", &b, 0L, scgp->cap->c_baddr);
		(void) reassign_one_block(scgp, b);
		(void) rezero_unit(scgp);
	} else {
		do {
			getlong("Block Address", &b, 0L, scgp->cap->c_baddr);
			(void) ext_reassign_block(scgp, b);
			(void) rezero_unit(scgp);
		} while (yes("Do you wish to reassign another block? "));
	}
	exit(0);
}

EXPORT int
reassign_one_block(scgp, n)
	SCSI	*scgp;
	long	n;
{
	struct scsi_def_list d;

	i_to_4_byte(d.def_list.list_block[0], n);
	return (reassign_block(scgp, &d, 1));
}

EXPORT void
print_defect_list(l)
	struct scsi_def_list	*l;
{
	printf("format: %d\n", (int)l->hd.format);
	switch (l->hd.format) {

	case SC_DEF_BLOCK :
	case SC_DEF_BLOCK | 1 :
	case SC_DEF_BLOCK | 2 :
	case SC_DEF_BLOCK | 3 :
			print_def_block(l);
			return;
	case SC_DEF_BFI  :
			print_def_bfi(l);
			return;
	case SC_DEF_PHYS :
			print_def_phys(l);
			return;
	default:
			printf("Unknown defect list format\n");
			return;
	}

}

LOCAL void
print_def_block(l)
	register struct scsi_def_list *l;
{
	register int	ndef;
	register int	i;

	i = a_to_u_2_byte(l->hd.length);
	ndef = i/sizeof (l->def_block[0]);

	print_def_head(l, ndef);
	for (i = 0; i < ndef; i++) {
		printf("%3d Block %7ld\n", i, a_to_4_byte(l->def_block[i]));
	}
	flush();
}

EXPORT void
print_def_bfi(l)
	register struct scsi_def_list *l;
{
	register int	ndef;
	register int	i;

	i = a_to_u_2_byte(l->hd.length);
	ndef = i/sizeof (l->def_bfi[0]);

	print_def_head(l, ndef);
	for (i = 0; i < ndef; i++) {
		printf("%3d Cyl %5lu  Head %2d  bfi %7ld\n",
			i,
			a_to_u_3_byte(l->def_bfi[i].cyl),
			l->def_bfi[i].head,
			a_to_4_byte(l->def_bfi[i].bfi));
	}
	flush();
}

LOCAL void
print_def_phys(l)
	register struct scsi_def_list *l;
{
	register int	ndef;
	register int	i;

	i = a_to_u_2_byte(l->hd.length);
	ndef = i/sizeof (l->def_phys[0]);

	print_def_head(l, ndef);
	for (i = 0; i < ndef; i++) {
		printf("%3d Cyl %5lu  Head %2d  Sec %2ld\n",
			i,
			a_to_u_3_byte(l->def_phys[i].cyl),
			l->def_phys[i].head,
			a_to_4_byte(l->def_phys[i].sec));
	}
	flush();
}

LOCAL void
print_def_head(l, ndef)
	register struct scsi_def_list *l;
		int	ndef;
{
	if (l->hd.mdl)
		printf("Manufacturer defect list ");
	if (l->hd.gdl)
		printf("Grown defect list ");
	if (ndef > 1)
		printf("total of %d defects.\n", ndef);
	else if (l->hd.mdl || l->hd.gdl)
		printf("\n");
}
