/* @(#)close.c	1.3 18/05/17 Copyright 2017-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)close.c	1.3 18/05/17 Copyright 2017-2018 J. Schilling";
#endif
/*
 *	Close a StreamArchive
 *
 *	Copyright (c) 2017-2018 J. Schilling
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
#include <schily/schily.h>
#include <schily/errno.h>
#include <schily/strar.h>
#include "header.h"

extern	mode_t	strar_old_umask;

int
strar_close(info)
	register FINFO	*info;
{
	if (info->f_fp && info->f_fp != stdin && info->f_fp != stdout) {
		fclose(info->f_fp);
		info->f_fpname = NULL;
	}
	if (info->f_list && info->f_list != stdout && info->f_list != stderr) {
		fclose(info->f_list);
		info->f_listname = NULL;
	}
	umask(strar_old_umask);
	utf8_fini();
	return (0);
}
