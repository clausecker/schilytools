/* @(#)defect.c	1.39 09/07/13 Copyright 1988-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)defect.c	1.39 09/07/13 Copyright 1988-2009 J. Schilling";
#endif
/*
 *	Handle defect lists (user level copy)
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
#include <schily/utypes.h>
#include <schily/param.h>	/* Include various defs needed with some OS */
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/intcvt.h>
#include <schily/schily.h>

#include "dsklabel.h"

#include <scg/scgcmd.h>
#include <scg/scsidefs.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "scsicmds.h"
#include "defect.h"

#include "fmt.h"

defect	def;

extern	char	*Sbuf;
extern	long	Sbufsize;
extern	int	autoformat;
extern	struct dk_label *d_label;

EXPORT	void	convert_def_blk	__PR((SCSI *scgp));
EXPORT	void	write_def_blk	__PR((SCSI *scgp, BOOL));
EXPORT	void	read_def_blk	__PR((SCSI *scgp));
LOCAL	BOOL	scan_def	__PR((SCSI *scgp));
LOCAL	int	read_def	__PR((SCSI *scgp, long def_blk));
EXPORT	BOOL	edit_def_blk	__PR((void));
LOCAL	void	add_def		__PR((BOOL));
EXPORT	void	add_def_bfi	__PR((struct scsi_def_bfi *));
LOCAL	void	del_def		__PR((void));
LOCAL	void	list_def	__PR((void));
LOCAL	void	conv_def	__PR((BOOL));
LOCAL	int	cmp_def_bfi	__PR((struct scsi_def_bfi *, struct scsi_def_bfi *));
LOCAL	void	sort_def	__PR((void));

EXPORT void
convert_def_blk(scgp)
	SCSI	*scgp;
{
	register struct scsi_def_list *l;
	register int	ndef;
	register int	n;
	register int	i;
		int	length;

	if (is_ccs(scgp->dev) && read_defect_list(scgp, 3, SC_DEF_BFI, FALSE) >= 0)
		l = (struct scsi_def_list *)Sbuf;
	else
		l = (struct scsi_def_list *)&def;
	length = a_to_u_2_byte(l->hd.length);
	ndef = length/sizeof (l->def_bfi[0]);
	for (n = 0, i = 0; i < ndef; i++) {
		if (*(long *)l->def_bfi[i].bfi == -1)
			continue;
		if (n == 127) {
			if (!autoformat) {
				errmsgno(EX_BAD,
				"Notice: Too many defects for SUN defect list format.\n");
				errmsgno(EX_BAD,
				"Notice: This is not a problem with SCSI disks.\n");
			}
			break;
		}
		def.d_def[n++] = l->def_bfi[i];
	}
	n *= sizeof (struct scsi_def_bfi);

	i_to_2_byte(&def.d_magic, DMAGIC);
	i_to_2_byte(&def.d_size, n);
}

/*---------------------------------------------------------------------------
|
|	XXX Die Defektliste steht auf dem letzten Alternativen Zylinder
|	auf Kopf 0 und 1.
|	Alt: Die Defektliste steht auf dem 1. Alternativen Zylinder
|	Beginn auf Kopf 0 (9 Kopien).
|	Neu: Die Defektliste steht auf dem 2. Alternativen Zylinder
|	Kopf 0 und 1 (2 Kopien).
|
+---------------------------------------------------------------------------*/
EXPORT void
write_def_blk(scgp, v)
	SCSI	*scgp;
	BOOL	v;
{
	long	def_blk;
/*extern int verbose;*/

	def_blk = d_label->dkl_ncyl + d_label->dkl_acyl - 1;
	def_blk *= d_label->dkl_nhead * d_label->dkl_nsect;

	def_blk = d_label->dkl_ncyl;
	def_blk *= d_label->dkl_nhead * d_label->dkl_nsect;

/*printf("def_blk %d\n", def_blk); verbose=1;*/
	i_to_2_byte(&def.d_magic, DMAGIC);
	if (write_scsi(scgp, (caddr_t)&def, def_blk, 2) < 0)
		error("Could not write defect to disk\n");
/*verbose=0;*/

	def_blk += d_label->dkl_nsect;

	if (write_scsi(scgp, (caddr_t)&def, def_blk, 2) < 0)
		error("Could not write defect to disk\n");

	if (v)
		printf("Defect list written on disk.\n");
}

EXPORT void
read_def_blk(scgp)
	SCSI	*scgp;
{
	long	def_blk;
	int	ret;

	def_blk = d_label->dkl_ncyl + d_label->dkl_acyl - 1;
	def_blk *= d_label->dkl_nhead * d_label->dkl_nsect;

	def_blk = d_label->dkl_ncyl;
	def_blk *= d_label->dkl_nhead * d_label->dkl_nsect;

printf("def: %ld\n", def_blk);
	if ((ret = read_def(scgp, def_blk)) < 1) {
	printf("d_magic: %x d_size: %ld\n",
			a_to_u_2_byte(&def.d_magic),
			(long)a_to_u_2_byte(&def.d_size)/sizeof (struct scsi_def_bfi));
		def_blk += d_label->dkl_nsect;
		if ((ret = read_def(scgp, def_blk)) < 0) {
			error("Could not read defect from disk\n");
		}
	printf("d_magic: %x d_size: %ld\n",
			a_to_u_2_byte(&def.d_magic),
			(long)a_to_u_2_byte(&def.d_size)/sizeof (struct scsi_def_bfi));
	}
	if (ret != TRUE) {
		scan_def(scgp);
	}

	printf("d_magic: %x d_size: %ld\n",
			a_to_u_2_byte(&def.d_magic),
			(long)a_to_u_2_byte(&def.d_size)/sizeof (struct scsi_def_bfi));

	if (a_to_u_2_byte(&def.d_magic) == DMAGIC) {
		print_def_bfi((struct scsi_def_list *)&def);
		printf("Defect list read from disk.\n");
	} else {
		i_to_2_byte(&def.d_magic, 0);
		i_to_2_byte(&def.d_size, 0);
	}
}

LOCAL BOOL
scan_def(scgp)
	SCSI	*scgp;
{
	int	i;
	long	l;
	defect	xdef;

	scgp->silent++;
	if (read_capacity(scgp) < 0) {
		scgp->silent--;
		return (FALSE);
	}
	scgp->silent--;
	l = scgp->cap->c_baddr - 1;	/* Ist 2 Sektoren lang */
	for (i = 0; i < scgp->cap->c_baddr/300; i++, l--)
		if (read_def(scgp, l) == TRUE) {
			printf("Scan for defect list: found at: %ld n: %d\n",
								l, i);
			xdef = def;
/*			return (TRUE);*/
		} else if (a_to_u_4_byte(&def.d_magic) == 0x89898989) {
			printf("Scan for defect list: New found at: %ld n: %d\n",
								l, i);
		}
if (a_to_u_2_byte(&xdef.d_magic) == DMAGIC)
	def = xdef;
	printf("Scan for defect list: NOT found : %ld n: %d\n", l, i);
	return (FALSE);
}

LOCAL int
read_def(scgp, def_blk)
	SCSI	*scgp;
	long	def_blk;
{
	fillbytes((caddr_t)&def, sizeof (def), '\0');
	if (read_scsi(scgp, (caddr_t)&def, def_blk, 2) < 0)
		return (-1);
	return (a_to_u_2_byte(&def.d_magic) == DMAGIC);
}

EXPORT BOOL
edit_def_blk()
{
	char	line[128];
	char	*linep;
	int	n;
	int	rll = 0;

	i_to_2_byte(&def.d_magic, 0);	/* clean up SCSI def format field */
	for (;;) {
		printf("def> ");
		flush();
		if ((n = getline(line, 80)) == 0)
/*			return (FALSE);*/
			continue;
		if (n == EOF)
			exit(EX_BAD);
		linep = skipwhite(line);
		switch (linep[0]) {

		case '?':
		case 'h':
		case 'H':
			printf("Available commands are:\n");
			printf("a	add    defect\n");
			printf("d	delete defect\n");
			printf("l	list   defect\n");
			printf("q	quit this loop\n");
			printf("r	disk is not RLL (default)\n");
			printf("R	disk is RLL (multiply bfi by 1.5)\n");
			printf("c	convert defect list from/to RLL\n");
			break;
		case 'r':
			rll = 0;
			break;
		case 'R':
			rll = 1;
			break;
		case 'c':
		case 'C':
			conv_def(rll);
			break;
		case 'a':
		case 'A':
			add_def(rll);
			break;
		case 'd':
		case 'D':
			del_def();
			break;
		case 'l':
		case 'L':
			list_def();
			break;
		case 's':
		case 'S':
			sort_def();
			break;
		case 'q':
		case 'Q':
			return (TRUE);
		default:
			continue;
		}
	}
}

LOCAL void
add_def(rll)
	BOOL	rll;
{
	register struct scsi_def_list *l;
	register int	i;
	long	n;
	int	length;

	printf("add\n");

	l = (struct scsi_def_list *)&def;
	length = a_to_u_2_byte(l->hd.length);
	i = length/sizeof (l->def_bfi[0]);
	if (i >= 127) {
		error("Too many defects.\n");
		return;
	}

	n = -1L;
	getlong("Enter Cylinder", &n, 0L, 2048L);
	i_to_3_byte(l->def_bfi[i].cyl, n);
	n = -1L;
	getlong("Enter Head", &n, 0L, 2048L);
	l->def_bfi[i].head = n;
	n = -1L;
	getlong("Enter Bfi", &n, 0L, 40960L);
	if (rll)
		n += n/2;
	i_to_4_byte(l->def_bfi[i].bfi, n);

	length += sizeof (l->def_bfi[0]);
	i_to_2_byte(l->hd.length, length);
	sort_def();
}

EXPORT void
add_def_bfi(dp)
	struct scsi_def_bfi	*dp;
{
	register struct scsi_def_list *l;
	register int	i;
		int	length;

	l = (struct scsi_def_list *)&def;
	length = a_to_u_2_byte(l->hd.length);
	i = length/sizeof (l->def_bfi[0]);
	if (i >= 127) {
		error("Too many defects.\n");
		return;
	}
	l->def_bfi[i] = *dp;

	length += sizeof (l->def_bfi[0]);
	i_to_2_byte(l->hd.length, length);
	sort_def();
}

LOCAL void
del_def()
{
	register struct scsi_def_list *l;
	register int	i;
	long	n;
	int	length;

	printf("del\n");

	l = (struct scsi_def_list *)&def;
	length = a_to_u_2_byte(l->hd.length);
	i = length/sizeof (l->def_bfi[0]);
	n = -1L;
	getlong("Enter Defect to delete", &n, 0L, 2048L);
	if (n >= i)
		return;

	movebytes((char *)&l->def_bfi[n+1], (char *)&l->def_bfi[n],
				(i-n-1)*sizeof (struct scsi_def_bfi));
	length -= sizeof (l->def_bfi[0]);
	i_to_2_byte(l->hd.length, length);
}

LOCAL void
list_def()
{
	printf("list\n");
	print_def_bfi((struct scsi_def_list *)&def);
}

LOCAL void
conv_def(rll)
	BOOL	rll;
{
	register struct scsi_def_list *l;
	register int	i;
	register int	n;
	long	d;
	int	length;

	l = (struct scsi_def_list *)&def;
	length = a_to_u_2_byte(l->hd.length);
	n = length/sizeof (l->def_bfi[0]);

	for (i = 0; i < n; i++) {
		d = a_to_4_byte(l->def_bfi[i].bfi);
		if (rll)
			d += d/2;
		else
			d -= d/3;
		i_to_4_byte(l->def_bfi[i].bfi, d);
	}
}

LOCAL int
cmp_def_bfi(a, b)
	struct scsi_def_bfi	*a;
	struct scsi_def_bfi	*b;
{
	if (a_to_u_3_byte(a->cyl) > a_to_u_3_byte(b->cyl))
		return (1);
	if (a_to_u_3_byte(a->cyl) == a_to_u_3_byte(b->cyl)) {
		if (a->head > b->head)
			return (1);
		if (a->head == b->head) {
			if (a_to_4_byte(a->bfi) > a_to_4_byte(b->bfi))
				return (1);
			if (a_to_4_byte(a->bfi) == a_to_4_byte(b->bfi))
				return (0);
		}
	}
	return (-1);
}

LOCAL void
sort_def()
{
	register struct scsi_def_list *l;
	register int	i;
		int	length;

	l = (struct scsi_def_list *)&def;
	length = a_to_u_2_byte(l->hd.length);
	i = length/sizeof (l->def_bfi[0]);

	qsort((char *)&l->def_bfi[0], i, sizeof (struct scsi_def_bfi),
			(int (*)__PR((const void *, const void *)))cmp_def_bfi);
}
