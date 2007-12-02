/* @(#)autopart.c	1.22 07/02/22 Copyright 1995-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)autopart.c	1.22 07/02/22 Copyright 1995-2004 J. Schilling";
#endif
/*
 *	Automatic genation of partition tables
 *
 *	Copyright (c) 1995-2004 J. Schilling
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
#include <stdio.h>
#include <schily/utypes.h>
#ifdef	HAVE_SYS_PARAM_H
#include <sys/param.h>	/* Include various defs needed with some OS */
#endif
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/schily.h>
#include "dsklabel.h"

#include "fmt.h"

#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "scsicmds.h"

EXPORT	BOOL	get_part_defaults __PR((SCSI *scgp, struct disk *, struct dk_label *));

#define	REST	(-1)			/* take the rest of the disk */
#define	INF	(~(unsigned long)0)	/* a very big disk */

struct ptab {
	int	part[NDKMAP];
};

struct default_parts {
	unsigned long	min_cap;
	unsigned long	max_cap;
	struct ptab	parts;
} default_parts[] = {
	{ 0,	64,	{{  0,   0, 0, 0, 0, 0, REST, 0 }} },
	{ 64,	180,	{{ 16,  16, 0, 0, 0, 0, REST, 0 }} },
#ifdef	SVR4
	{ 180,	380,	{{ 24,  32, 0, 0, 0, 0, REST, 0 }} },
	{ 380,	680,	{{ 24,  64, 0, 0, 0, 0, REST, 0 }} },
	{ 680,	1024,	{{ 24, 128, 0, 0, 0, 0, REST, 0 }} },
	{ 1024,	2048,	{{ 32, 128, 0, 0, 0, 0, REST, 0 }} },
	{ 2048,	INF,	{{ 64, 128, 0, 0, 0, 0, REST, 0 }} },
#else
	{ 180,	380,	{{ 16,  32, 0, 0, 0, 0, REST, 0 }} },
	{ 380,	680,	{{ 16,  64, 0, 0, 0, 0, REST, 0 }} },
	{ 680,	1024,	{{ 15, 128, 0, 0, 0, 0, REST, 0 }} },
	{ 1024,	2048,	{{ 24, 128, 0, 0, 0, 0, REST, 0 }} },
	{ 2048,	INF,	{{ 32, 128, 0, 0, 0, 0, REST, 0 }} },
#endif
};

#define	NPART_ENTS	(sizeof (default_parts) / sizeof (struct default_parts))

EXPORT BOOL
get_part_defaults(scgp, dp, lp)
	SCSI		*scgp;
	struct disk	*dp;
	register struct dk_label *lp;
{
	int	i;
	double	dcap;
	double	dmax;
	double	dmin;
	int	scyl;
	int	ncyl;
	long	l;
	struct ptab *pt;
	char	*p;

	if (scgp->cap->c_baddr <= 0) {
		if (read_capacity(scgp) < 0 &&
			dp->secsize > 0 && dp->capacity > 0) {
			scgp->cap->c_bsize = dp->secsize;
			scgp->cap->c_baddr = dp->capacity - 1;
		} else {
			return (FALSE);
		}
	}
	dcap = (scgp->cap->c_baddr+1.0) * (scgp->cap->c_bsize*1.0) / (1.024*1.024);
	for (i = 0; i < NPART_ENTS; i++) {
		dmin = default_parts[i].min_cap * 1024.0 * 1024.0;
		dmax = default_parts[i].max_cap * 1024.0 * 1024.0;
		if (dcap >= dmin && dcap < dmax)
			break;
	}
	if (i == NPART_ENTS) {
		errmsgno(EX_BAD,
			"No autopart range found (this should not happen).\n");
		return (FALSE);
	}
	pt = &default_parts[i].parts;

	if (default_parts[i].max_cap == INF) {
		printf("Using autopart range %lu MB - Infinity\n",
			default_parts[i].min_cap);
	} else {
		printf("Using autopart range %lu MB - %lu MB\n",
			default_parts[i].min_cap, default_parts[i].max_cap);
	}

	if (dp->lncyl < 0)
		dp->lncyl = get_default_lncyl(scgp, dp);

	scyl = 0;
	for (i = 0; i < NDKMAP; i++) {
		if (pt->part[i]) {
			lp->dkl_map[i].dkl_cylno = scyl;
			l = pt->part[i];

			if (l == INF) {
				ncyl = dp->lncyl - scyl;
			} else {
				l *= 2048;
				ncyl = l / (dp->lhead * dp->lspt);

				if (l % (dp->lhead * dp->lspt))
					ncyl++;
			}
			l = ncyl * (dp->lhead * dp->lspt);
			lp->dkl_map[i].dkl_nblk = l;
			scyl += ncyl;
		}
	}

	/*
	 * Create ascilabel from vendor_info & prod_ident,
	 * skip multiple blanks.
	 */
	strncpy(lp->dkl_asciilabel, scgp->inq->vendor_info, sizeof (scgp->inq->vendor_info));
	p = &lp->dkl_asciilabel[sizeof (scgp->inq->vendor_info)-1];
	while (*p == ' ' && p > lp->dkl_asciilabel)
		p--;
	if (p > lp->dkl_asciilabel)
		p += 2;

	strncpy(p, scgp->inq->prod_ident, sizeof (scgp->inq->prod_ident));
	p[sizeof (scgp->inq->prod_ident)] = '\0';

	if ((p = strchr(p, ' ')) != NULL)
		*p = '\0';

	set_default_vtmap(lp);

	lp->dkl_magic = DKL_MAGIC;
	lp->dkl_cksum = do_cksum(lp);
	setlabel_from_val(scgp, dp, lp);
	return (TRUE);
}
