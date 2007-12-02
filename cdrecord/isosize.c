/* @(#)isosize.c	1.10 06/09/13 Copyright 1996, 2001-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)isosize.c	1.10 06/09/13 Copyright 1996, 2001-2004 J. Schilling";
#endif
/*
 *	Copyright (c) 1996, 2001-2004 J. Schilling
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
#include <schily/stat.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/intcvt.h>

#include "iso9660.h"
#include "cdrecord.h"	/* to verify isosize() prototype */

Llong	isosize		__PR((int f));

Llong
isosize(f)
	int	f;
{
	struct iso9660_voldesc		vd;
	struct iso9660_pr_voldesc	*vp;
	Llong				isize;
	struct stat			sb;
	mode_t				mode;

	/*
	 * First check if a bad guy tries to call isosize()
	 * with an unappropriate file descriptor.
	 * return -1 in this case.
	 */
	if (isatty(f))
		return ((Llong)-1);
	if (fstat(f, &sb) < 0)
		return ((Llong)-1);
	mode = sb.st_mode & S_IFMT;
	if (!S_ISREG(mode) && !S_ISBLK(mode) && !S_ISCHR(mode))
		return ((Llong)-1);

	if (lseek(f, (off_t)(16L * 2048L), SEEK_SET) == -1)
		return ((Llong)-1);

	vp = (struct iso9660_pr_voldesc *) &vd;

	do {
		read(f, &vd, sizeof (vd));
		if (GET_UBYTE(vd.vd_type) == VD_PRIMARY)
			break;

	} while (GET_UBYTE(vd.vd_type) != VD_TERM);

	lseek(f, (off_t)0L, SEEK_SET);

	if (GET_UBYTE(vd.vd_type) != VD_PRIMARY)
		return (-1L);

	isize = (Llong)GET_BINT(vp->vd_volume_space_size);
	isize *= GET_BSHORT(vp->vd_lbsize);
	return (isize);
}
