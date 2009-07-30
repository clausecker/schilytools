/* @(#)alias.c	1.15 09/07/28 Copyright 1986-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)alias.c	1.15 09/07/28 Copyright 1986-2009 J. Schilling";
#endif
/*
 *	Copyright (c) 1986-2009 J. Schilling
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

#include <schily/stdio.h>
#include "bsh.h"
#include "btab.h"
#include "abbrev.h"
#include "str.h"

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
		int	ac;
		char	* const *av;
		char	*opt	= "l,r,reload";
		BOOL	dolocal	= FALSE;
		BOOL	doreload = FALSE;

	ac = vp->av_ac - 1;		/* set values */
	av = &vp->av_av[1];

	if (getargs(&ac, &av, opt, &dolocal, &doreload, &doreload) < 0) {
		fprintf(std[2], ebadopt, vp->av_av[0], av[0]);
		fprintf(std[2], "%s", nl);
		busage(vp, std);
		ex_status = 1;
		return;
	}
	if ((ac > 1) || ((ac > 0 && doreload))) {
		wrong_args(vp, std);
		return;
	}
	if (doreload) {
		abidx_t curtab = dolocal?LOCAL_AB:GLOBAL_AB;
		char	*fname;

		fname = ab_gname(curtab);
		if (fname)
			ab_use(curtab, fname);
		return;
	}
	if (ac == 0) {
		ab_dump(dolocal?LOCAL_AB:GLOBAL_AB, std[1], 0);
		return;
	}
	ab_list(dolocal?LOCAL_AB:GLOBAL_AB, av[0], std[1], 0);
}

/* ARGSUSED */
EXPORT void
bunalias(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
		int	ac;
		char	* const *av;
		char	*opt	= "l";
		BOOL	dolocal	= FALSE;

	ac = vp->av_ac - 1;		/* set values */
	av = &vp->av_av[1];

	if (getargs(&ac, &av, opt, &dolocal) < 0) {
		fprintf(std[2], ebadopt, vp->av_av[0], av[0]);
		fprintf(std[2], "%s", nl);
		busage(vp, std);
		ex_status = 1;
		return;
	}
	if (ac < 1) {
		wrong_args(vp, std);
		return;
	}
	ab_delete(dolocal?LOCAL_AB:GLOBAL_AB, av[0]);
}
