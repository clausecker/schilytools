/* @(#)find.c	1.54 09/01/13 Copyright 2004-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)find.c	1.54 09/01/13 Copyright 2004-2009 J. Schilling";
#endif
/*
 *	Another find implementation...
 *	The main code is now in libfind.
 *
 *	Copyright (c) 2004-2009 J. Schilling
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
#include <stdio.h>		/* walk.h and find.h need stdio.h */
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include <schily/walk.h>
#include <schily/find.h>

EXPORT	int	main		__PR((int ac, char **av));

EXPORT int
main(ac, av)
	int	ac;
	char	**av;
{
	FILE	*std[3];

	std[0] = stdin;
	std[1] = stdout;
	std[2] = stderr;
	return (find_main(ac, av, NULL, std, NULL));
}
