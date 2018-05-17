/* @(#)checkmount.c	1.20 09/07/11 Copyright 1991-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)checkmount.c	1.20 09/07/11 Copyright 1991-2009 J. Schilling";
#endif
/*
 *	Check if disk or part of disk is mounted
 *
 *	Copyright (c) 1991-2009 J. Schilling
 *
 *	XXX #ifdef HAVE_DKIO ist vorerst nur ein Hack
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
#include <schily/unistd.h>
#include <schily/fcntl.h>
#include <schily/stat.h>
#include "dsklabel.h"
#include <schily/device.h>
#include <schily/standard.h>
#include <schily/schily.h>
#ifdef	HAVE_SYS_MNTENT_H
#include <sys/mnttab.h>		/* Before sys/mntent.h for SCO UnixWare */
#endif
#ifdef	HAVE_SYS_MNTENT_H
#include <sys/mntent.h>
#endif

#ifdef	HAVE_MNTENT_H
#include <mntent.h>
#ifndef	MNTTAB
#	define	MNTTAB	MOUNTED
#endif
#define	mnt_special	mnt_fsname
#endif

#include "fmt.h"
#include "map.h"

extern	int	autoformat;

extern	struct	dk_label *d_label;

EXPORT	BOOL	checkmount __PR((int, int, int, long, long));

EXPORT BOOL
checkmount(scsibus, target, lun, start, end)
	int	scsibus;
	int	target;
	int	lun;
	long	start;
	long	end;
{
#ifndef	HAVE_DKIO
	return (FALSE);
#else
	FILE	*mf;
	int	f;
	struct stat sb;
	struct dk_conf	conf;
	scgdrv	*scgmap;
#ifdef	SVR4
	struct	mnttab	_mnt;
	struct	mnttab	*mnt = &_mnt;
#else
	struct	mntent	*mnt;
#endif
	char	*dname;
	char	cdisk[128];
	char	rdisk[128];
	int	part;
	long	pstart;
	long	pend;
	BOOL	found = FALSE;

	dname = diskname(maptodisk(scsibus, target, lun));

	/*
	 * Open the mount table.
	 */
	mf = fopen(MNTTAB, "r");
	if (mf == NULL) {
		errmsgno(-1, "WARNING: Cannot open mount table.\n");
		return (found);
	}
#ifdef	SVR4
	while (getmntent(mf, mnt) != -1) {
#else
	while ((mnt = getmntent(mf)) != NULL) {
#endif

/*		printf("testing: %s\n", mnt->mnt_special);*/

		if (sscanf(mnt->mnt_special, "/dev/%s", cdisk) != 1)
			continue;

		strcatl(rdisk, "/dev/r", cdisk, (char *)NULL);

		if ((f = open(rdisk, O_RDONLY|O_NDELAY)) < 0)
			continue;
		if (fstat(f, &sb) < 0) {
			close(f);
			continue;
		}
		if ((sb.st_mode & S_IFMT) != S_IFCHR) {
			close(f);
			continue;
		}
		if (ioctl(f, DKIOCGCONF, &conf) < 0) {
			close(f);
			continue;
		}
		close(f);

		scgmap = scg_getdrv(scsibus);
		if (scgmap->scg_cunit != conf.dkc_cnum ||
		    scgmap->scg_caddr != conf.dkc_addr ||
				!streql(scgmap->scg_cname, conf.dkc_cname))
			continue;

		if (conf.dkc_slave != target*8 + lun)
			continue;

		printf("disk: %s %s %s\n", dname, cdisk, mnt->mnt_special);

		if (start < 0) {
			found = TRUE;
			break;
		}
		part = PART(sb.st_rdev);
		/*
		 * Cannot use struct dk_map * because starting with
		 * Solaris 9 there are several different versions of this
		 * structure.
		 */
		pstart = d_label->dkl_map[part].dkl_cylno *
				d_label->dkl_nhead * d_label->dkl_nsect;

		pend = pstart + d_label->dkl_map[part].dkl_nblk;
printf("pstart: %ld pend: %ld\n", pstart, pend);
		if ((start >= pend) || (end < pstart))
			continue;

		printf("disk part: %s %s %s\n", dname, cdisk, mnt->mnt_special);

		found = TRUE;
		break;
	}
	/*
	 * Close the mount table.
	 */
	(void) fclose(mf);

	if (autoformat && found)
		comerrno(-1, "Cannot format mounted disks.\n");

	return (found);
#endif
}
