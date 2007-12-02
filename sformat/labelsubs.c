/* @(#)labelsubs.c	1.23 07/02/22 Copyright 1988-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)labelsubs.c	1.23 07/02/22 Copyright 1988-2004 J. Schilling";
#endif
/*
 *	Subroutines that deal with the primary disk label
 *
 *	Copyright (c) 1988-2004 J. Schilling
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
#include <sys/param.h>	/* XXX nonportable to use u_char */
#endif
#include <stdio.h>
#include <schily/standard.h>
#include <signal.h>
#include <schily/stdlib.h>
#include <schily/errno.h>
#include <schily/schily.h>

#include "dsklabel.h"

#include <scg/scsireg.h>
#include <scg/scsidefs.h>
#include <scg/scsitransp.h>

#include "scsicmds.h"

#include "fmt.h"

extern	int	debug;
extern	int	nowait;

extern	int	autoformat;
extern	int	reformat_only;
extern	int	label;

union x_label {
	struct	dk_label label;
	char		 space[MAX_SECSIZE];
} x_label;
struct	dk_label *d_label = (struct  dk_label *)&x_label;

extern	char		*Lname;

EXPORT	void	read_primary_label	__PR((SCSI *scgp, struct disk *dp));
EXPORT	void	create_label		__PR((SCSI *scgp, struct disk *dp));
LOCAL	void	set_driver_geom		__PR((SCSI *scgp));
EXPORT	void	label_disk		__PR((SCSI *scgp, struct disk *dp));
LOCAL	BOOL	read_backup_label	__PR((SCSI *scgp, struct disk *dp, struct dk_label *lp));
LOCAL	void	labelintr		__PR((int sig));
LOCAL	BOOL	scan_backup_label	__PR((SCSI *scgp, struct dk_label *lp, long *start, BOOL lgeom_ok));
EXPORT	int	read_disk_label		__PR((SCSI *scgp, struct dk_label *lp, long secno));
LOCAL	void	print_label_err		__PR((struct dk_label *lp));
LOCAL	BOOL	has_space_for_acyl	__PR((SCSI *scgp, struct disk *dp, long *lblk));
LOCAL	BOOL	has_unused_space	__PR((SCSI *scgp, struct disk *dp, long *lblk));
EXPORT	long	get_default_lncyl	__PR((SCSI *scgp, struct disk *dp));
EXPORT	void	select_label_geom	__PR((SCSI *scgp, struct disk *dp));
EXPORT	BOOL	select_backup_label	__PR((SCSI *scgp, struct disk *dp, BOOL lgeom_ok));
LOCAL	BOOL	get_defpart		__PR((SCSI *scgp, struct disk *dp, struct dk_label *lp));
EXPORT	void	select_partition	__PR((SCSI *scgp, struct disk *dp));
EXPORT	void	get_default_partition	__PR((SCSI *scgp, struct disk *dp));

EXPORT void
read_primary_label(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	if (dp->formatted > 0 && !yes("Ignore old disk Label? ")) {
		/*
		 * Disk is formatted,
		 * Label magic and Label checksum OK, use it!
		 */
		if (read_disk_label(scgp, d_label, 0L) < 0) {
			error("Could not read label from disk\n");

			if (select_backup_label(scgp, dp, FALSE) == TRUE)
				dp->labelread = 1;

		} else if (setval_from_label(dp, d_label)) {
			dp->labelread = 1;
			printf("<%s>\n", d_label->dkl_asciilabel);

		} else {
			print_label_err(d_label);
		}
	} else if (dp->formatted == 0) {
		/*
		 * Disk is formatted,
		 * Corrupt Label.
		 */
		if (select_backup_label(scgp, dp, FALSE) == TRUE)
			dp->labelread = 1;
	}
}

EXPORT void
create_label(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
extern	int	format_done;
	BOOL	geom_changed	= FALSE;
	BOOL	write_label	= format_done && !nowait;

	if (autoformat) {
		label_disk(scgp, dp);
		return;
	}
	scgp->silent++;
	read_capacity(scgp);
	scgp->silent--;
	if ((scgp->cap->c_baddr + 1) != dp->cur_capacity) {
		printf("WARNING:\n");
		printf("Format changed Disk Geometry.\n");
		printf("Capacity before: %ld Capacity after: %ld\n",
				dp->cur_capacity,
				(long)(scgp->cap->c_baddr + 1));

		dp->cur_capacity = scgp->cap->c_baddr + 1;
		select_label_geom(scgp, dp);
		geom_changed = TRUE;
		write_label = TRUE;
	}
	if (debug)
		printf("nowait: %d write_label: %d geom_changed: %d\n",
					nowait, write_label, geom_changed);
	/*
	 * ALT: !nowait
	 * NEU:
	 * Wenn nowait, Label nicht schreiben es sei denn geometrie geändert.
	 * Wenn nicht formatiert, fragen ob Label schreiben
	 * Wenn formatiert, fragen ob Label ändern
	 * (impliziert geändertes Label schreiben)
	 * Wenn formatiert & geometrie geändert, dann immber Label ändern.
	 */
	if ((write_label || !format_done) &&
					yes("Print disk label? ")) {
		printlabel(d_label);
		checklabel(dp, d_label, 0);
	}
	if ((write_label || !format_done) &&
			(geom_changed || yes("Modify disk label? "))) do {
		write_label = TRUE;

		makelabel(scgp, dp, d_label);
		printlabel(d_label);
		checklabel(dp, d_label, 1);
	} while (!yes("Use this label? "));

	if (debug)
		printf("nowait: %d write_label: %d geom_changed: %d\n",
					nowait, write_label, geom_changed);
	/*
	 * fill in default vtmap if not done yet
	 */
	check_vtmap(d_label, label || write_label);
	if (label) {
		writelabel(Lname, d_label);
		/* NOTREACHED */
	} else if (write_label) {
		if (!format_done && !yes("Write label on disk? "))
			exit(1);
		label_disk(scgp, dp);
	}
}

LOCAL void
set_driver_geom(scgp)
	SCSI	*scgp;
{
	int	diskno = maptodisk(scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp));
	char	*dname;
	struct dk_label xl;

	if (diskno < 0) {
			errmsgno(EX_BAD,
			"Cannot set geometry to driver (cannot map disk name).\n");
		return;
	}
	dname = diskdevname(diskno);

	if (!readlabel(dname, &xl)) {
		if (geterrno() == ENXIO) {
			errmsgno(EX_BAD, "Cannot verify mapping.\n");
			if (yes("Set geometry to driver? "))
				(void) setlabel(dname, d_label);
		}
		return;
	}

	if (cmpbytes(d_label, &xl, sizeof (xl)) < sizeof (xl)) {
		errmsgno(EX_BAD,
			"Will not set geometry to driver (mapping error).\n");
		return;
	}
	(void) setlabel(dname, d_label);
}

/*---------------------------------------------------------------------------
|
|	Die Backuplabels stehen auf dem letzten Reservezylinder
|	auf dem letzten Kopf.
|
+---------------------------------------------------------------------------*/
EXPORT void
label_disk(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	long	backup_label_blk;
	int	i;

	backup_label_blk = d_label->dkl_ncyl + d_label->dkl_acyl - 1;
	backup_label_blk *= d_label->dkl_nhead * d_label->dkl_nsect;
	backup_label_blk += (d_label->dkl_nhead-1) * d_label->dkl_nsect;

	if (old_acb(scgp->dev)) {
		backup_label_blk = d_label->dkl_pcyl;
		backup_label_blk *= d_label->dkl_nhead * d_label->dkl_nsect;
		backup_label_blk -= (d_label->dkl_nhead-1) * d_label->dkl_nsect;
	}

	if (write_scsi(scgp, (caddr_t)d_label, 0L, 1) < 0)
		error("Could not write label to disk\n");
	if (dp->lacyl > 0) {
		for (i = 1; i < 10; i += 2) {
			if (write_scsi(scgp, (caddr_t)d_label, backup_label_blk+i, 1) < 0)
				error("Could not write backup label to disk\n");
		}
	}
	set_driver_geom(scgp);
}

/*---------------------------------------------------------------------------
|
|	Die Backuplabels stehen auf dem letzten Alternativen Zylinder
|	auf dem letzten Kopf auf den ersten 5 ungeraden Sektoren.
|	Bei Adaptec 4000 geht das nicht, weil i.a. die Kapazitaet nicht
|	ausreicht. Daher:
|	Alt: Letzter Alternativen Zylinder Kopf 1
|	Neu: Letzter Alternativen Zylinder Kopf 2
|
+---------------------------------------------------------------------------*/
LOCAL BOOL
read_backup_label(scgp, dp, lp)
	SCSI		*scgp;
	struct disk	*dp;
	struct dk_label *lp;
{
	long	backup_label_blk;
	int	i;
	int	ret;

	backup_label_blk = d_label->dkl_ncyl + d_label->dkl_acyl - 1;
	backup_label_blk *= d_label->dkl_nhead * d_label->dkl_nsect;
	backup_label_blk += (d_label->dkl_nhead-1) * d_label->dkl_nsect;
printf("lab: %ld\n", backup_label_blk+1);
	backup_label_blk = dp->lncyl + dp->lacyl - 1;
	backup_label_blk *= dp->lhead * dp->lspt;
	backup_label_blk += (dp->lhead-1) * dp->lspt;
printf("lab: %ld\n", backup_label_blk+1);

	if (old_acb(scgp->dev)) {
		if (dp->lpcyl > 0)
			backup_label_blk = dp->lpcyl;
		else
			backup_label_blk = dp->pcyl;
		backup_label_blk *= dp->lhead * dp->lspt;
		backup_label_blk -= (dp->lhead-1) * dp->lspt;
	}

	if (dp->lacyl > 0) {
		for (i = 1; i < 10; i += 2) {
			if ((ret = read_disk_label(scgp, lp, backup_label_blk+i)) < 0)
				error("Could not read backup label from disk\n");
			if (ret == TRUE)
				return (TRUE);
			error("Backup label: ");
			print_label_err(lp);
		}
		return (FALSE);
	}
	error("No alt Cylinders!\n");
	return (FALSE);
}

LOCAL	int	scan_intr;

LOCAL void
labelintr(sig)
	int	sig;
{
	scan_intr++;
}

LOCAL BOOL
scan_backup_label(scgp, lp, start, lgeom_ok)
	SCSI	*scgp;
	struct dk_label *lp;
	long	*start;
	BOOL	lgeom_ok;
{
	int	i;
	int	limit;
	long	l;
	void	(*osig) __PR((int));

	scgp->silent++;
	if (read_capacity(scgp) < 0) {
		scgp->silent--;
		return (FALSE);
	}
	scgp->silent--;
	if (start && *start)
		l = *start;
	else
		l = scgp->cap->c_baddr;

	osig = signal(SIGINT, labelintr);

	printf("Scanning for backup label..."); flush();

	limit = scgp->cap->c_baddr/200;
	if (!lgeom_ok)
		limit *= 10;
	scan_intr = 0;
	for (i = 0; scan_intr == 0 && i < limit; i++, l--) {
		if (read_disk_label(scgp, lp, l) == TRUE) {
			printf("\nScan for backup label: found at: %ld n: %d\n",
								l, i);
			signal(SIGINT, osig);
			if (start)
				*start = --l;
			return (TRUE);
		}
	}
	signal(SIGINT, osig);

	printf("\nScan for backup label: NOT found : %ld n: %d\n", l, i);
	return (FALSE);
}

EXPORT int
read_disk_label(scgp, lp, secno)
	SCSI	*scgp;
	struct dk_label *lp;
	long	secno;
{
	fillbytes((caddr_t)lp, sizeof (*lp), '\0');
	if (read_scsi(scgp, (caddr_t)lp, secno, 1) < 0)
		return (-1);
	return (lp->dkl_magic == DKL_MAGIC && lp->dkl_cksum == do_cksum(lp));
}

LOCAL void
print_label_err(lp)
	struct dk_label *lp;
{
/*	if (lp->dkl_magic == 0 || do_cksum(lp) == 0)*/
/*		error("Could not read label.\n");*/
	if (lp->dkl_magic != DKL_MAGIC)
		error("No label found.\n");
	if (lp->dkl_cksum != do_cksum(lp))
		error("Corrupt label.\n");
}

LOCAL BOOL
has_space_for_acyl(scgp, dp, lblk)
	SCSI	*scgp;
	struct disk	*dp;
	long	*lblk;
{
	*lblk = dp->lncyl + dp->lacyl - 1;
	*lblk *= dp->lhead * dp->lspt;
	*lblk += (dp->lhead - 1) * dp->lspt;
	*lblk += 9;	/* XXX letztes backuplabel */
	if (*lblk > scgp->cap->c_baddr) {
		*lblk -= scgp->cap->c_baddr;
		*lblk += dp->lhead * dp->lspt;
		*lblk /= dp->lhead * dp->lspt;
		printf("WARNING: Alternate cylinders exceed end of disk. (%ld cyls)\n", *lblk);
		return (FALSE);
	}
	return (TRUE);
}

LOCAL BOOL
has_unused_space(scgp, dp, lblk)
	SCSI	*scgp;
	struct disk	*dp;
	long	*lblk;
{
	/*
	 * Wenn aus Sicht der Labelgeometrie mindestens ein ganzer
	 * Zylinder oder mehr verschenkt wird, wird gewarnt.
	 */
	*lblk = scgp->cap->c_baddr + 1;
	*lblk -= (dp->lncyl+dp->lacyl) * dp->lhead*dp->lspt;

	if (*lblk >= dp->lhead * dp->lspt) {
		*lblk /= dp->lhead * dp->lspt;
		printf("WARNING: Unused space on disk. (%ld cyls)\n", *lblk);
		printf("\tThis may be correct on a generic label that fits for a group of disks.\n");
		printf("\tIn this case the disk space must match the smallest disk of that group.\n");
		return (TRUE);
	}
	return (FALSE);
}

EXPORT long
get_default_lncyl(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	long	lncyl;

	if (dp->lpcyl < 0 || dp->lpcyl != dp->pcyl) {
		printf("NOTICE: setting lpcyl:\n");
		printf("get_default_lncyl lpcyl: %ld lhead: %ld lspt: %ld\n", dp->lpcyl, dp->lhead, dp->lspt);
		get_lgeom_defaults(scgp, dp);
		printf("get_default_lncyl lpcyl: %ld lhead: %ld lspt: %ld\n", dp->lpcyl, dp->lhead, dp->lspt);
	}
	if (scgp->cap->c_baddr <= 0) {
		lncyl = dp->lpcyl-dp->lacyl;
	} else if (dp->lhead > 0 && dp->lspt > 0 && dp->lacyl > 0) {
		lncyl = scgp->cap->c_baddr+1;
		lncyl /= dp->lhead*dp->lspt;
		lncyl -= dp->lacyl;
	} else {
		lncyl = -1;
	}
	return (lncyl);
}

EXPORT void
select_label_geom(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	long	minlacyl = old_acb(scgp->dev) ? 2L : 1L;
	long	maxlncyl;
	long	lncyl;
	long	lblk;

	lncyl = get_default_lncyl(scgp, dp);
top:
	getlong("Enter number of data      heads     (Label)",
						&dp->lhead, 1L, 0xFFFEL);
	getlong("Enter number of data sectors/track  (Label)",
						&dp->lspt, 1L, 0xFFFEL);
	getlong("Enter number of alternate cylinders (Label)",
					&dp->lacyl, minlacyl, 10L);

	maxlncyl = dp->pcyl;
	maxlncyl *= dp->nhead*dp->spt;

	if (!reformat_only)
	printf("Schätze cap: %ld ist cap: %ld\n", maxlncyl, (long)scgp->cap->c_baddr+1);

	maxlncyl /= dp->lhead*dp->lspt;
	maxlncyl -= dp->lacyl;

	lncyl = get_default_lncyl(scgp, dp);

	if (!reformat_only) {
		printf("Current lncyl: %ld\n", dp->lncyl);
		printf("Schätze lncyl: %ld\n", lncyl);
		printf("old: %ld new: %ld\n",
				dp->pcyl-dp->lacyl, maxlncyl);
	}
	if (lncyl > maxlncyl && lncyl > dp->lpcyl)
		maxlncyl = lncyl;
	if (dp->lpcyl-dp->lacyl > maxlncyl)
		maxlncyl = dp->lpcyl-dp->lacyl;
	if (dp->lncyl < 0)
		dp->lncyl = lncyl;
	getlong("Enter number of data      cylinders (Label)",
					&dp->lncyl, 1L, maxlncyl);

	dp->int_cyl = dp->pcyl - dp->lncyl
			- dp->lacyl - dp->atrk/dp->nhead;
/*printf("int_cyl: %d\n", dp->int_cyl);*/

	if (!has_space_for_acyl(scgp, dp, &lblk)) {
		if (!yes("Use this label geometry? ")) {
			if (yes("Adjust number of data cylinders to maximum possible value? "))
				dp->lncyl -= lblk;
			goto top;
		}
	}

	/*
	 * Hilfe fuer die Wartung der Datei 'sformat.dat'
	 * Der Service soll nicht sehen, dasz Platz verschenkt ist.
	 */
	if (!reformat_only && has_unused_space(scgp, dp, &lblk)) {
		if (!yes("Use this label geometry? ")) {
			if (yes("Adjust number of data cylinders to maximum possible value? "))
				dp->lncyl += lblk;
			goto top;
		}
	}
}

EXPORT BOOL
select_backup_label(scgp, dp, lgeom_ok)
	SCSI	*scgp;
	struct disk	*dp;
	BOOL	lgeom_ok;
{
	union	x_label d_blabel;
	long	start = 0L;

	if (!lgeom_ok) {
		printf("WARNING:\n\n");
		printf("The label geometry is not known at this time.\n\n");
		printf("Scanning for backup labels may take a long time!\n");
		printf("If you want to confirm the correct geometry,\n");
		printf("hit RETURN on the next question.\n");
	}
	if (!yes("%s for backup labels? ", lgeom_ok ? "Search":"Scan"))
		return (FALSE);

	if (lgeom_ok && read_backup_label(scgp, dp, &d_blabel.label)) {
		printf("<%s>\n", d_blabel.label.dkl_asciilabel);
	} else {
		do {
			if (!scan_backup_label(scgp, &d_blabel.label,
							&start, lgeom_ok))
				return (FALSE);

			printf("<%s>\n", d_blabel.label.dkl_asciilabel);
			if (yes("Print partition table? ")) {
				printparts(&d_blabel.label);
			}
		} while (yes("Scan for other backup label? "));
	}

	if (yes("Use backuplabel? ")) {
		movebytes((caddr_t)&d_blabel.label,
				(caddr_t)d_label, sizeof (*d_label));

		if (!setval_from_label(dp, d_label)) {
			print_label_err(d_label);
			fillbytes((caddr_t)d_label, sizeof (*d_label), '\0');
			return (FALSE);
		}
		return (TRUE);
	}
	return (FALSE);
}

LOCAL BOOL
get_defpart(scgp, dp, lp)
	SCSI		*scgp;
	struct disk	*dp;
	struct dk_label *lp;
{
	get_lgeom_defaults(scgp, dp);
	label_null(lp);
	return (get_part_defaults(scgp, dp, lp));
}

EXPORT void
select_partition(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	struct	dk_label d_xlabel;
	char	*pname = dp->default_part;

	label_null(&d_xlabel);
	if (dp->labelread > 0)
		movebytes((char *)d_label, (char *)&d_xlabel, sizeof (*d_label));
	if (!yes("Use default partition? "))
		pname = NULL;
	if (ext_part(scgp, dp->disk_type, pname, dp->default_part,
						&d_xlabel, get_defpart, dp)) {
		if (dp->labelread < 0 || yes("Use selected label? ")) {
			printf("setval: %d\n", setval_from_label(dp, &d_xlabel));
/*			merge_label(&d_xlabel, d_label);*/
			movebytes((char *)&d_xlabel, (char *)d_label,
							sizeof (*d_label));
			if (dp->labelread < 0)
				dp->labelread = 0;
			else
				dp->labelread = 1;
		}
#ifdef	ALT_deflabel
	/*
	 * Wenn die Autpart hier und nicht in ext_part() vorgenommen wird, dann
	 * wird dp->labelread nicht auf 0 gesetzt und damit
	 * immer Labelgeometrie abgefragt. Ist das besser?
	 */
	} else if (pname == NULL) {
		if (get_defpart(&d_xlabel)) {
			printparts(&d_xlabel);
			movebytes((char *)&d_xlabel, (char *)d_label,
							sizeof (*d_label));
		}
#endif
	}
}

EXPORT void
get_default_partition(scgp, dp)
	SCSI		*scgp;
	struct disk	*dp;
{
	struct	dk_label d_xlabel;
	char	*pname = dp->default_part;

	label_null(&d_xlabel);
	/*
	 * Wenn nur default Label aus der Datenbank zulässig sein sollen, dann
	 * muß statt get_defpart() ein NULL Pointer genommen werden.
	 */
	if (ext_part(scgp, dp->disk_type, pname, pname,
						&d_xlabel, get_defpart, dp)) {
			if (!setval_from_label(dp, &d_xlabel)) {
				comerrno(EX_BAD, "BAD default partition.\n");
				/* NOTREACHED */
			}
/*			merge_label(&d_xlabel, d_label);*/
			movebytes((char *)&d_xlabel, (char *)d_label,
							sizeof (*d_label));
			dp->labelread = 1;
	} else {
		comerrno(EX_BAD, "Default partition not found.\n");
		/* NOTREACHED */
	}
}
