/* @(#)chdir.c	1.5 08/12/22 Copyright 1997-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)chdir.c	1.5 08/12/22 Copyright 1997-2008 J. Schilling";
#endif
/*
 *	Copyright (c) 1997-2008 J. Schilling
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
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include "star.h"
#include "starsubs.h"

#include <schily/dirent.h>
#include <schily/maxpath.h>
#include <schily/getcwd.h>

EXPORT	char	*dogetwdir	__PR((void));
EXPORT	BOOL	dochdir		__PR((const char *dir, BOOL doexit));

extern	BOOL	debug;		/* -debug has been specified	*/

EXPORT char *
dogetwdir()
{
	char	dir[PATH_MAX+1];
	char	*ndir;

/* XXX MAXPATHNAME vs. PATH_MAX ??? */

	if (getcwd(dir, PATH_MAX) == NULL)
		comerr("Cannot get working directory\n");
	ndir = ___malloc(strlen(dir)+1, "working dir");
	strcpy(ndir, dir);
	return (ndir);
}

EXPORT BOOL
dochdir(dir, doexit)
	const char	*dir;
	BOOL		doexit;
{
	if (debug) /* temporary */
		error("dochdir(%s) = ", dir);

	if (chdir(dir) < 0) {
		int	ex = geterrno();

		if (debug) /* temporary */
			error("%d\n", ex);

		errmsg("Cannot change directory to '%s'.\n", dir);
		if (doexit)
			exit(ex);
		return (FALSE);
	}
	if (debug) /* temporary */
		error("%d\n", 0);

	return (TRUE);
}
