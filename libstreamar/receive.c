/* @(#)receive.c	1.3 17/02/16 Copyright 2011-2017 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)receive.c	1.3 17/02/16 Copyright 2011-2017 J. Schilling";
#endif
/*
 *	Receive files from a StreamArchive
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
#include "table.h"

int
strar_receive(info, func)
	strar	*info;
	int	(*func) __PR((strar *));
{
	char nbuf[4096];
	char lbuf[4096];
	int	n;
	FILE	*f = info->f_fp;

	info->f_name = nbuf;
	info->f_lname = lbuf;
	strar_reset(info);
	strar_xbreset();
	n = 0;

	do {
		n = strar_hparse(info);

		if (info->f_xflags & XF_EOF) {
#ifdef	DEBUG
			printf("EOF\n");
#endif
			return (0);
		}
		if (info->f_xflags & XF_STATUS) {
			/*
			 * status was read above.
			 */
			if (feof(f))
				return (1);

			strar_reset(info);
			continue;
		}
		if (!n)
			return (1);

		if (feof(f))
			return (1);

		if (func(info) < 0)
			return (1);

		if (feof(f))
			return (1);
		if (info->f_xflags & XF_STATUS) {
			/*
			 * status was read from within strar_get()
			 */
			if (feof(f))
				return (1);

			strar_reset(info);
		}
	} while (1);
}
