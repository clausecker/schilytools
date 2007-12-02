/* @(#)saveargs.c	1.13 06/10/05 Copyright 1995-2003 J. Schilling */
/*
 *	save argc, argv for command error printing routines
 *
 *	Copyright (c) 1995-2003 J. Schilling
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
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/stdlib.h>
#include <schily/avoffset.h>
#include <schily/schily.h>

#if	!defined(AV_OFFSET) || !defined(FP_INDIR)
#	ifdef	HAVE_SCANSTACK
#	undef	HAVE_SCANSTACK
#	endif
#endif

static	int	ac_saved;
static	char	**av_saved;
static	char	*av0_saved;
static	char	*progname_saved;

static	char	av0_sp[32];	/* av0 space, avoid malloc() in most cases */
static	char	prn_sp[32];	/* name space, avoid malloc() in most cases */
static	char	dfl_str[] = "?";

EXPORT void
save_args(ac, av)
	int	ac;
	char	*av[];
{
	int	slen;

	ac_saved = ac;
	av_saved = av;

	if (av0_saved && av0_saved != av0_sp)
		free(av0_saved);

	slen = strlen(av[0]) + 1;

	if (slen <= (int)sizeof (av0_sp))
		av0_saved = av0_sp;
	else
		av0_saved = malloc(slen);

	if (av0_saved)
		strcpy(av0_saved, av[0]);
}

EXPORT int
saved_ac()
{
	return (ac_saved);
}

EXPORT char **
saved_av()
{
	return (av_saved);
}

EXPORT char *
saved_av0()
{
	return (av0_saved);
}

EXPORT void
set_progname(name)
	const char	*name;
{
	int	slen;

	if (progname_saved && progname_saved != prn_sp)
		free(progname_saved);

	slen = strlen(name) + 1;

	if (slen <= sizeof (prn_sp))
		progname_saved = prn_sp;
	else
		progname_saved = malloc(slen);

	if (progname_saved)
		strcpy(progname_saved, name);
}

EXPORT char *
get_progname()
{
#ifdef	HAVE_SCANSTACK
	char	*progname;
#endif

	if (progname_saved)
		return (progname_saved);
	if (av0_saved)
		return (av0_saved);
#ifdef	HAVE_SCANSTACK
	progname = getav0();		/* scan stack to get argv[0] */
	if (progname)
		return (progname);
#endif
	return (dfl_str);
}
