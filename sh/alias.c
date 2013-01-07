/* @(#)alias.c	1.2 13/01/05 Copyright 1986-2013 J. Schilling */
#include <schily/mconfig.h>
static	UConst char sccsid[] =
	"@(#)alias.c	1.2 13/01/05 Copyright 1986-2013 J. Schilling";
/*
 *	The built-in commands "alias" and "unalias".
 *
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

#include "defs.h"
#include "abbrev.h"

#undef	tab
#define	LOCAL	static

void
sysalias(argc, argv)
	int	argc;
	unsigned char	**argv;
{
extern int opterr, optind;
	int	savopterr = opterr;
	int	savoptind = optind;
	int	savsp = _sp;
	char	*savoptarg = optarg;
	int	c;
	int	ret = 1;
	int	badflag = 0;
	int	allflag = 0;
	int	persist = 0;
	int	doglobal = 0;
	int	dolocal = 0;
	int	pflag = 0;
	int	doreload = 0;
	int	doraw = 0;
	abidx_t	tab;
	int	aflags = 0;
	int	lflags = 0;
	int	pflags = 0;
	unsigned char	*a1;
	unsigned char	o[3];

	optind = 1;
	_sp = 1;
	opterr = 0;
	o[0] = '-';
	o[2] = '\0';
	while ((c = getopt(argc, (char **)argv,
			    "aeglprR(raw)")) != -1) {
		switch (c) {
		case 'a':
			allflag++;
			break;
		case 'e':
			persist++;
			break;
		case 'g':
			if ((flags & globalaliasflg) == 0) {
				o[1] = c;
				badflag++;
				goto err;
			}
			dolocal = 0;
			doglobal++;
			break;
		case 'l':
			if ((flags & localaliasflg) == 0) {
				o[1] = c;
				badflag++;
				goto err;
			}
			doglobal = 0;
			dolocal++;
			break;
		case 'p':
			pflag++;
			break;
		case 'r':
			doreload++;
			break;
		case 'R':
			doraw++;
			break;
		case '?':
			gfailure((unsigned char *)usage, aliasuse);
			goto err;
		}
	}
	ret = 0;
	c = optind;
err:
	optind = savoptind;
	opterr = savopterr;
	_sp = savsp;
	optarg = savoptarg;
	if (badflag) {
		failed(o, badopt);
		/* NOTREACHED */
	}
	if (ret)
		return;

	tab = dolocal?LOCAL_AB:GLOBAL_AB;
	if (!allflag)
		aflags = AB_BEGIN;
	lflags = (persist?AB_PERSIST:0) |
			(doraw?0:AB_POSIX) |
			(pflag?AB_PARSE|AB_ALL:0);
	if (pflag && dolocal)
		pflags |= AB_PLOCAL;
	else if ((flags & globalaliasflg) != 0)
		pflags |= AB_PGLOBAL;
	if (doreload) {
		char	*fname;

		if (c < argc) {
			failed(argv[0], toomanyargs);
			/* NOTREACHED */
		}
		fname = ab_gname(tab);
		ab_use(tab, fname);
		return;
	}
	if (c >= argc) {
		ab_dump(tab, 1, lflags | pflags);
		return;
	}
	for (; c < argc; c++) {
		unsigned char *val;

		a1 = argv[c];
		val = UC strchr((char *)a1, '=');
		if (val) {
			*val++ = '\0';
			if (pflag || (doglobal == 0 && dolocal == 0))
				ab_push(tab, (char *)make(a1), (char *)make(val), aflags);
			else
				ab_insert(tab, (char *)make(a1), (char *)make(val), aflags);
		} else {
			ab_list(tab, (char *)a1, 1, lflags | pflags);
		}
	}
}

void
sysunalias(argc, argv)
	int	argc;
	unsigned char	**argv;
{
extern int opterr, optind;
	int	savopterr = opterr;
	int	savoptind = optind;
	int	savsp = _sp;
	char	*savoptarg = optarg;
	int	c;
	int	ret = 1;
	int	badflag = 0;
	int	allflag = 0;
	int	doglobal = 0;
	int	dolocal = 0;
	int	pflag = 0;
	abidx_t	tab;
	unsigned char	*a1;
	unsigned char	o[3];

	optind = 1;
	_sp = 1;
	opterr = 0;
	o[0] = '-';
	o[2] = '\0';
	while ((c = getopt(argc, (char **)argv,
			    "aglp")) != -1) {
		switch (c) {
		case 'a':
			allflag++;
			break;
		case 'g':
			if ((flags & globalaliasflg) == 0) {
				o[1] = c;
				badflag++;
				goto err;
			}
			dolocal = 0;
			doglobal++;
			break;
		case 'l':
			if ((flags & localaliasflg) == 0) {
				o[1] = c;
				badflag++;
				goto err;
			}
			doglobal = 0;
			dolocal++;
			break;
		case 'p':
			pflag++;
			break;
		case '?':
			gfailure((unsigned char *)usage, unaliasuse);
			goto err;
		}
	}
	ret = 0;
	c = optind;
err:
	optind = savoptind;
	opterr = savopterr;
	_sp = savsp;
	optarg = savoptarg;
	if (badflag) {
		failed(o, badopt);
		/* NOTREACHED */
	}
	if (ret)
		return;

	tab = dolocal?LOCAL_AB:GLOBAL_AB;
	if (c >= argc) {
		if (allflag) {
			ab_deleteall(tab, AB_INTR | AB_POP);
			return;
		}
		gfailure((unsigned char *)usage, unaliasuse);
		return;
	}
	for (; c < argc; c++) {
		a1 = argv[c];
		if (pflag || (doglobal == 0 && dolocal == 0))
			ab_delete(tab, (char *)a1, AB_POP | AB_POPALL);
		else
			ab_delete(tab, (char *)a1, 0);
	}
}
