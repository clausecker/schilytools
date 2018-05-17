/* @(#)acb4000.c	1.26 09/07/13 Copyright 1989-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)acb4000.c	1.26 09/07/13 Copyright 1989-2009 J. Schilling";
#endif
/*
 *	Routines for ADAPTEC 40xx & 5000 series.
 *
 *	Copyright (c) 1989-2009 J. Schilling
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
#include <schily/param.h>	/* Include various defs needed with some OS */
#include <schily/standard.h>
#include <schily/schily.h>

#include <scg/scgcmd.h>
#include <scg/scsireg.h>
#include <scg/scsidefs.h>
#include <scg/scsitransp.h>

#include "scsicmds.h"
#include "fmt.h"

EXPORT	int	acbdev 			__PR((SCSI *scgp));
EXPORT	BOOL	get_acb4000defaults	__PR((SCSI *scgp, struct disk *));
EXPORT	BOOL	set_acb4000params	__PR((SCSI *scgp, struct disk *));

EXPORT int
acbdev(scgp)
	SCSI	*scgp;
{
	struct scsi_mode_data	md;
	long	blocksize;
	long	sectorcnt;

	fillbytes((caddr_t)&md, sizeof (md), '\0');

	if (mode_sense(scgp, (u_char *)&md, 24, 0, 0) < 0)
		return (scgp->dev);

	blocksize = a_to_u_3_byte(md.blockdesc.lblen);
	sectorcnt = md.pagex.acb.sect_per_trk;
if (scgp->silent < 2)
printf("blocksize: %ld sectorcnt: %ld\n", blocksize, sectorcnt);

	/*
	 * Wenn 'blocksize' und 'sectorcnt'
	 * auf ihren Defaultwerten sind, dann läßt sich der
	 * Typ des Kontrollers nicht bestimmen.
	 */
	if (blocksize == 256 && sectorcnt == 32)
		return (scgp->dev);

	if (blocksize * sectorcnt > 10416) /* 5MB/s / 60rps / 8bit/Byte */
		return (DEV_ACB4070);

	if (md.pagex.acb.hard_sec && (scgp->dev == DEV_ACB40X0 || scgp->dev == DEV_ACB4000))
		return (DEV_ACB4010);

	return (DEV_ACB4000);
}

EXPORT BOOL
get_acb4000defaults(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	struct scsi_mode_data	md;
	long	tmp;

	fillbytes((caddr_t)&md, sizeof (md), '\0');

	(void) test_unit_ready(scgp);
	if (mode_sense(scgp, (u_char *)&md, 24, 0, 0) < 0)
		return (FALSE);

	tmp = a_to_u_3_byte(md.blockdesc.lblen);
	if (dp->secsize < 0 && tmp != 0)
		dp->secsize = tmp;

	tmp = a_to_u_2_byte(md.pagex.acb.ncyl);
	if (dp->pcyl < 0 && tmp != 0)
		dp->pcyl = tmp;
	if (dp->lpcyl < 0 && tmp != 0)
		dp->lpcyl = tmp;

	tmp = md.pagex.acb.nhead;
	if (dp->nhead < 0 && tmp != 0)
		dp->nhead = tmp;

	tmp = a_to_u_2_byte(md.pagex.acb.start_red_wcurrent);
	if (dp->reduced_curr < 0 && tmp != 0)
		dp->reduced_curr = tmp;

	tmp = a_to_u_2_byte(md.pagex.acb.start_precomp);
	if (dp->write_precomp < 0 && tmp != 0)
		dp->write_precomp = tmp;

	tmp = md.pagex.acb.step_rate;
	if (dp->step_rate < 0 && tmp != 0)
		dp->step_rate = tmp;

	tmp = md.pagex.acb.sect_per_trk;
	if (dp->spt < 0 && tmp != 0)
		dp->spt = tmp;

	return (TRUE);
}

EXPORT BOOL
set_acb4000params(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	struct scsi_mode_data	md;
	int	mode_len = 24;

	fillbytes((caddr_t)&md, sizeof (md), '\0');

	(void) test_unit_ready(scgp);
	if (mode_sense(scgp, (u_char *)&md, mode_len, 0, 0) < 0)
		return (FALSE);

	i_to_3_byte(md.blockdesc.nlblock, 0);
	i_to_3_byte(md.blockdesc.lblen, dp->secsize);
/*#ifdef	notneeded*/
	/*
	 * Listformat muß laut Handbuch bei softsektorierten Festplatten
	 * auf '1', in allen anderen Fällen auf '2' gesetzt werden.
	 *
	 * Es scheint aber nur die Anzahl der Bytes zu steuern, die bei
	 * einem MODE_SELECT entgegengenommen werden.
	 * Da wir über die korrekten Daten von MODE_SENSE verfügen,
	 * können wir auf diese Unterscheidung verzichten.
	 */
	if (md.pagex.acb.fixed_media == 1 && md.pagex.acb.hard_sec == 0) {
		md.pagex.acb.listformat = 1;
		mode_len = 22;
	} else {
		md.pagex.acb.listformat = 2;
	}
/*#endif*/
	i_to_2_byte(md.pagex.acb.ncyl, dp->pcyl);
	md.pagex.acb.nhead = dp->nhead;
	i_to_2_byte(md.pagex.acb.start_red_wcurrent, dp->reduced_curr);
	i_to_2_byte(md.pagex.acb.start_precomp, dp->write_precomp);
	md.pagex.acb.landing_zone = 0;
	md.pagex.acb.step_rate = dp->step_rate;
	md.pagex.acb.sect_per_trk = dp->spt;

	if (mode_select(scgp, (u_char *)&md, mode_len, 0, 0) < 0)
		return (FALSE);
	return (TRUE);
}
