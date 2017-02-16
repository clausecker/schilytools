/* @(#)hparse.c	1.3 17/02/15 Copyright 2011-2017 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)hparse.c	1.3 17/02/15 Copyright 2011-2017 J. Schilling";
#endif
/*
 *	Handling routines to parse a block of header entries from a
 *	StreamArchive
 *
 *	Copyright (c) 2011-2017 J. Schilling
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
#include <schily/utypes.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/strar.h>
#include "header.h"
#include "table.h"

EXPORT void
strar_reset(info)
	register strar	*info;
{
	info->f_mode = 0;
	info->f_size = 0;
	info->f_rsize = 0;
	info->f_xflags = 0;
	info->f_status = 0;
	info->f_xftype = info->f_rxftype = XT_NONE;
	info->f_atime = info->f_mtime = info->f_ctime = 0;
	info->f_name[0] = '\0';
	info->f_lname[0] = '\0';
	info->f_uid = info->f_gid = 0;
	info->f_uname = NULL;
	info->f_gname = NULL;
}

EXPORT BOOL
strar_hparse(info)
	register strar	*info;
{
	register char	*p;
	register char	*ep;
		long	length;
		int	c;
		int	i;
		FILE	*f = info->f_fp;

again:
	strar_xbreset();
	if (strar_gxblen() < 10)
		strar_xbgrow(10);

	/*
	 * Fetch the length element to know about the size of the next field.
	 */
	p = ep = strar_gxbuf();
	length = 0;
	for (i = 0; (c = getc(f)) != EOF && i < 10; i++) {
		*p++ = c;
		if (c >= '0' && c <= '9') {
			length *= 10;
			length += c - '0';
		} else if (c == ' ') {
			break;
		} else {
			return (FALSE);	/* SYNTAX ERROR */
		}
	}
	if (feof(f)) {
		return (FALSE);
	}
	if (strar_gxblen() < length) {
		size_t	off = p - ep;

		strar_xbgrow(length);
		p = ep = strar_gxbuf();
		p += off;
	}
	fileread(f, p, length - i - 1);

	p = strar_gxbuf();
	ep = p + length;

	i = strar_xhparse(info, p, ep);
	if (i > TRUE) {
		/*
		 * Allow a minimalistic archive.
		 */
		if ((info->f_xflags & XF_FILETYPE) == 0)
			info->f_rxftype = XT_FILE;
		/*
		 * If there is no arfiletype (e.g. "hardlink")
		 * set it to the real file type.
		 */
		if (info->f_xftype == XT_NONE)
			info->f_xftype = info->f_rxftype;

		if ((info->f_xflags & XF_MODE) == 0)
			info->f_mode = 0666;
		return (TRUE);
	}
	if (i > 0)
		goto again;
	return (i);
}
