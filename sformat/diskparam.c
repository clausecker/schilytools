/* @(#)diskparam.c	1.13 09/07/11 Copyright 1988-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)diskparam.c	1.13 09/07/11 Copyright 1988-2009 J. Schilling";
#endif
/*
 *	Query Disk parameters
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
#include <schily/standard.h>
#include <schily/schily.h>

#include <scg/scsidefs.h>
#include <scg/scsitransp.h>

#include "fmt.h"

extern	char	*Sbuf;
extern	long	Sbufsize;
extern	int	debug;

extern	int	Proto;
extern	int	wrveri;
extern	int	Nveri;
extern	int	Cveri;
extern	int	CWveri;
extern	BOOL	datfile_chk;

EXPORT	void	select_parameters	__PR((SCSI *scgp, struct disk *dp));
LOCAL	void	select_scsi_params	__PR((struct disk *dp));
LOCAL	void	select_fmt_time		__PR((struct disk *dp));
LOCAL	void	select_disk_geom	__PR((SCSI *scgp, struct disk *dp));
LOCAL	void	select_error_rec_params	__PR((struct disk *dp));
LOCAL	void	select_disre_params	__PR((struct disk *dp));
LOCAL	BOOL	need_acb_spec		__PR((struct disk *dp));

EXPORT void
select_parameters(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	BOOL	xtd = FALSE;
	BOOL	x2;

	if (!datfile_chk)
		printf("WARNING: Disk database '%s' contains uncertified data.\n",
							datfilename());
	if (dp->flags & D_DB_BAD) {
		printf("WARNING: Disk found in local extensions of database '%s',\n",
							datfilename());
		printf("\t entry for disk is not certified.\n");
	}

	for (;;) {
		if (debug)
			printf("geom: %d\n", need_geom_params(dp));
		x2 = need_geom_params(dp) || (old_acb(scgp->dev) && need_acb_spec(dp));

		if (x2 || xtd ||
				yes("Modify Disk Geometry Parameters? "))
			select_disk_geom(scgp, dp);
		get_lgeom_defaults(scgp, dp);

		if (debug)
			printf("error_rec: %d\n", need_error_rec_params(dp));
		x2 = need_error_rec_params(dp);
		if (has_error_rec_params(dp) &&
				(x2 || xtd ||
				yes("Modify Error recovery Parameters? ")))
			select_error_rec_params(dp);

		if (debug)
			printf("disre: %d\n", need_disre_params(dp));
		x2 = need_disre_params(dp);
		if (has_disre_params(dp) &&
				(x2 || xtd ||
				yes("Modify Disconnect Parameters? ")))
			select_disre_params(dp);

		/*
		 * Es gibt z.Zt. keine Moeglichkeit
		 * zu erkennen, ob das Selektieren eines alternativen Labels
		 * moeglich ist, denn die Variable parts in 'dp' wird
		 * noch nicht benutzt.
		 */
		if (dp->labelread <= 0 || yes("Select alternate Label? "))
			select_partition(scgp, dp);

		if (debug)
			printf("label: %d\n", need_label_params(dp));
		x2 = need_label_params(dp);
		if (x2 || dp->labelread < 0 ||
				yes("Modify Label Geometry Parameters? "))
			select_label_geom(scgp, dp);

		if (dp->labelread > 0 || !select_backup_label(scgp, dp, TRUE))
			break;
		dp->labelread = 1;
	}
	/*
	 * Hier werden keine Geometrieabhaengigen Werte modifiziert,
	 * daher musz die Abfrage nicht innerhalb der Schleife stehen.
	 */
	if (debug)
		printf("scsi: %d\n", need_scsi_params(dp));
	x2 = need_scsi_params(dp) || Proto;
	if (x2 || xtd || yes("Modify SCSI Parameters? ")) {
		select_scsi_params(dp);
	}
	if (dp->fmt_time <= 0 || (dp->flags & D_FTIME_FOUND) == 0)
		select_fmt_time(dp);
}

LOCAL void
select_scsi_params(dp)
	struct disk	*dp;
{
	if (dp->dis_queuing >= 0)
		getlong("Disable command queuing (CMDQ enabled= 0 CMDQ disabled= 1)",
					&dp->dis_queuing, 0L, 0x01L);
	if (dp->dis_queuing <= 0 && dp->queue_alg_mod >= 0)
		getlong("Enter queuing algorithm (Restr. reord= 0 Unrestr. reord= 1)",
					&dp->queue_alg_mod, 0L, 0x0FL);

	getlong("Enter FORMAT defect list format (BLOCK= 0 BFI= 4 PHYS= 5)",
					&dp->def_lst_format, 0L, 0x07L);
	dp->split_wv_cmd = yes("Split WRITE-VERIFY-Command? ");
	if (wrveri == 0)
		wrveri = yes("Do write_verify? ");
	getint("Enter number of verify loops ", &Nveri, 0, 1000);
	getint("Enter number of blocks / verify command ", &Cveri, 1, 10000);
	if (wrveri != 0) {
		getint("Enter number of blocks / write_verify command ",
			&CWveri, 1, (int)(Sbufsize/dp->secsize));
	}
	if (dp->bridge_controller < 0)
		dp->bridge_controller = yes("Is the disk connected to a bridge controller (e.g. SCSI/ESDI)? ");

	select_fmt_time(dp);
}

LOCAL void
select_fmt_time(dp)
	struct disk	*dp;
{
	if (dp->fmt_time < 0 || (dp->flags & D_FTIME_FOUND) == 0) {
#ifdef	nonono
		long	fmt_time    = dp->fmt_time;
		long	fmt_timeout = dp->fmt_timeout;
		long	veri_time   = dp->veri_time;
#endif

		printf("WARNING: disk formatting time not known.\n");
		estimate_times(dp);
		print_fmt_time(dp);
#ifdef	nonono
		printf("If this time is wrong formatting may be aborted.\n");
		if (yes("Don't use estimated default? ")) {
			dp->fmt_time    = fmt_time;
			dp->fmt_timeout = fmt_timeout;
			dp->veri_time   = veri_time;
		}
#endif
	}
	printf("WARNING: if format time is wrong formatting may be aborted.\n");
	if (getlong("Enter format time in secs ", &dp->fmt_time, 0, 10000)) {
		dp->flags |= D_FTIME_FOUND;
		dp->fmt_timeout = -1;
	}
	estimate_times(dp);
	print_fmt_timeout(dp);
}

LOCAL void
select_disk_geom(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	BOOL	is_acbdev = old_acb(scgp->dev);
	long	maxhead = 255L;
	long	minintlv = 0L;

	if (is_acbdev) {
		maxhead = 16L;
		/*
		 * Wichtig minimal 2 alternate Spuren, weil Platte kleiner als
		 * erwartet wird, denn es gibt keine Reservesektoren und
		 * keine Reservespuren.
		 */
		minintlv = 1L;
	}

	getlong("Enter number of heads", &dp->nhead, 1L, maxhead);
	getlong("Enter number of physical cylinders",
						&dp->pcyl, 1L, 0xFFFFFFL);
	if (is_acbdev) {
		do {
			getlong("Enter Sectorsize",
					&dp->secsize, 256L, 1024L);
		} while (!(dp->secsize != 256 ||
			dp->secsize != 512 || dp->secsize != 1024));
	} else {
		long	old_secsize = dp->secsize;

		getlong("Enter Sectorsize", &dp->secsize,
					(long)MIN_SECSIZE, (long)MAX_SECSIZE);
		/*
		 * XXX I hope that this is ok and physical secsize is
		 * XXX normally not needed.
		 */
		if (dp->phys_secsize > 0) {
			if (old_secsize == dp->phys_secsize)
				dp->phys_secsize = dp->secsize;
			else
				getlong("Enter physical Sectorsize",
					&dp->phys_secsize,
					(long)MIN_SECSIZE, (long)MAX_SECSIZE);
		}
	}
	getlong("Enter number of physical sectors/track (Controller)",
						&dp->spt, 1L, 512L);

	if (is_acbdev) {
/*XXX		dp->reduced_curr = dp->write_precomp = dp->pcyl;*/
		getlong("Enter reduced write current cylinder",
				&dp->reduced_curr, 1L, dp->pcyl);
		getlong("Enter write precomp cylinder",
				&dp->write_precomp, 1L, dp->pcyl);
		getlong("Enter step time (0 = 3ms, 1 = 28 us, 2 = 12 us)",
						&dp->step_rate, 0L, 2L);
	} else {
		long	maxaspz;

		/* if controller does not support format parameter page ... */
		if (dp->mintpz < 0)
			dp->mintpz = 0;
		if (dp->maxtpz < 0)
			dp->maxtpz = 0xFFFF;
		getlong("Enter number of tracks/zone (Controller)",
			&dp->tpz, dp->mintpz, dp->maxtpz);

		maxaspz = dp->tpz ? dp->spt*dp->tpz : 0xFFFFL;
		if (maxaspz > 0xFFFFL)
			maxaspz = 0xFFFFL;
		getlong("Enter number of alternate sectors/zone (Controller)",
			&dp->aspz, 0L, maxaspz);

		getlong("Enter number of alternate tracks/volume (Controller)",
						&dp->atrk, 0L, 8192L);

		if (dp->fmt_mode >= 0) {	/* It's a Sony */
			getlong("Enter format mode (0 = xdebug, 1 = debug, 2 = sony, 3 = iso)",
						&dp->fmt_mode, 0L, 0x3L);
			getlong("Enter Spare Band Size",
					&dp->spare_band_size, 0L, 8192L);
			dp->atrk = dp->spare_band_size/dp->spt;
		}

		getlong("Enter track skew", &dp->track_skew, 0L, dp->spt);
		getlong("Enter cylinder skew", &dp->cyl_skew, 0L, dp->spt);

		getlong("Enter rot pos locking (0 = off, 1 = slave, 2 = master, 3 = master ctl)", &dp->rot_pos_locking, 0L, 3L);
		if (dp->rot_pos_locking > 0)
			getlong("Enter rotational offset", &dp->rotational_off, 0L, 255L);
		else
			dp->rotational_off = 0;
	}
	getlong("Enter rpm of drive", &dp->rpm, 0L, 20000L);
	getlong("Enter interleave factor", &dp->interleave, minintlv, dp->spt);
	getlong("Enter format pattern", &dp->fmt_pattern, 0L, 0xFFL);
}

LOCAL void
select_error_rec_params(dp)
	struct disk	*dp;
{
	if (dp->rd_retry_count >= 0)
		getlong("Enter Read  retry count",
				&dp->rd_retry_count, 0L, 0xFFL);

	if (dp->wr_retry_count >= 0)
		getlong("Enter Write retry count",
				&dp->wr_retry_count, 0L, 0xFFL);

	if (dp->recov_timelim >= 0)
		getlong("Enter Error recovery limit (1 unit = 1 msec)",
				&dp->recov_timelim, 0L, 0xFFFFL);

}

LOCAL void
select_disre_params(dp)
	struct disk	*dp;
{
	if (dp->buf_full_ratio >= 0)
		getlong("Enter Buffer full  ratio",
				&dp->buf_full_ratio, 0L, 0xFFL);

	if (dp->buf_empt_ratio >= 0)
		getlong("Enter Buffer empty ratio",
				&dp->buf_empt_ratio, 0L, 0xFFL);

	if (dp->bus_inact_limit >= 0)
		getlong("Enter Bus inactivity  limit (1 unit = 100 usec)",
				&dp->bus_inact_limit, 0L, 0xFFFFL);

	if (dp->disc_time_limit >= 0)
		getlong("Enter Disconnect time limit (1 unit = 100 usec)",
				&dp->disc_time_limit, 0L, 0xFFFFL);

	if (dp->conn_time_limit >= 0)
		getlong("Enter Connect    time limit (1 unit = 100 usec)",
				&dp->conn_time_limit, 0L, 0xFFFFL);

	if (dp->max_burst_size >= 0)
		getlong("Enter max Burst size (1 unit = 512 Bytes)",
				&dp->max_burst_size, 0L, 0xFFFFL);

	if (dp->data_tr_dis_ctl >= 0)
		getlong("Enter Data transfer disconnect control",
				&dp->data_tr_dis_ctl, 0L, 0x3L);
}

LOCAL BOOL
need_acb_spec(dp)
	struct disk	*dp;
{
	if (dp->reduced_curr < 0 || dp->write_precomp < 0 ||
						dp->step_rate < 0)
		return (TRUE);
	return (FALSE);
}
