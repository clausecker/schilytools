/* @(#)paxopts.c	1.1 18/05/21 Copyright 2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)paxopts.c	1.1 18/05/21 Copyright 2018 J. Schilling";
#endif
/*
 *	Copyright (c) 2018 J. Schilling
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
#include <schily/types.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/schily.h>

#include "starsubs.h"

EXPORT	int	ppaxopts	__PR((const char *opts));

extern	BOOL	binflag;

EXPORT BOOL
ppaxopts(opts)
	const char	*opts;
{
	if (opts == NULL)
		return (FALSE);
	if (streql(opts, "binary")) {
		binflag = TRUE;
		return (TRUE);
	}
	return (FALSE);
}
