/* @(#)find.c	1.48 07/10/31 Copyright 2004-2007 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)find.c	1.48 07/10/31 Copyright 2004-2007 J. Schilling";
#endif
/*
 *	Another find implementation...
 *	The main code is now in libfind.
 *
 *	Copyright (c) 2004-2007 J. Schilling
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
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/schily.h>

#include "walk.h"
#include "find.h"

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
	return (find_main(ac, av, std, NULL));
}
