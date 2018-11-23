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
/*
 * @(#)homesubs.c	1.1 18/11/07 Copyright 2018 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)homesubs.c 1.1 18/11/07 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)fdirsubs.c"
#pragma ident	"@(#)sccs:lib/comobj/fdirsubs.c"
#endif
#include	<defines.h>

EXPORT	int	opencwd		__PR((void));
EXPORT	int	openphome	__PR((void));

/*
 * Open cwd as reference for fchdir() and things like openat().
 */
EXPORT int
opencwd()
{
	int	fd;

	if ((fd = opendirfd(".")) < 0)
		efatal(gettext("cannot open current directory (co39)"));
	return (fd);
}

/*
 * Open changeset home directory as reference for fchdir() and things like
 * openat().
 */
EXPORT int
openphome()
{
	int	fd;

	if ((fd = opendirfd(setphome)) < 0)
		efatal(gettext("cannot open project home directory (co40)"));
	return (fd);
}
