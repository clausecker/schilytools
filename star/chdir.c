/* @(#)chdir.c	1.9 18/07/17 Copyright 1997-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)chdir.c	1.9 18/07/17 Copyright 1997-2018 J. Schilling";
#endif
/*
 *	Copyright (c) 1997-2018 J. Schilling
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
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/standard.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include "star.h"
#include "starsubs.h"
#include "checkerr.h"

#include <schily/dirent.h>
#include <schily/maxpath.h>
#include <schily/getcwd.h>

EXPORT	char	*dogetwdir	__PR((void));
EXPORT	BOOL	dochdir		__PR((const char *dir, BOOL doexit));

extern	BOOL	debug;		/* -debug has been specified	*/

EXPORT char *
dogetwdir()
{
	char	*dir;
	char	*ndir;

	if ((dir = lgetcwd()) == NULL)
		comerr("Cannot get working directory\n");
	ndir = ___malloc(strlen(dir)+1, "working dir");
	strcpy(ndir, dir);
	free(dir);
	return (ndir);
}

EXPORT BOOL
dochdir(dir, doexit)
	const	char	*dir;
	BOOL		doexit;
{
	char	*d;

	if (debug) /* temporary */
		error("dochdir(%s) = ", dir);

	d = strdup(dir);
	if (d == NULL || lchdir(d) < 0) {
		int	ex = geterrno();

		if (debug) /* temporary */
			error("%d\n", ex);

		if (!errhidden(E_CHDIR, dir)) {
			if (!errwarnonly(E_CHDIR, dir))
				xstats.s_chdir++;
			errmsg("Cannot change directory to '%s'.\n", dir);
			(void) errabort(E_CHDIR, dir, TRUE);
		}
		if (doexit)
			exit(ex);
		if (d)
			free(d);
		return (FALSE);
	}
	if (debug) /* temporary */
		error("%d\n", 0);

	if (d)
		free(d);
	return (TRUE);
}
