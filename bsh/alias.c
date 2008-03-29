/* @(#)alias.c	1.10 08/03/27 Copyright 1986-2008 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)alias.c	1.10 08/03/27 Copyright 1986-2008 J. Schilling";
#endif
/*
 *	Copyright (c) 1986-2008 J. Schilling
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
#include "bsh.h"
#include "btab.h"
#include "abbrev.h"

extern	abidx_t	deftab;

EXPORT	void	balias		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bunalias	__PR((Argvec * vp, FILE ** std, int flag));

/* ARGSUSED */
EXPORT void
balias(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	if (vp->av_ac == 1)
		ab_dump(deftab, std[1], 0);
	else if (vp->av_ac == 2)
		ab_list(deftab, vp->av_av[1], std[1], 0);
}

/* ARGSUSED */
EXPORT void
bunalias(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	if (vp->av_ac < 2)
		wrong_args(vp, std);
	else
		ab_delete(deftab, vp->av_av[1]);
}
