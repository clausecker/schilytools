/* @(#)alias.c	1.17 13/06/03 Copyright 1986-2013 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)alias.c	1.17 13/06/03 Copyright 1986-2013 J. Schilling";
#endif
/*
 *	Copyright (c) 1986-2013 J. Schilling
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
#include "strsubs.h"
#include "str.h"

extern	abidx_t	deftab;

LOCAL	int	parselocal	__PR((const char *arg, void *valp, int *pac, char *const **pav, const char *opt));
EXPORT	void	balias		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bunalias	__PR((Argvec * vp, FILE ** std, int flag));


/* ARGSUSED */
LOCAL int
parselocal(arg, valp, pac, pav, opt)
	const char	*arg;
	void		*valp;
	int		*pac;
	char	*const	**pav;
	const char	*opt;
{
	char	*op = *pav[0];
	char	c;

	while ((c = *(++op)) != '\0') {
		if (c == 'l')
			*(int *)valp = TRUE;
		else if (c == 'g')
			*(int *)valp = FALSE;
	}
	return (1);
}


/*
 * alias		list all
 * alias name		list name
 * alias name=value	push new alias
 * alias -a		create non-begin type alias
 * alias -p		list with "alias " prefix (bash/ksh93)
 * alias -g		use .global aliases
 * alias -l		use .local aliases
 * alias -p -g		push .global aliases
 * alias -p -l		push .local aliases
 * alias -e		everlasting aliases ???
 * alias -r		reload from .globals/.locals
 * alias -R		list in raw mode
 *
 * alias -t		outdated ksh93
 * alias -x		outdated ksh93
 */
/* ARGSUSED */
EXPORT void
balias(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	int	ac;
	char	* const *av;
	char	*opt	= "a,e,g~,l~,p,r,reload,R,raw";
	int	islocal = -1;
	BOOL	allflag = FALSE;	/* -a non-begin type alias (#a) */
	BOOL	persist = FALSE;	/* -e persistent (everlasing) macros */
	BOOL	doglobal = FALSE;	/* -g persistent global aliases */
	BOOL	dolocal = FALSE;	/* -l persistent local aliases */
	BOOL	pflag = FALSE;		/* -p push or list parsable */
	BOOL	doreload = FALSE;	/* -r reload from persistent definitions */
	BOOL	doraw = FALSE;		/* -R/-raw list in raw mode */
	abidx_t	tab;
	int	aflags = 0;		/* All (non-begin) type alias */
	int	lflags = 0;		/* List flags */
	int	pflags = 0;		/* List parseable flags */

	ac = vp->av_ac - 1;		/* set values */
	av = &vp->av_av[1];

	if (getargs(&ac, &av, opt, &allflag, &persist,
				parselocal, &islocal,
				parselocal, &islocal,
				&pflag,
				&doreload, &doreload,
				&doraw, &doraw) < 0) {
		if (av[0][0] == '-') {
			fprintf(std[2], ebadopt, vp->av_av[0], av[0]);
			fprintf(std[2], "%s", nl);
			busage(vp, std);
			ex_status = 1;
			return;
		}
	}
	if (islocal > 0)
		dolocal = TRUE;
	else if (islocal == 0)
		doglobal = TRUE;
	tab = dolocal?LOCAL_AB:GLOBAL_AB;
	if (!allflag)
		aflags = AB_BEGIN;
	lflags = (persist?AB_PERSIST:0) |
			(doraw?0:AB_POSIX) |
			(pflag?AB_PARSE|AB_ALL:0);
	if (pflag) {
		if (dolocal)
			pflags |= AB_PLOCAL;
		else
			pflags |= AB_PGLOBAL;
	}

	if (doreload) {
		char	*fname;

		if (ac > 0) {
			wrong_args(vp, std);
			return;
		}
		fname = ab_gname(tab);
		if (fname)
			ab_use(tab, fname);
		return;
	}
	if (ac == 0) {
		ab_dump(tab, std[1], lflags | pflags);
		return;
	}
	for (; ac > 0; ac--, av++ ) {
		char	*a1;
		char	*val;

		a1 = av[0];
		val = strchr(a1, '=');
		if (val) {
			*val++ = '\0';
			if (pflag || (doglobal == 0 && dolocal == 0))
				ab_push(tab, makestr(a1), makestr(val), aflags);
			else
				ab_insert(tab, makestr(a1), makestr(val), aflags);
		} else {
			ab_list(tab, a1, std[1], lflags | pflags);
		}
	}
}

/*
 * unalias name		pop alias
 * unalias -a		pop all aliases
 * unalias -g		use .global aliases
 * unalias -l		use .local aliases
 * unalias -p -g	pop .global aliases
 * unalias -p -l	pop .local aliases
 * unalias -e		everlasting aliases ???
 */
/* ARGSUSED */
EXPORT void
bunalias(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	int	ac;
	char	* const *av;
	char	*opt	= "a,g~,l~,p";
	int	islocal = -1;
	BOOL	allflag = FALSE;	/* -a remove all aliases */
	BOOL	doglobal = FALSE;	/* -g persistent global aliases */
	BOOL	dolocal = FALSE;	/* -l persistent local aliases */
	BOOL	pflag = FALSE;		/* -p pop all (non-persistent) */
	abidx_t	tab;

	ac = vp->av_ac - 1;		/* set values */
	av = &vp->av_av[1];

	if (getargs(&ac, &av, opt, &allflag,
				parselocal, &islocal,
				parselocal, &islocal,
				&pflag) < 0) {
		fprintf(std[2], ebadopt, vp->av_av[0], av[0]);
		fprintf(std[2], "%s", nl);
		busage(vp, std);
		ex_status = 1;
		return;
	}
	if (islocal > 0)
		dolocal = TRUE;
	else if (islocal == 0)
		doglobal = TRUE;
	tab = dolocal?LOCAL_AB:GLOBAL_AB;

	if (ac < 1) {
		if (allflag) {
			ab_deleteall(tab, AB_INTR | AB_POP);
			return;
		}
		wrong_args(vp, std);
		return;
	}
	if (allflag) {
		wrong_args(vp, std);
		return;
	}
	for (; ac > 0; ac--, av++ ) {
		char	*a1 = av[0];

		if (pflag || (doglobal == 0 && dolocal == 0))
			ab_delete(tab, a1, AB_POP | AB_POPALL);
		else
			ab_delete(tab, a1, 0);
	}
}
