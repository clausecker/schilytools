/* @(#)diskfmt.c	1.30 15/11/18 Copyright 1988-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)diskfmt.c	1.30 15/11/18 Copyright 1988-2015 J. Schilling";
#endif
/*
 *	Format SCSI disks
 *
 *	Copyright (c) 1988-2015 J. Schilling
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
#include <schily/standard.h>
#include <schily/signal.h>
#include <schily/sigset.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/wait.h>
#include <schily/time.h>
#include <schily/string.h>
#include <schily/hostname.h>
#include <schily/schily.h>

#include "dsklabel.h"

#include <scg/scgcmd.h>
#include <scg/scsireg.h>
#include <scg/scsidefs.h>
#include <scg/scsitransp.h>

#include "scsicmds.h"

#include "defect.h"
#include "fmt.h"

#define	strindex(s1, s2)	strstr((s2), (s1))

extern	struct timeval	starttime;

extern	char	*Sbuf;
extern	long	Sbufsize;
extern	int	xdebug;
extern	int	debug;
extern	int	force;

extern	int	wrveri;
extern	int	n_test_patterns;
#define	NWVERI	n_test_patterns
extern	int	Nveri;
extern	int	Cveri;
extern	int	CWveri;
extern	int	noformat;
extern	int	save_mp;
extern	int	defmodes;
extern	int	no_heuristic_defaults;

	struct scsi_format_data	fmt;

extern	int	Prpart;
extern	int	label;
extern	int	autoformat;
extern	int	disable_mdl;
extern	int	setmodes;
extern	int	format_confirmed;
extern	int	format_done;

extern	struct disk		cur_disk;
extern	struct disk		alt_disk;

extern	struct	dk_label	*d_label;

extern	defect			def;

EXPORT	int	printgeom		__PR((SCSI *scgp, int current));
EXPORT	void	testformat		__PR((SCSI *scgp, struct disk *dp));
LOCAL	void	print_diskgeom		__PR((struct disk *dp));
EXPORT	int	Adaptec4000		__PR((SCSI *scgp));
LOCAL	void	get_Adaptec_defaults	__PR((SCSI *scgp, struct disk *dp));
EXPORT	int	Emulex_MD21		__PR((SCSI *scgp));
EXPORT	void	get_defaults		__PR((SCSI *scgp, struct disk *dp));
LOCAL	void	get_proto_defaults	__PR((SCSI *scgp, struct disk *dp, struct disk *xp));
LOCAL	void	get_Emulex_defaults	__PR((SCSI *scgp, struct disk *dp));
LOCAL	void	get_sony_defaults	__PR((SCSI *scgp, struct disk *dp));
LOCAL	void	get_general_defaults	__PR((struct disk *dp));
EXPORT	void	get_lgeom_defaults	__PR((SCSI *scgp, struct disk *dp));
LOCAL	void	get_missing_defaults	__PR((SCSI *scgp, struct disk *dp));
LOCAL	BOOL	confirmformat		__PR((void));
LOCAL	void	sigalrm			__PR((int sig));
LOCAL	int	prpercent		__PR((long ftim));
LOCAL	int	format_disk		__PR((SCSI *scgp, struct disk *dp, int clear_gdl));
EXPORT	int	reformat_disk		__PR((SCSI *scgp, struct disk *dp));
EXPORT	int	acb_format_disk		__PR((SCSI *scgp, struct disk *dp, int clear_gdl));
LOCAL	void	prconfig		__PR((SCSI *scgp, struct disk *dp, struct disk *xp));

EXPORT int
printgeom(scgp, current)
	SCSI	*scgp;
	int	current;
{
	u_char	mode[0x100];
	u_char	*p;
	u_char	*ep;

	scgp->silent++;
	getdev(scgp, FALSE);

	/* XXX Quick and dirty, musz verallgemeinert werden !!! */

	(void) unit_ready(scgp);
	scgp->silent--;

	fillbytes(mode, sizeof (mode), '\0');
	if (mode_sense(scgp, mode, 0xFF, 0x3F, current?0:2) < 0) { /* All Pages */
		if (mode_sense(scgp, mode, 0xFF, 0, 0) < 0) /* VU (block desc) */
			return (-1);
	}

	if (scgp->verbose)
		prbytes("Mode Sense Data", mode, 0xFF - scg_getresid(scgp));

	ep = mode+mode[0];	/* Points to last byte of data */
	p = &mode[4];
	p += mode[3];
	printf("Pages: ");
	while (p < ep) {
		printf("0x%x ", *p&0x3F);
		p += p[1]+2;
	}
	printf("\n");

	if (mode[3] == 8) {
		printf("Density: 0x%x\n", mode[4]);
		printf("Blocks:  %ld\n", a_to_u_3_byte(&mode[5]));
		printf("Blocklen:%ld\n", a_to_u_3_byte(&mode[9]));
	}
	p = &mode[4];
	p += mode[3];
	while (p < ep) {
		if ((*p&0x3F) == 0x03) {
			printf("Tr/zone: %d\n", a_to_u_2_byte(&p[2]));
			printf("As/zone: %d\n", a_to_u_2_byte(&p[4]));
			printf("At/zone: %d\n", a_to_u_2_byte(&p[6]));
			printf("At/vol:  %d\n", a_to_u_2_byte(&p[8]));
			printf("Nsect:   %d\n", a_to_u_2_byte(&p[10]));
			printf("Psecsize:%d\n", a_to_u_2_byte(&p[12]));
			printf("Tr skew: %d\n", a_to_u_2_byte(&p[16]));
			printf("Cy skew: %d\n", a_to_u_2_byte(&p[18]));
		}
		if ((*p&0x3F) == 0x04) {
			printf("Ncyl:    %ld\n", a_to_u_3_byte(&p[2]));
			printf("Nhead:   %d\n", p[5]);
		}
		p += p[1]+2;
	}
	return (0);
}

EXPORT void
testformat(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	scgp->silent++;
	if (read_capacity(scgp) >= 0) {
		dp->formatted =
				read_disk_label(scgp, (struct dk_label *)Sbuf, 0L);
		/*
		 * XXX check if we might get problems.
		 */
		if (dp->capacity != (scgp->cap->c_baddr + 1))
			printf("WARNING: cap: %ld baddr+1: %ld\n",
					dp->capacity, (long)scgp->cap->c_baddr + 1);

		dp->capacity = dp->cur_capacity = scgp->cap->c_baddr + 1;
	}
	scgp->silent--;
	printf("Disk %sformatted, %sLabel found.\n",
				dp->formatted < 0 ? "un":"",
				dp->formatted > 0 ? "":"no ");
	if (dp->formatted > 0) {
		printf("Current Label:     '%s'\n",
				getasciilabel((struct dk_label *)Sbuf));
	}
	if (autoformat)
		dp->formatted = -1;
}

LOCAL void
print_diskgeom(dp)
	register struct disk	*dp;
{
	printf("Sektorsize   : %ld Bytes\n", dp->secsize);
	printf("Sektors/Track: %ld alt %ld\n", dp->spt,
				dp->tpz <= 0 ? 0 : dp->aspz/dp->tpz);
	printf("Cylinders:     %ld alt %ld\n", dp->pcyl,
				dp->nhead <= 0 ? 0 : dp->atrk/dp->nhead);
	printf("Heads:         %ld\n", dp->nhead);
}

EXPORT int
Adaptec4000(scgp)
	SCSI	*scgp;
{
	struct disk	*dp = &cur_disk;

	testformat(scgp, dp);		/* test if disk is formatted */

	if (dp->formatted > 0)	/* Sonst sind die Werte sinnlos */
		get_acb4000defaults(scgp, dp);

	get_ext_diskdata(scgp, scgp->inq->inq_vendor_info, dp);

	if (!autoformat) {	/* XXX */
		read_primary_label(scgp, dp);
		if (dp->labelread <= 0)
			printf("No Label: Defaulting to Bull D585\n\n");
	} else {
		get_default_partition(scgp, dp);
	}

	if (!no_heuristic_defaults)
		get_Adaptec_defaults(scgp, dp);

	get_missing_defaults(scgp, dp);
	get_general_defaults(dp);

	if (dp->formatted > 0 && !yes("Ignore old defect List? "))
		read_def_blk(scgp);
	edit_def_blk();

	print_diskgeom(dp);

	if (!autoformat)
		select_parameters(scgp, dp);

	setlabel_from_val(scgp, dp, d_label);
	if (Prpart) {
		prconfig(scgp, dp, (struct disk *)0);
		/* NOTREACHED */
	}
	if (label)
		return (TRUE);

	if (!yes("Set mode pages (needed for format)? "))
		return (TRUE);

	set_acb4000params(scgp, dp);
	ext_modeselect(scgp, dp);

	if (setmodes)
		return (TRUE);

	estimate_times(dp);
	print_fmt_time(dp);

	if (!confirmformat())
		return (TRUE);

	prdate();
	getstarttime();

	if (unit_ready(scgp))
		return (acb_format_disk(scgp, dp, TRUE));
	return (FALSE);
}


/*---------------------------------------------------------------------------
|
|	Hier folgen default Werte fuer eine Bull D585 (Vertex V185)
|
+---------------------------------------------------------------------------*/

LOCAL void
get_Adaptec_defaults(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	if (dp->nhead < 0)
		dp->nhead	= 7;
	if (dp->pcyl < 0)
		dp->pcyl	= 1166;
	if (dp->tpz > 0)
		comerrno(EX_BAD, "Bad tpz value.\n");
	dp->tpz	= 0;
	if (dp->atrk > 0)
		comerrno(EX_BAD, "Bad atrk value.\n");
	dp->atrk	= 0;
	if (dp->lacyl < 0)
		dp->lacyl	= 3;
	dp->int_cyl = 0;
	if (dp->lncyl < 0)
		dp->lncyl	= dp->pcyl - dp->lacyl;
	if (dp->spt < 0) {
		if (scgp->dev == DEV_ACB4070)
			dp->spt	= 26;
		else
			dp->spt	= 17;
	}
	if (dp->aspz > 0)
		comerrno(EX_BAD, "Bad aspz value.\n");
	dp->aspz	= 0;
	if (dp->rpm < 0)
		dp->rpm	= 3600;

	if (dp->reduced_curr < 0)
		dp->reduced_curr = dp->pcyl;
	if (dp->write_precomp < 0)
		dp->write_precomp = dp->pcyl;
	if (dp->step_rate < 0)
		dp->step_rate = 2;

	if (dp->interleave < 0)
		dp->interleave = 1;
	if (dp->split_wv_cmd < 0)
		dp->split_wv_cmd = 0;
	if (dp->gap1 < 0)
		dp->gap1 = 0;
	if (dp->gap2 < 0)
		dp->gap2 = 0;
	get_lgeom_defaults(scgp, dp);
}

EXPORT int
Emulex_MD21(scgp)
	SCSI	*scgp;
{
	struct disk	*dp = &cur_disk;
	int	cdl = TRUE;

	if (debug)
		printf("int_cyl: %ld lncyl: %ld\n", dp->int_cyl,
			dp->lncyl);
	testformat(scgp, dp);		/* test if disk is formatted */

	if (Prpart)
		get_proto_defaults(scgp, dp, &alt_disk);
	else
		get_defaults(scgp, dp);
	if (debug)
		printf("int_cyl: %ld lncyl: %ld\n", dp->int_cyl,
			dp->lncyl);

	get_ext_diskdata(scgp, scgp->inq->inq_vendor_info, dp);
/*printf("XXX: disk_type: %s flags: %X\n", dp->disk_type, dp->flags);*/


	if (debug)
		printf("INQ_FOUND: %s (%lX)\n",
			(dp->flags&(D_INQ_FOUND|D_FIRMW_FOUND))?"TRUE":"FALSE",
			dp->flags);

	if (!Prpart && !dp->disk_type && (dp->flags & D_INQ_FOUND)) {
		int	odefmodes = defmodes;

		printf("WARNING: Inquiry for Controller found but no disk matches\n\n");
		printf("This may be because current changeable media in not known\n");
		printf("or because the disk has firmware bugs.\n\n");

		printf("Trying to match disk with current mode pages:\n");
		defmodes = FALSE;
		alt_disk.formatted = 1;
		get_defaults(scgp, &alt_disk);
		alt_disk.formatted = dp->formatted;
		defmodes = odefmodes;

		get_ext_diskdata(scgp, scgp->inq->inq_vendor_info, &alt_disk);
		alt_disk.capacity = dp->capacity;
		alt_disk.cur_capacity = dp->cur_capacity;
		if (alt_disk.disk_type)
			movebytes(&alt_disk, dp, sizeof (struct disk));
		else
			printf("No match found.\n");
	}
	if (debug)
		printf("int_cyl: %ld lncyl: %ld\n", dp->int_cyl,
			dp->lncyl);

	if (!autoformat) {	/* XXX */
		read_primary_label(scgp, dp);
		if (dp->labelread <= 0)
			printf("No Label: Defaulting to Drive values\n\n");
	if (debug)
		printf("int_cyl: %ld lncyl: %ld\n", dp->int_cyl,
			dp->lncyl);
	} else {
		get_default_partition(scgp, dp);
	}

	if (!no_heuristic_defaults)
		get_Emulex_defaults(scgp, dp);

	get_missing_defaults(scgp, dp);
	get_general_defaults(dp);

	if (debug)
		printf("int_cyl: %ld lncyl: %ld\n", dp->int_cyl,
			dp->lncyl);

	if (scgp->dev == DEV_SONY_SMO) {
		if (dp->interleave == 1)
			dp->interleave = 0;
	}

	print_diskgeom(dp);

	if (!autoformat)
		select_parameters(scgp, dp);

	setlabel_from_val(scgp, dp, d_label);
	if (debug && !Prpart)
		prpartab(stdout, "", d_label);
	if (Prpart) {
		if (defmodes)
			prconfig(scgp, dp, &alt_disk);
		else
			prconfig(scgp, &alt_disk, dp);
		/* NOTREACHED */
	}
	if (label)
		return (TRUE);

	if (!autoformat) {
		cdl = yes("Clear old grown defect list? ");
	}

	if (!yes("Set mode pages (needed for format)? "))
		return (TRUE);

	if (scgp->dev == DEV_SONY_SMO) {
		if (dp->fmt_mode == 3)
			save_mp = 0;
	}
	set_format_params(scgp, dp);
	set_geom(scgp, dp);
	set_error_rec_params(scgp, dp);
	set_disre_params(scgp, dp);
	if (scgp->dev == DEV_SONY_SMO)
		set_sony_params(scgp, dp);
	set_common_control(scgp, dp);
	ext_modeselect(scgp, dp);

	if (setmodes)
		return (TRUE);

	estimate_times(dp);
	print_fmt_time(dp);

	if (!confirmformat())
		return (TRUE);

	prdate();
	getstarttime();

	if (scgp->dev == DEV_SONY_SMO || unit_ready(scgp))
		return (format_disk(scgp, dp, cdl));
	return (FALSE);
}


EXPORT void
get_defaults(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	if (scgp->dev == DEV_SONY_SMO)
		get_sony_defaults(scgp, dp);
	else
		get_mode_defaults(scgp, dp);
}

LOCAL void
get_proto_defaults(scgp, dp, xp)
	SCSI		*scgp;
	struct disk	*dp;	/* ptr to return main disk data */
	struct disk	*xp;	/* ptr to return alt disk data */
{
	int	formatted = dp->formatted;
	int	odefmodes = defmodes;

	/*
	 * If defmodes is TRUE,
	 * 	'dp' points to default disk data and 'xp' points to current data
	 * else
	 * 	'xp' points to default disk data and 'dp' points to current data
	 */
	dp->formatted = 1;		/* use only defmodes in modes.c */
	get_defaults(scgp, dp);

	defmodes = !defmodes;
	dp->formatted = 1;		/* use only defmodes in modes.c */
	xp->formatted = 1;		/* use only defmodes in modes.c */
	get_defaults(scgp, xp);

	dp->formatted = formatted;
	xp->formatted = formatted;
	defmodes = odefmodes;

	get_lgeom_defaults(scgp, xp);
	if (xp->lncyl < 0)
		xp->lncyl = get_default_lncyl(scgp, xp);
	get_missing_defaults(scgp, xp);
	get_general_defaults(xp);
	if (xp->cur_capacity < 0)
		xp->cur_capacity = dp->cur_capacity;
}

/*---------------------------------------------------------------------------
|
|	Hier folgen ungefaehre default Werte fuer eine CCS SCSI Platte
|
+---------------------------------------------------------------------------*/

LOCAL void
get_Emulex_defaults(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	if (dp->tpz < 0)
		dp->tpz	= 1;
	if (dp->atrk < 0 && dp->nhead > 0)
		dp->atrk	= 2 * dp->nhead;
	if (strindex("EMULEX", scgp->inq->inq_vendor_info))
		dp->int_cyl = 3;
	else if (dp->int_cyl < 0)
		dp->int_cyl = 0;
	if (dp->aspz < 0)
		dp->aspz	= 1;
	if (dp->rpm < 0)
		dp->rpm	= 3600;
	if (dp->track_skew < 0)
		dp->track_skew = 0;
	if (dp->cyl_skew < 0)
		dp->cyl_skew = 0;
	if (dp->interleave < 0)
		dp->interleave = 1;	/* Interleaving Faktor */
	if (dp->gap1 < 0)
		dp->gap1 = 0;
	if (dp->gap2 < 0)
		dp->gap2 = 0;
	get_lgeom_defaults(scgp, dp);
}


/*---------------------------------------------------------------------------
|
|	Hier folgen default Werte fuer ein Sony SMO 501
|
+---------------------------------------------------------------------------*/

LOCAL void
get_sony_defaults(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	scgp->silent++;
	get_error_rec_defaults(scgp, dp);
	get_disre_defaults(scgp, dp);
	if (read_capacity(scgp) >= 0)
		dp->secsize = scgp->cap->c_bsize;
	get_sony_format_defaults(scgp, dp);
	scgp->silent--;
	if (no_heuristic_defaults)
		return;
	if (dp->nhead < 0)
		dp->nhead	= 1;
	if (dp->pcyl < 0)
		dp->pcyl	= 18852;
	if (dp->tpz < 0)
		dp->tpz	= 1;
	dp->int_cyl = 174;
	if (dp->spt < 0) {
		if (dp->secsize == 512)
			dp->spt	= 31;
		else if (dp->secsize == 1024)
			dp->spt = 17;
	}
	if (dp->aspz < 0)
		dp->aspz	= 0;
	if (dp->spare_band_size < 0)
		dp->spare_band_size = 2048;
	if (dp->atrk < 0) {
		dp->atrk	= dp->spare_band_size / dp->spt;
/*			if (dp->spare_band_size % dp->spt)*/
/*	???		dp->atrk++;*/
	}
	if (dp->rpm < 0)
		dp->rpm	= 2400;
	if (dp->track_skew < 0)
		dp->track_skew = 0;
	if (dp->cyl_skew < 0)
		dp->cyl_skew = 0;
	if (dp->interleave < 0)
		dp->interleave = 0;	/* Interleaving Faktor */
	if (dp->fmt_pattern < 0)
		dp->fmt_pattern = 3;	/* MkCDA | MkPlst */
	if (dp->fmt_mode < 0)
		dp->fmt_mode = 3;
	if (dp->def_lst_format < 0)
		dp->def_lst_format = SC_DEF_PHYS;
	if (dp->fmt_mode == 3)
		save_mp = 0;
	get_lgeom_defaults(scgp, dp);
	if (dp->lncyl < 0 && dp->pcyl > 0)	/*XXX hier ??*/
		dp->lncyl	= dp->pcyl
				- dp->atrk/dp->nhead
				- dp->lacyl - dp->int_cyl;
}

LOCAL void
get_general_defaults(dp)
	struct disk	*dp;
{
	if (dp->fmt_pattern < 0)
		dp->fmt_pattern = 0;

	if (dp->def_lst_format < 0)
		dp->def_lst_format = SC_DEF_BLOCK;
	if (dp->split_wv_cmd < 0)
		dp->split_wv_cmd = 0;

	if (!no_heuristic_defaults)
		estimate_times(dp);

	if (Cveri < 0)
		Cveri = dp->veri_count > 0 ? dp->veri_count : CVERI;
	if (CWveri < 0)
		CWveri = dp->wr_veri_count > 0 ?
					dp->wr_veri_count : CWVERI;
	if (Nveri < 0) {
		Nveri = dp->veri_loops > 0 ?
					dp->veri_loops : (wrveri?NWVERI:NVERI);
		if (wrveri)
			Nveri *= 2;
	}
}

EXPORT void
get_lgeom_defaults(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	if (dp->lhead < 0)
		dp->lhead = dp->nhead;
	if (dp->lspt < 0)
		dp->lspt = dp->spt;
	if (dp->lacyl < 0)
		dp->lacyl =
			(old_acb(scgp->dev) || (dp->nhead == 1)) ? 2L : 1L;
	if (dp->lpcyl < 0 ||
	    (dp->lpcyl != dp->pcyl && ((dp->flags & D_DISK_LPCYL) == 0))) {
		/*
		 * Let us assume the sector size is 512 bytes.
		 * Sun Disk Labels allow (cheated) disks up to ~ 100 PB.
		 * The current SCSI standard only allows disks up to 1 TB.
		 * For this reason we make the conversion simple.
		 */
		dp->lpcyl = dp->pcyl;
		while (dp->lpcyl > 0xFFFE) {
			dp->lhead *= 2;
			dp->lpcyl /= 2;
		}
		dp->flags |= D_DISK_LPCYL;
	}
}

LOCAL void
get_missing_defaults(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	if (dp->rpm < 0) {
		if (scgp->inq->data_format >= 2)
			printf("Disk states to be SCSI-2 but really is not.\n");
		printf("WARNING: disk rotation rate not known.\n");
		if (!yes("Don't use 3600 as default? "))
			dp->rpm = 3600;
	}
	if (!is_ccs(scgp->dev)) {
		if (dp->track_skew < 0)
			dp->track_skew = 0;
		if (dp->cyl_skew < 0)
			dp->cyl_skew = 0;
	} else {
#ifdef	needed
		if (dp->reduced_curr < 0)
			dp->reduced_curr = dp->pcyl;
		if (dp->write_precomp < 0)
			dp->write_precomp = dp->pcyl;
		if (dp->step_rate < 0)
			dp->step_rate = 0;
#endif
	}
}

LOCAL BOOL
confirmformat()
{
	if (noformat) {
		printf("WARNING: The -noformat option has been specified.\n");
		printf("	 This inhibits formatting the disk regardles of the next confirmation.\n");
	}
	if (format_confirmed)
		return (TRUE);

	if (!yes("Format Disk destroys all Data. Really? ")) {
		if (autoformat) {
			printf("Platte wurde  n i c h t  formatiert.\n");
			exit(EX_BAD);
		}
		return (FALSE);
	}
	format_confirmed = TRUE;
	return (TRUE);
}

LOCAL void
sigalrm(sig)
	int	sig;
{
}

LOCAL int
prpercent(ftim)
	long	ftim;
{
#ifdef	HAVE_FORK
	int	pid = fork();
	int	fatim;
	struct timeval	tv;

	if (pid != 0)
		return (pid);

	fatim = ftim / 100;
	if (fatim == 0)
		fatim = 1;
	for (;;) {
#ifdef	SIGALRM
		signal(SIGALRM, sigalrm);
		alarm(fatim);
		pause();
#endif
		if (gettimeofday(&tv, (struct timezone *)0) < 0)
			break;
		printf("%4lld%%\b\b\b\b\b",
			(Llong)(tv.tv_sec-starttime.tv_sec)*100/ftim);
		flush();
	}
	return (pid);
#else
	return (999999999);
#endif
}

LOCAL int
format_disk(scgp, dp, clear_gdl)
	SCSI		*scgp;
	struct disk	*dp;
	int	clear_gdl;
{
	BOOL	ret	= FALSE;
	sigset_t	oldmask;
	int	pid;

	if (!confirmformat())
		return (TRUE);

	format_done = FALSE;
	if (noformat) {
		printf("No Format!!\n");
		return (TRUE);
	}
	read_sinfo(scgp, dp, TRUE);
	printf("Formatting ... "); flush();
	pid = prpercent(dp->fmt_time);

	block_sigs(oldmask);
	if (format_unit(scgp, &fmt, 0, (int)dp->def_lst_format,
				disable_mdl, clear_gdl,
				(int)dp->interleave,
				(int)dp->fmt_pattern,
				(int)dp->fmt_timeout) >= 0) {
		format_done = TRUE;
		if (rezero_unit(scgp) >= 0)
			ret = TRUE;
	} else if (force) {
		if (yes("Continue? "))
			ret = TRUE;
	}
#ifdef	SIGALRM
	alarm(0);
#endif
	printf("done.\n");
	if (read_capacity(scgp) < 0)
		ret = FALSE;
	write_sinfo(scgp, dp);
	label_disk(scgp, dp);
	convert_def_blk(scgp);
	write_def_blk(scgp, TRUE);
	restore_sigs(oldmask);
#ifdef	HAVE_FORK
#ifdef	SIGKILL
	kill(pid, SIGKILL);
#endif
	while (wait(0) > 0)
		;
#endif
	return (ret);
}

EXPORT int
reformat_disk(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	BOOL	ret	= FALSE;
	sigset_t	oldmask;
	int	pid;

	if (!confirmformat())
		return (TRUE);

	format_done = FALSE;
	if (noformat) {
		printf("No Reformat!!\n");
		return (TRUE);
	}
	read_sinfo(scgp, dp, TRUE);
	printf("Reformatting ... "); flush();
	getstarttime();
	pid = prpercent(dp->fmt_time);

	block_sigs(oldmask);
	if (format_unit(scgp, &fmt, 0, (int)dp->def_lst_format,
				disable_mdl, FALSE,
				(int)dp->interleave,
				(int)dp->fmt_pattern,
				(int)dp->fmt_timeout) >= 0) {
		format_done = TRUE;
		if (rezero_unit(scgp) >= 0)
			ret = TRUE;
	} else if (force) {
		if (yes("Continue? "))
			ret = TRUE;
	}
#ifdef	SIGALRM
	alarm(0);
#endif
	printf("done.\n");
	if (read_capacity(scgp) < 0)
		ret = FALSE;
	write_sinfo(scgp, dp);
	label_disk(scgp, dp);
	convert_def_blk(scgp);
	write_def_blk(scgp, TRUE);
	restore_sigs(oldmask);
#ifdef	HAVE_FORK
#ifdef	SIGKILL
	kill(pid, SIGKILL);
#endif
	while (wait(0) > 0)
		;
#endif
	return (ret);
}

#ifdef	used
reformat_with_bad(dp)
	struct disk	*dp;
{
	BOOL	ret	= FALSE;
	sigset_t	oldmask;
	int	pid;

	if (!confirmformat())
		return (TRUE);

	format_done = FALSE;
	if (noformat) {
		printf("No Reformat!!\n");
		return (TRUE);
	}
	read_sinfo(dp, TRUE);
	printf("Reformatting ... "); flush();
	getstarttime();
	pid = prpercent(dp->fmt_time);

	block_sigs(oldmask);
	if (format_unit((struct scsi_format_data *)bad, nbad, SC_DEF_BLOCK,
				disable_mdl, TRUE,
				(int)dp->interleave,
				(int)dp->fmt_pattern,
				(int)dp->fmt_timeout) >= 0) {
		format_done = TRUE;
		if (rezero_unit() >= 0)
			ret = TRUE;
	} else if (force) {
		if (yes("Continue? "))
			ret = TRUE;
	}
	printf("done.\n");
	if (read_capacity() < 0)
		ret = FALSE;
	write_sinfo(dp);
	label_disk(dp);
	convert_def_blk();
	write_def_blk(TRUE);
	restore_sigs(oldmask);
#ifdef	HAVE_FORK
#ifdef	SIGKILL
	kill(pid, SIGKILL);
#endif
	while (wait(0) > 0)
		;
#endif
	return (ret);
}
#endif

EXPORT int
acb_format_disk(scgp, dp, clear_gdl)
	SCSI		*scgp;
	struct disk	*dp;
	int	clear_gdl;
{
	BOOL	ret	= FALSE;
	sigset_t	oldmask;
	int	pid;

	if (!confirmformat())
		return (TRUE);

	format_done = FALSE;
	if (noformat) {
		printf("No Format!!\n");
		return (TRUE);
	}
	read_sinfo(scgp, dp, TRUE);
	printf("Formatting ... "); flush();
	pid = prpercent(dp->fmt_time);

	block_sigs(oldmask);
	if (format_unit(scgp, (struct scsi_format_data *)&def,
			(int)a_to_u_2_byte(&def.d_size)/sizeof (def.d_def[0]),
			SC_DEF_BFI, disable_mdl, clear_gdl,
				(int)dp->interleave,
				(int)dp->fmt_pattern,
				(int)dp->fmt_timeout) >= 0) {
		format_done = TRUE;
		if (rezero_unit(scgp) >= 0)
			ret = TRUE;
	} else if (force) {
		if (yes("Continue? "))
			ret = TRUE;
	}
#ifdef	SIGALRM
	alarm(0);
#endif
	printf("done.\n");
	if (read_capacity(scgp) < 0)
		ret = FALSE;
	write_sinfo(scgp, dp);
	label_disk(scgp, dp);
	convert_def_blk(scgp);
	write_def_blk(scgp, TRUE);
	restore_sigs(oldmask);
#ifdef	HAVE_FORK
#ifdef	SIGKILL
	kill(pid, SIGKILL);
#endif
	while (wait(0) > 0)
		;
#endif
	return (ret);
}

LOCAL void
prconfig(scgp, dp, xp)
	SCSI		*scgp;
	struct disk	*dp;	/* ptr to default data */
	struct disk	*xp;	/* ptr to current data */
{
	FILE	*f;
	char	name[80];
	char	hname[257];
	char	dname[257];
	BOOL	fbug	= FALSE;
	int	cur_match = 0;
	int	tot_match = 0;

	if (dp && !dp->disk_type)
		dp->disk_type = permstring(getasciilabel(d_label));
	if (dp && !dp->default_part)
		dp->default_part = permstring(getasciilabel(d_label));

	if (xp && !xp->disk_type)
		xp->disk_type = permstring(getasciilabel(d_label));
	if (xp && !xp->default_part)
		xp->default_part = permstring(getasciilabel(d_label));

	if (xp) {
		if (!pdisk_eql(dp, xp)) {
			printf("\nWARNING: firmware bug detected:\n\n");
			printf("\tdefault: nhead: %ld pcyl: %ld\n",
							dp->nhead, dp->pcyl);
			printf("\tcurrent: nhead: %ld pcyl: %ld\n\n",
							xp->nhead, xp->pcyl);
			fbug = TRUE;
		}

		cur_match = cmp_disk(dp, dp);
		tot_match = cmp_disk(xp, dp);

/*		if (cur_match > tot_match)*/
		if (cur_match != tot_match) {
			printf("WARNING: %d current settings differ from default settings.\n\n", cur_match-tot_match);

			if (cur_match < tot_match)
				printf("cur_match: %d tot_match: %d\n",
							cur_match, tot_match);
			if (xdebug == 0) {
				printf("Mode setting differences:\n");
				printf("name: offset in structure:	current(1): default(2):\n");

				xdebug++;
				(void) cmp_disk(xp, dp);
				xdebug--;
			}
		}
	}

	printf("Enter filename for database prototype [proto.dat]: "); flush();
	(void) getline(name, sizeof (name));
	if (name[0] == '\0')
		strcpy(name, "proto.dat");
	if (streql(name, "-"))
		f = stdout;
	else if ((f = fileopen(name, "rwca")) == NULL)
		comerr("Cannot open '%s'.\n", name);

	gethostname(hname, sizeof (hname)-1);
	hname[sizeof (hname)-1] = '\0';
	getdomainname(dname, sizeof (dname)-1);
	dname[sizeof (dname)-1] = '\0';

	fprintf(f, "#\n# New disk/partition type %s\n# saved by %s@%s%s%s on %s#%s\n",
			dp->disk_type,
			getlogin(), hname, dname[0] == '.' ? "":".", dname,
			datestr(),
			fbug?"\n# Remember the Firmware Bug!!! (put correct values into database)\n#":"");

	if (!defmodes) {
		fprintf(f, "# Warning: entry not generated with default modes\n#\n");
		printf("\n*******************\n\n");
		printf("Warning the generated prototype file is not derived from default data.\n");
		printf("Use -Proto option to get correct values\n");
		printf("\n*******************\n\n");
	}

	if (xp && cur_match > tot_match) {
		fprintf(f, "# current data\n");
		prdisk(f, xp, scgp->inq);
		fprintf(f, "# default data\n");
	}
	prdisk(f, dp, scgp->inq);
	prpartab(f, dp->disk_type, d_label);
	checklabel(dp, d_label, 0);
	fclose(f);
	exit(0);
}
