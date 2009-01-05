/* @(#)xdisk.c	1.31 08/12/22 Copyright 1991-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)xdisk.c	1.31 08/12/22 Copyright 1991-2008 J. Schilling";
#endif
/*
 *	Routines to handle external disk definitions
 *
 *	Copyright (c) 1991-2008 J. Schilling
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
#include <schily/stdlib.h>
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/utypes.h>
#include <schily/schily.h>
#ifdef	HAVE_STDC_HEADERS
#	include <stddef.h>
#	define	soff(str, fld)	offsetof(struct str, fld)
#else
#	include <struct.h>
#	define	soff(str, fld)	fldoff(str, fld)
#endif
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "fmt.h"

#ifdef	FMT
extern	int	xdebug;
extern	int	debug;
extern	int	save_mp;
extern	int	autoformat;
#else
	int	xdebug = 1;
#endif

struct  dsk_fnd {
	struct disk	disk;
	int		match;
	char		name[512];
};

LOCAL	struct dsk_fnd *expand_df	__PR((struct dsk_fnd *, int));
LOCAL	BOOL	fuzzy_capacity	__PR((struct disk *, struct dsk_fnd *));
EXPORT	BOOL	pdisk_eql	__PR((struct disk *dp1, struct disk *dp2));
LOCAL	BOOL	add_disk	__PR((char *, struct disk *, struct dsk_fnd *));
LOCAL	void	check_deflt	__PR((struct dsk_fnd *, int *, int));
LOCAL	void	use_disk	__PR((struct disk *, struct dsk_fnd *, int));
LOCAL	void	select_disk	__PR((SCSI *scgp, struct disk *, struct dsk_fnd *, int, int));
EXPORT	BOOL	get_ext_diskdata __PR((SCSI *scgp, char *name, struct disk *dp));
LOCAL	int	do_disk		__PR((char *, struct disk *, BOOL));
LOCAL	void	set_diskitem	__PR((struct disk *, char *, char *, char *));
LOCAL	BOOL	set_diskvar	__PR((char *, struct disk *));
LOCAL	void	copy_disk	__PR((struct disk *, struct disk *));
EXPORT	int	cmp_disk	__PR((struct disk *dp1, struct disk *dp2));
LOCAL	BOOL	disk_eql	__PR((struct disk *, struct disk *));
EXPORT	BOOL	has_error_rec_params	__PR((struct disk *dp));
EXPORT	BOOL	has_disre_params __PR((struct disk *dp));
LOCAL	void	copy_vendor_info __PR((char *, struct scsi_inquiry *));
EXPORT	void	prdisk		__PR((FILE *f, struct disk *dp, struct scsi_inquiry *ip));
EXPORT	int	need_params		__PR((struct disk *dp, int type));
EXPORT	int	need_label_params	__PR((struct disk *dp));
EXPORT	int	need_geom_params	__PR((struct disk *dp));
EXPORT	int	need_scsi_params	__PR((struct disk *dp));
EXPORT	int	need_error_rec_params	__PR((struct disk *dp));
EXPORT	int	need_disre_params	__PR((struct disk *dp));


#define	K_NONE	0x01	/* This entry is not used but might be in database */
#define	K_NDB	0x02	/* This entry must not be in the database */
#define	K_LABEL	0x04	/* This is a label - not a disk - parameter */
#define	K_IGN	0x08	/* Inore this entry when comparing parameters */
#define	K_COPY	0x10	/* Copy this parameter even if dest is initialized */
#define	K_INIT	0x20	/* Copy over only to non initialized parameters */
#define	K_BOOL	0x40	/* This is a boolean entry */

#define	K_GEOM	0x0100	/* This is a disk geometry parameter */
#define	K_SCSI	0x0200	/* This is a SCSI parameter */
#define	K_ERREC	0x0400	/* This is an error recovery parameter */
#define	K_DISRE	0x0800	/* This is a disconnect/reconnect parameter */

#define	K_N	0x1000	/* We don't need to fill this out */

#define	lfld(ptr, off)		((long *)(((char *)(ptr)) + (off)))

struct keyw {
	char	*name;
	int	flags;
	int	off;
} keyw[] = {

/* Disk Label geometry */
{ "lhead",		K_INIT|K_LABEL,		soff(disk, lhead) },
{ "lacyl",		K_INIT|K_LABEL,		soff(disk, lacyl) },
{ "lpcyl",		K_INIT|K_LABEL,		soff(disk, lpcyl) },
{ "lncyl",		K_INIT|K_LABEL,		soff(disk, lncyl) },
{ "lspt",		K_INIT|K_LABEL,		soff(disk, lspt) },
{ "lapc",		K_INIT|K_LABEL|K_N,	soff(disk, lapc) },

/* Disk Geometry */
{ "nhead",		K_INIT|K_GEOM,		soff(disk, nhead) },
{ "pcyl",		K_INIT|K_GEOM,		soff(disk, pcyl) },
{ "atrk",		K_INIT|K_GEOM,		soff(disk, atrk) },
{ "tpz",		K_INIT|K_GEOM,		soff(disk, tpz) },
{ "spt",		K_INIT|K_GEOM,		soff(disk, spt) },
{ "mintpz",		K_INIT|K_GEOM|K_N|K_NDB, soff(disk, mintpz) },
{ "maxtpz",		K_INIT|K_GEOM|K_N|K_NDB, soff(disk, maxtpz) },
{ "aspz",		K_INIT|K_GEOM,		soff(disk, aspz) },
{ "secsize",		K_INIT|K_GEOM,		soff(disk, secsize) },
{ "phys_secsize",	K_INIT|K_GEOM|K_N,	soff(disk, phys_secsize) },
			/*
			 * ignore capacity on compares
			 * use fuzzy compare instead
			 */
{ "capacity",		K_INIT|K_N|K_IGN,	soff(disk, capacity) },
{ "min_capacity",	K_NONE|K_N,		soff(disk, dummy) },
{ "rpm",		K_INIT|K_GEOM,		soff(disk, rpm) },
{ "track_skew",		K_INIT|K_GEOM,		soff(disk, track_skew) },
{ "cyl_skew",		K_INIT|K_GEOM,		soff(disk, cyl_skew) },

{ "reduced_curr",	K_INIT|K_GEOM|K_N,	soff(disk, reduced_curr) },
{ "write_precomp",	K_INIT|K_GEOM|K_N,	soff(disk, write_precomp) },
{ "step_rate",		K_INIT|K_GEOM|K_N,	soff(disk, step_rate) },

{ "rot_pos_locking",	K_INIT|K_GEOM|K_N,	soff(disk, rot_pos_locking) },
{ "rotational_off",	K_INIT|K_GEOM|K_N,	soff(disk, rotational_off) },

{ "interleave",		K_INIT|K_GEOM,		soff(disk, interleave) },
{ "fmt_pattern",	K_INIT|K_GEOM,		soff(disk, fmt_pattern) },

{ "fmt_mode",		K_INIT|K_GEOM|K_N,	soff(disk, fmt_mode) },
{ "spare_band_size",	K_INIT|K_GEOM|K_N,	soff(disk, spare_band_size) },

/* SCSI parameters */
{ "def_lst_format",	K_INIT|K_SCSI,		soff(disk, def_lst_format) },
{ "split_wv_cmd",	K_INIT|K_SCSI|K_BOOL,	soff(disk, split_wv_cmd) },

/* Error recovery */
{ "rd_retry_count",	K_INIT|K_ERREC|K_N,	soff(disk, rd_retry_count) },
{ "wr_retry_count",	K_INIT|K_ERREC|K_N,	soff(disk, wr_retry_count) },
{ "recov_timelim",	K_INIT|K_ERREC|K_N,	soff(disk, recov_timelim) },

/* Disconnect / reconnect */
{ "buf_full_ratio",	K_INIT|K_DISRE|K_N,	soff(disk, buf_full_ratio) },
{ "buf_empt_ratio",	K_INIT|K_DISRE|K_N,	soff(disk, buf_empt_ratio) },
{ "bus_inact_limit",	K_INIT|K_DISRE|K_N,	soff(disk, bus_inact_limit) },
{ "disc_time_limit",	K_INIT|K_DISRE|K_N,	soff(disk, disc_time_limit) },
{ "conn_time_limit",	K_INIT|K_DISRE|K_N,	soff(disk, conn_time_limit) },
{ "max_burst_size",	K_INIT|K_DISRE|K_N,	soff(disk, max_burst_size) },
{ "data_tr_dis_ctl",	K_INIT|K_DISRE|K_N,	soff(disk, data_tr_dis_ctl) },

/* Common device control */
{ "queue_alg_mod",	K_INIT|K_SCSI|K_N,	soff(disk, queue_alg_mod) },
{ "dis_queuing",	K_INIT|K_SCSI|K_BOOL|K_N,
						soff(disk, dis_queuing) },

{ "gap1",		K_INIT|K_IGN,		soff(disk, gap1) },
{ "gap2",		K_INIT|K_IGN,		soff(disk, gap2) },
{ "int_cyl",		K_INIT|K_IGN|K_N,	soff(disk, int_cyl) },
{ "fmt_time",		K_INIT|K_IGN,		soff(disk, fmt_time) },
{ "fmt_timeout",	K_INIT|K_IGN|K_N,	soff(disk, fmt_timeout) },
{ "veri_time",		K_INIT|K_IGN|K_N,	soff(disk, veri_time) },
{ "veri_count",		K_INIT|K_IGN|K_N,	soff(disk, veri_count) },
{ "wr_veri_count",	K_INIT|K_IGN|K_N,	soff(disk, wr_veri_count) },
{ "veri_loops",		K_INIT|K_IGN|K_N,	soff(disk, veri_loops) },
{ "default_data",	K_INIT|K_IGN|K_N|K_BOOL, soff(disk, default_data) },
{ "bridge_controller",	K_INIT|K_SCSI|K_IGN|K_N|K_BOOL,
						soff(disk, bridge_controller) },

};

#define	NKEYW	(sizeof (keyw) / sizeof (keyw[0]))

struct keyw	*keywNKEYW = &keyw[NKEYW];

#ifdef	FMT
#else
main(ac, av)
	int	ac;
	char	**av;
{
	char	*name;
	struct disk	test_disk;

	if (ac > 1)
		name = av[1];
	else {
		name = "QUANTUM P105S 910-10-94xA.1 ";
		test_disk.nhead = 6;
		test_disk.pcyl = 1019;
	}

	if (!opendatfile("sformat.dat"))
		return;

	get_ext_diskdata(name, &test_disk);

	closedatfile();
}
#endif

LOCAL struct dsk_fnd *
expand_df(df, found)
	struct dsk_fnd	*df;
	int		found;
{
	struct dsk_fnd	*xdf;

	xdf = (struct  dsk_fnd *) realloc(df,
					sizeof (struct  dsk_fnd) * (found+1));
	disk_null(&xdf[found].disk, 1);
	return (xdf);
}

LOCAL BOOL
fuzzy_capacity(dp, df)
	struct disk	*dp;
	struct dsk_fnd	*df;
{
	double	dc;	/* current disk */
	double	fc;	/* found disk from database */

	/*
	 * if capacity is not known this is a match
	 */
	if (dp->capacity < 0 || df->disk.capacity < 0)
		return (TRUE);

	if (dp->secsize < 0)
		dc = dp->capacity * 512.0;
	else
		dc = dp->capacity * (dp->secsize * 1.0);
	if (df->disk.secsize < 0) {
		if (dp->secsize > 0)
			fc = df->disk.capacity * (dp->secsize * 1.0);
		else
			fc = df->disk.capacity * 512.0;
	} else
		fc = df->disk.capacity * (df->disk.secsize * 1.0);

	if (debug || xdebug) {
		printf("fuzzy_cap current: %g found: %g (limits: %g, %g)\n",
						dc, fc, dc/1.3, dc*1.3);
	}

	/*
	 * XXX NOTE:	Some versions of Sunpro C compile 1.3 t0 1.0 if
	 * XXX		in german locale.
	 * XXX		If fuzzy capacity does not work as expected:
	 * XXX		check your C-compiler.
	 */
	if ((dc * 1.3) < fc || (dc / 1.3) > fc)
		return (FALSE);

	if (xdebug) printf("fuzzy_cap: TRUE\n");
	return (TRUE);
}

EXPORT BOOL
pdisk_eql(dp1, dp2)
	struct disk	*dp1;
	struct disk	*dp2;
{
	return ((dp1->nhead <= 0 || dp1->nhead == dp2->nhead) &&
			(dp1->pcyl <= 0 || dp1->pcyl == dp2->pcyl));
}

LOCAL BOOL
add_disk(name, dp, df)
	char		*name;
	struct disk	*dp;
	struct dsk_fnd	*df;
{
	if (pdisk_eql(dp, &df->disk) && fuzzy_capacity(dp, df)) {
		strcpy(df->name, name);
		return (TRUE);
	}
	return (FALSE);
}

LOCAL void
check_deflt(df, deflt, found)
	struct dsk_fnd	*df;
	int		*deflt;
	int		found;
{
	if (df->disk.default_data > 0) {
		if (*deflt >= 0) {
			if (!autoformat) {
				datfileerr("second default_data for '%s'",
					df->name);
			} else {
				comerrno(EX_BAD,
					"Die Datenbasis besitzt mehrere default Diskparameter fuer diese Platte.\n");
			/* NOTREACHED */
			}
		} else {
			*deflt = found;
		}
	}
}

LOCAL void
use_disk(dp, df, this)
	struct disk	*dp;
	struct dsk_fnd	*df;
	int		this;
{
	copy_disk(&df[this].disk, dp);
	if (dp->disk_type)
		free(dp->disk_type);
	dp->disk_type = permstring(df[this].name);
}

#define	item_equal(a, b)	(((a) <  0 && (b) <  0) || ((a) == (b)))

LOCAL void
select_disk(scgp, dp, df, found, deflt)
	SCSI		*scgp;
	struct disk	*dp;
	struct dsk_fnd	*df;
	int		found;
	int		deflt;
{
	int	i;
	int	this = -1;
	int	match = -1;
	int	max = -1;
	int	ndbentry = found;
	BOOL	selected = FALSE;
	BOOL	bridge	 = FALSE;

	for (i = 0; i < found; i++) {
		df[i].match = cmp_disk(dp, &df[i].disk);
		if (df[i].match > max &&
			item_equal(dp->secsize, df[i].disk.secsize) &&
			/*
			 * Sony Format Mode
			 */
			item_equal(dp->fmt_mode, df[i].disk.fmt_mode)) {
			max = df[i].match;
			match = i;
		}
		if (disk_eql(dp, &df[i].disk))
			this = i;
	}
	if (this < 0) {
		/*
		 * This disk is not known in database.
		 * Create an entry for it (name == INQUIRY).
		 */
		this = i;
		disk_null(&df[this].disk, 0);
		copy_disk(dp, &df[this].disk);
		df[this].disk.flags |= D_DISK_CURRENT;
		df[this].match = cmp_disk(dp, &df[this].disk);
		copy_vendor_info(df[this].name, scgp->inq);
		found++;
	}
	/*
	 * If the disk is unformatted or has no label
	 * use default database entry au current selection.
	 * Otherwise use current disk settings.
	 */
	if (dp->formatted <= 0)
		this = (deflt < 0) ? this : deflt;

	for (i = 0; i < ndbentry; i++) {
		if (df[0].disk.bridge_controller > 0)
			bridge = TRUE;
	}

	max = 0;
	if (autoformat) {
		if (deflt < 0) {
			/* Paranoia */
			comerrno(EX_BAD,
				"Die Datenbasis besitzt keine default Diskparameter fuer diese Platte.\n");
			/* NOTREACHED */
		}
		use_disk(dp, df, this = deflt);
	} else do {
		if (xdebug) printf("found: %d deflt: %d this: %d match: %d\n",
						found, deflt, this, match);

		if ((deflt >= 0) && (deflt != this)) {
			printf("WARNING: disk settings differ from default\n");
			selected = TRUE;
		}
		if (found > ndbentry) {
			printf("WARNING: disk settings differ from all database entries\n");
			if (bridge) {
				printf("\tThis may because an unknown disk is connected to the controller\n");
				printf("\tor because the current disk is not formatted as noted in database.\n");
				printf("\tIf this is an unknown disk, use the current parameters ...\n");
				printf("\totherwise:\n");
			}
			printf("\tCheck if you really want the disk settings to be\n");
			printf("\tdifferent from the settings in the database entry.\n");
			selected = TRUE;
		}
		if ((found > 1 && deflt < 0) ||
		    (found > 1 && (selected ||
				yes("Select alternate disk type? ")))) {
			printf("Available disk types:\n");
			for (i = 0; i < found; i++) {
				printf("%s%2d)%s%s(%2d)\t\"%s\"\n",
							i == this ? "*":" ",
							i,
							i == deflt ? "+":" ",
							i == match ? "~":" ",
							df[i].match,
							df[i].name);
			}
			if (max++ == 0 && ndbentry == 1 && found > 1 &&
					!yes("Don't use database entry or view selection? ")) {
				this = 0;	/* The one and only database entry */
				selected = FALSE;
			} else {
				getint("Select disk", &this, 0, found - 1);
				selected = TRUE;
			}
		}
		use_disk(dp, df, this);

		if (found > 1 && selected)
			prdisk(stdout, dp, scgp->inq);

	} while (selected && found > 1 && !yes("Use this disk parameters? "));

	if (deflt < 0)
		deflt = 0;
	if (this >= ndbentry) {
		printf("WARNING: using non database entry\n");
		printf("If you deny the next question there will be no label from data base.\n");
		if (yes("Use label list from default entry? ")) {
			dp->disk_type = permstring(df[deflt].name);
			dp->default_part = df[deflt].disk.default_part;
		}
		if (dp->mode_pages == 0 && df[deflt].disk.mode_pages) {
			printf("If you deny the next question there will be no mode pages from data base.\n");
			if (yes("Use mode page list from default entry? "))
				dp->mode_pages = df[deflt].disk.mode_pages;
		}
	}
}

EXPORT BOOL
get_ext_diskdata(scgp, name, dp)
	SCSI		*scgp;
	char		*name;
	struct disk	*dp;
{
	int	found = 0;
	int	inq_found = 0;		/* Vendor & Product */
	int	firmw_found = 0;	/* Vendor & Product & Firmware */
	BOOL	ign_firmw = FALSE;
	int	deflt = -1;
	char	nbuf[512];
	struct  dsk_fnd *disks_found;

again:
	if (rewinddatfile() < 0)
		return (FALSE);

	if (xdebug)
		printf("scan for disk: %.28s\n", name);

	disks_found = (struct  dsk_fnd *) malloc(sizeof (struct  dsk_fnd));
	disk_null(&disks_found[found].disk, 1);

	/*
	 * scan for next disk entry
	 */
	while (scanfortable("disk_type", NULL)) {
		int	i;
		strcpy(nbuf, curword());
		/*
		 * parse current disk entry and tell if it matched
		 */
		switch (i = do_disk(name, &disks_found[found].disk, ign_firmw)) {

		case D_INQ_FOUND|D_FIRMW_FOUND:
			firmw_found++;
			disks_found[found].disk.flags |=
						D_INQ_FOUND|D_FIRMW_FOUND;
			if (add_disk(nbuf, dp, &disks_found[found])) {
				check_deflt(&disks_found[found], &deflt, found);
				disks_found[found].disk.flags |=
						D_DISK_FOUND;
				found++;
				disks_found = expand_df(disks_found, found);
			}
			break;
		case D_INQ_FOUND:
			inq_found++;
			/*
			 * Don't increment found here,
			 * we must not use this entry.
			 */
			break;
		default:
			printf("illegal return code %X from do_disk()\n", i);
		case 0:	break;
		}
	}
	if (found) {
		/*
		 * Wenn nur ein Eintrag vorhanden ist, dann ist er
		 * in jedem Fall der Default Eintrag.
		 */
		if (found == 1)
			deflt = 0;
		printf("Default Disk type: '%s'\n",
				deflt < 0 ? "none" : disks_found[deflt].name);
		if (deflt < 0 && autoformat) {
			comerrno(EX_BAD,
				"Die Datenbasis besitzt keine default Diskparameter fuer diese Platte.\n");
			/* NOTREACHED */
		}
		if (deflt >= 0 && disks_found[deflt].disk.default_part) {
			printf("Default partition: '%s'\n",
					disks_found[deflt].disk.default_part);
		} else if (autoformat) {
			comerrno(EX_BAD,
				"Die Datenbasis besitzt keine default Partition fuer diese Platte.\n");
			/* NOTREACHED */
		}
		if (deflt >= 0 && disks_found[deflt].disk.mode_pages) {
			printf("Mode Pages:        '%s'\n",
					disks_found[deflt].disk.mode_pages);
		}
		if (autoformat ||
			!yes("Ignore database disk parameters from '%s'? ",
							datfilename())) {
			select_disk(scgp, dp, disks_found, found, deflt);
			free(disks_found);
			return (TRUE);
		}
	} else if (autoformat) {
		/* EMPTY */ ;		/* Checked below */
	} else if (firmw_found) {
		errmsgno(EX_BAD,
			"WARNING: Inquiry OK (%d controller matches) but no disk found.\n", firmw_found);
	} else if (inq_found) {
		errmsgno(EX_BAD,
			"WARNING: Inquiry OK (%d controller matches) but no matching firmware found.\n", inq_found);
		if (yes("Try to use entry for other firmware? ")) {
			ign_firmw = TRUE;
			goto again;
		} else {
			/*
			 * Report that we found vendor & product.
			 */
			dp->flags |= D_INQ_FOUND;
		}
	}
	if (autoformat) {
		comerrno(EX_BAD,
			"Diese Platte besitzt keine Produktfreigabe.\n");
		/* NOTREACHED */
	}
	free(disks_found);
	return (FALSE);
}


/*---------------------------------------------------------------------------
|
|	Parst einen Disk Eintrag in der Steuerungsdatei
|	Returnwert zeigt an, ob dieser Eintrag passend war.
|
+---------------------------------------------------------------------------*/

LOCAL int
do_disk(name, dp, ign_firmw)
	char		*name;
	struct disk	*dp;
	BOOL		ign_firmw;
{
	char	inqname[80];
	char	defpart[80];
	char	mpages[80];
	int	ret = 0;

	disk_null(dp, 0);
	if (xdebug) printf("do_disk: line: %d curword: '%s'\n",
						getlineno(), curword());
	(void) garbage(skipwhite(peekword()));
	if (!nextline())
		return (FALSE);

	while (scanforline(NULL, NULL) != NULL) {
		if ((ret & D_FIRMW_FOUND) == 0)
			inqname[0] = defpart[0] = mpages[0] = 0;

		set_diskitem(dp, inqname, defpart, mpages);
		if ((ret & D_FIRMW_FOUND) == 0 && *inqname) {
			if (xdebug) {
				printf("name: %.28s\n", name);
				printf("DATA: %.28s\n", inqname);
			}
			/*
			 * allow short inquiry entry in database
			 * to match i.e. all firmware
			 */
			if (strncmp(name, inqname, strlen(inqname)) == 0)
				ret = D_INQ_FOUND|D_FIRMW_FOUND;
			else if (strncmp(name, inqname, 24) == 0) {
				ret = D_INQ_FOUND;
				if (ign_firmw)
					ret |= D_FIRMW_FOUND;
			}
		}
	}
	if (past_df_sig())
		dp->flags = D_DB_BAD;
	if (dp->fmt_time > 0)
		dp->flags |= D_FTIME_FOUND;
	if (dp->veri_time > 0)
		dp->flags |= D_VTIME_FOUND;

	if (ret & D_FIRMW_FOUND) {
		if (xdebug)
			printf("defpart: '%s' mpages: '%s'\n", defpart, mpages);
		if (defpart[0])
			dp->default_part = permstring(defpart);
		if (mpages[0])
			dp->mode_pages   = permstring(mpages);
	}
	return (ret);
}

LOCAL void
set_diskitem(dp, inqname, defpart, mpages)
	struct disk	*dp;
	char		*inqname;
	char		*defpart;
	char		*mpages;
{
	char	*word;

	for (word = curword(); *word; word = nextword()) {
		if (streql(word, ":"))
			continue;
		if (streql(word, "inquiry")) {
			if (!set_stringvar("Inquiry Name", inqname, 28))
				break;
		} else if (streql(word, "default_partition")) {
			if (!set_stringvar("Partition Name", defpart, 79))
				break;
		} else if (streql(word, "mode_pages")) {
			if (!set_stringvar("Modepage Name", mpages, 79))
				break;
		} else if (!set_diskvar(word, dp))
			break;
	}
	(void) nextword();
}

LOCAL BOOL
set_diskvar(word, dp)
	char		*word;
	struct disk	*dp;
{
	register struct keyw *kp = keyw;

	long	*valp = (long *)0;
	long	l;

	if (xdebug) printf("diskvar: '%s' : ", word);

	for (; kp < keywNKEYW; kp++) {
		if ((kp->name != NULL) &&
				((kp->flags & (K_LABEL|K_NDB)) == 0) &&
						streql(word, kp->name)) {
			valp = lfld(dp, kp->off);
			break;
		}
	}

	if (kp >= keywNKEYW) {
		skip_illvar("disk", word);
		return (FALSE);
	}

	if (!checkequal())
		return (FALSE);

	if (!isval(word = nextword()))
		return (FALSE);
	if (kp->flags & K_BOOL) {
		if (streql(word, "TRUE")) {
			l = TRUE;
		} else if (streql(word, "FALSE")) {
			l = FALSE;
		} else {
			datfileerr("%s: not a bool '%s'", kp->name, word);
			return (FALSE);
		}
	} else if (*astol(word, &l) != '\0') {
		datfileerr("%s: not a number '%s'", kp->name, word);
		return (FALSE);
	}
	*valp = l;

	if (xdebug) printf("%ld\n", *valp);
	return (TRUE);
}

LOCAL void
copy_disk(from, to)
	register struct disk *from;
	register struct disk *to;
{
	register struct keyw *kp = keyw;

	for (; kp < keywNKEYW; kp++) {
		if (kp->flags & K_COPY)
			*lfld(to, kp->off) = *lfld(from, kp->off);
		else if (kp->flags & K_INIT) {
			if (*lfld(from, kp->off) >= 0)
				*lfld(to, kp->off) = *lfld(from, kp->off);
		}
	}

	to->flags |= from->flags;

	if (from->disk_type) {
		to->disk_type = from->disk_type;
	}
	if (from->default_part) {
		if (to->default_part)
			free(to->default_part);
		to->default_part = from->default_part;
	}
	if (from->mode_pages) {
		if (to->mode_pages)
			free(to->mode_pages);
		to->mode_pages = from->mode_pages;
	}
	if (from->parts) {
		/*XXX*/
		to->parts = from->parts;
	}
	if (from->bridge_controller > 0) {
		to->bridge_controller = TRUE;
	}
}

EXPORT int
cmp_disk(dp1, dp2)
	register struct disk *dp1;
	register struct disk *dp2;
{
	register struct keyw *kp = keyw;
	register int	n = 0;

	for (; kp < keywNKEYW; kp++) {
		if ((kp->flags & (K_NONE|K_IGN)) == 0 &&
				*lfld(dp1, kp->off) >= 0 &&
				*lfld(dp1, kp->off) == *lfld(dp2, kp->off)) {
			n++;
		} else {
			if (xdebug == 0)
				continue;

			if (*lfld(dp1, kp->off) == -1)
				continue;
			printf("name:%-16s off:%3d flags:%02X 1:%9ld 2:%9ld\n",
				kp->name, kp->off, kp->flags,
				*lfld(dp1, kp->off), *lfld(dp2, kp->off));
		}
	}
	return (n);
}

LOCAL BOOL
disk_eql(dp1, dp2)
	register struct disk *dp1;
	register struct disk *dp2;
{
	register struct keyw *kp = keyw;
	register int	diffs = 0;

	/*
	 * Zwei Platten sind gleich, wenn alle in beiden Strukturen definierten
	 * d.h. != -1 Felder gleich sind.
	 */
	for (; kp < keywNKEYW; kp++) {
		if ((kp->flags & (K_NONE|K_LABEL|K_IGN)) == 0 &&
				*lfld(dp1, kp->off) >= 0 &&
				*lfld(dp2, kp->off) >= 0 &&
				*lfld(dp1, kp->off) != *lfld(dp2, kp->off)) {
			if (xdebug)
				printf("not equal: %-10s %ld != %ld\n",
				kp->name,
				*lfld(dp1, kp->off), *lfld(dp2, kp->off));
			diffs++;
		}
	}
	if (xdebug)
		printf("total of %d diff%s.\n", diffs, diffs > 1?"s":"");
	return (diffs == 0);
}

EXPORT BOOL
has_error_rec_params(dp)
	register struct disk	*dp;
{
	if (dp->rd_retry_count >= 0 ||
			dp->wr_retry_count >= 0 ||
			dp->recov_timelim >= 0) {
		return (TRUE);
	}
	return (FALSE);
}

EXPORT BOOL
has_disre_params(dp)
	register struct disk	*dp;
{
	if (dp->buf_full_ratio >= 0 ||
			dp->buf_empt_ratio >= 0 ||
			dp->bus_inact_limit >= 0 ||
			dp->disc_time_limit >= 0 ||
			dp->conn_time_limit >= 0 ||
			dp->max_burst_size >= 0 ||
			dp->data_tr_dis_ctl >= 0) {
		return (TRUE);
	}
	return (FALSE);
}


LOCAL void
copy_vendor_info(s, ip)
	char			*s;
	struct scsi_inquiry	*ip;
{
	sprintf(s, "CURRENT %.28s", ip->vendor_info);
}

EXPORT void
prdisk(f, dp, ip)
	register FILE *f;
	register struct disk		*dp;
	register struct scsi_inquiry	*ip;
{
	int	neednl = 0;

	fprintf(f, "disk_type = \"%s\"\n", dp->disk_type);
	fprintf(f, "\tinquiry = \"%.28s\" :\n", ip->vendor_info);
	if (dp->bridge_controller > 0)
		fprintf(f, "\tbridge_controller = TRUE :\n");
#ifdef	nono
	if (dp->fmt_time <= 0 || dp->veri_time <= 0)
		estimate_times(dp);
#endif
	if (dp->fmt_time > 0) {
		fprintf(f, "\tfmt_time = %ld :", dp->fmt_time);
		neednl++;
	}

	if (dp->veri_time > 0) {
		fprintf(f, "%sveri_time = %ld :",
					neednl?" ":"\t", dp->veri_time);
		neednl++;
	}
	if (neednl)
		fprintf(f, "\n");
	neednl = 0;

	fprintf(f, "\tnhead = %ld : ", dp->nhead);
	fprintf(f, "pcyl = %ld : ", dp->pcyl);
	fprintf(f, "spt = %ld : ", dp->spt);
	fprintf(f, "secsize = %ld :\n", dp->secsize);

	if (dp->reduced_curr >= 0 ||
			dp->write_precomp >= 0 || dp->step_rate >= 0) {
		fprintf(f, "\treduced_curr = %ld : ", dp->reduced_curr);
		fprintf(f, "write_precomp = %ld : ", dp->write_precomp);
		fprintf(f, "step_rate = %ld :\n", dp->step_rate);
	}
	if (dp->atrk > 0 || dp->tpz != 1 || dp->aspz > 0) {
		fprintf(f, "\tatrk = %ld : ", dp->atrk);
		fprintf(f, "tpz = %ld : ", dp->tpz);
		fprintf(f, "aspz = %ld : ", dp->aspz);
		neednl++;
	}
	if (dp->phys_secsize > 0 && dp->phys_secsize != dp->secsize) {
		fprintf(f, "phys_secsize = %ld :", dp->phys_secsize);
		neednl++;
	}
	if (neednl)
		fprintf(f, "\n");
	if (dp->cur_capacity > 0) {
		fprintf(f, "\tcapacity = %ld :\n", dp->cur_capacity);
	}
	if (dp->track_skew > 0 || dp->cyl_skew > 0) {
		fprintf(f, "\ttrack_skew = %ld : ", dp->track_skew);
		fprintf(f, "cyl_skew = %ld :\n", dp->cyl_skew);
	}
	if (dp->fmt_mode >= 0 || dp->spare_band_size >= 0) {
		fprintf(f, "\tfmt_mode = %ld : ", dp->fmt_mode);
		fprintf(f, "spare_band_size = %ld :\n", dp->spare_band_size);
	}
	if (dp->def_lst_format >= 0 && dp->def_lst_format != SC_DEF_BLOCK) {
		fprintf(f, "\tdef_lst_format = %ld :\n", dp->def_lst_format);
	}
	if (dp->split_wv_cmd > 0) {
		fprintf(f, "\tsplit_wv_cmd = %s :\n",
					dp->split_wv_cmd?"TRUE":"FALSE");
	}
	neednl = 0;
	if (dp->queue_alg_mod >= 0) {
		fprintf(f, "\tqueue_alg_mod = %ld :", dp->queue_alg_mod);
		neednl++;
	}
	if (dp->dis_queuing > 0) {
		fprintf(f, "%sdis_queuing = TRUE :", neednl?" ":"\t");
		neednl++;
	}
	if (neednl)
		fprintf(f, "\n");

	fprintf(f, "\tinterleave = %ld : ", dp->interleave);
	fprintf(f, "rpm = %ld :", dp->rpm);
	if (dp->fmt_pattern > 0)
		fprintf(f, " fmt_pattern = %ld :", dp->fmt_pattern);
	fprintf(f, "\n");

	if (has_error_rec_params(dp)) {
		int	n = 0;

		fprintf(f, "\t");
		if (dp->rd_retry_count >= 0) {
			fprintf(f, "rd_retry_count = %ld :",
						dp->rd_retry_count);
			n++;
		}
		if (dp->wr_retry_count >= 0) {
			fprintf(f, "%swr_retry_count = %ld :", n ? " " : "",
						dp->wr_retry_count);
			n++;
		}
		if (dp->recov_timelim >= 0) {
			fprintf(f, "%srecov_timelim = %ld :", n ? " " : "",
						dp->recov_timelim);
			n++;
		}
		fprintf(f, "\n");
	}
	if (has_disre_params(dp)) {
		int	n = -1;

		fprintf(f, "\t");
		if (dp->buf_full_ratio >= 0) {
			n++;
			fprintf(f, "buf_full_ratio = %ld :",
						dp->buf_full_ratio);
		}
		if (dp->buf_empt_ratio >= 0) {
			n++;
			fprintf(f, "%sbuf_empt_ratio = %ld :", n ? " " : "",
						dp->buf_empt_ratio);
		}
		if (dp->bus_inact_limit >= 0) {
			if (++n >= 3) { fprintf(f, "\n\t"); n = 0; }

			fprintf(f, "%sbus_inact_limit = %ld :", n ? " " : "",
						dp->bus_inact_limit);
		}
		if (dp->disc_time_limit >= 0) {
			if (++n >= 3) { fprintf(f, "\n\t"); n = 0; }

			fprintf(f, "%sdisc_time_limit = %ld :", n ? " " : "",
						dp->disc_time_limit);
		}
		if (dp->conn_time_limit >= 0) {
			if (++n >= 3) { fprintf(f, "\n\t"); n = 0; }

			fprintf(f, "%sconn_time_limit = %ld :", n ? " " : "",
						dp->conn_time_limit);
		}
		if (dp->max_burst_size >= 0) {
			if (++n >= 3) { fprintf(f, "\n\t"); n = 0; }

			fprintf(f, "%smax_burst_size = %ld :", n ? " " : "",
						dp->max_burst_size);
		}
		if (dp->data_tr_dis_ctl >= 0) {
			if (++n >= 3) { fprintf(f, "\n\t"); n = 0; }

			fprintf(f, "%sdata_tr_dis_ctl = %ld :", n ? " " : "",
						dp->data_tr_dis_ctl);
		}
		fprintf(f, "\n");
	}

	fprintf(f, "\tdefault_partition = \"%s\" :\n\n", dp->default_part);
}

EXPORT	int
need_params(dp, type)
	register struct disk	*dp;
	register int		type;
{
	register struct keyw *kp = keyw;
	register int	n = 0;

	for (; kp < keywNKEYW; kp++) {
		if ((kp->flags & K_N) != 0)
			continue;
		if ((kp->flags & type) == 0)
			continue;

		if (*lfld(dp, kp->off) != -1)
			continue;
		n++;
/*		if (xdebug == 0)*/
/*			continue;*/

		printf("name:%-16s off:%3d flags:%02X == -1\n",
				kp->name, kp->off, kp->flags);
	}
	return (n);
}

EXPORT	int
need_label_params(dp)
	struct disk	*dp;
{
	return (need_params(dp, K_LABEL));
}

EXPORT	int
need_geom_params(dp)
	struct disk	*dp;
{
	return (need_params(dp, K_GEOM));
}

EXPORT	int
need_scsi_params(dp)
	struct disk	*dp;
{
	return (need_params(dp, K_SCSI));
}

/* NOT NEEDED */
EXPORT	int
need_error_rec_params(dp)
	struct disk	*dp;
{
	return (need_params(dp, K_ERREC));
}

/* NOT NEEDED */
EXPORT	int
need_disre_params(dp)
	struct disk	*dp;
{
	return (need_params(dp, K_DISRE));
}
