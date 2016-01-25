/* @(#)alias.c	1.11 16/01/21 Copyright 1986-2016 J. Schilling */
#include <schily/mconfig.h>
static	UConst char sccsid[] =
	"@(#)alias.c	1.11 16/01/21 Copyright 1986-2016 J. Schilling";
/*
 *	The built-in commands "alias" and "unalias".
 *
 *	Copyright (c) 1986-2016 J. Schilling
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

#include "defs.h"
#include "abbrev.h"

#undef	tab
#define	LOCAL	static

void
sysalias(argc, argv)
	int	argc;
	unsigned char	**argv;
{
	struct optv optv;
	int	c;
	int	ret = 1;
	int	badflag = 0;	/* -g/-l with {local!global}aliases disabled */
	int	allflag = 0;	/* -a non-begin type alias (#a) */
	int	persist = 0;	/* -e persistent (everlasing) macros */
	int	doglobal = 0;	/* -g persistent global aliases */
	int	dolocal = 0;	/* -l persistent local aliases */
	int	pflag = 0;	/* -p push or list parsable */
	int	doreload = 0;	/* -r reload from persistent definitions */
	int	doraw = 0;	/* -R/-raw list in raw mode */
	abidx_t	tab;
	int	aflags = 0;	/* All (non-begin) type alias */
	int	lflags = 0;	/* List flags */
	int	pflags = 0;	/* List parseable flags */
	unsigned char	*a1;
	unsigned char	o[3];

	optinit(&optv);
	o[0] = '-';
	o[2] = '\0';
	while ((c = optget(argc, argv, &optv,
			    "()aeglprR(raw)")) != -1) {
		switch (c) {
		case 'a':
			allflag++;
			break;
		case 'e':
			persist++;
			break;
		case 'g':
			if ((flags2 & globalaliasflg) == 0) {
				o[1] = c;
				badflag++;
				goto err;
			}
			dolocal = 0;
			doglobal++;
			break;
		case 'l':
			if ((flags2 & localaliasflg) == 0) {
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
	c = optv.optind;
err:
	if (badflag) {
		failure(o, badopt);
		return;
	}
	if (ret)
		return;

	tab = dolocal?LOCAL_AB:GLOBAL_AB;
	if (!allflag)
		aflags = AB_BEGIN;
	lflags = (persist?AB_PERSIST:0) |
			(doraw?0:AB_POSIX) |
			(pflag?AB_PARSE|AB_ALL:0);
	if (pflag) {
		if (dolocal)
			pflags |= AB_PLOCAL;
		else if ((flags2 & globalaliasflg) != 0)
			pflags |= AB_PGLOBAL;
	}
	if (doreload) {
		char	*fname;

		if (c < argc) {
			failure(argv[0], toomanyargs);
			return;
		}
		fname = ab_gname(tab);
		ab_use(tab, fname);
		return;
	}
	if (c >= argc) {
		/*
		 * Just list everysthing, never fail.
		 */
		ab_dump(tab, 1, lflags | pflags);
		return;
	}
	for (; c < argc; c++) {
		unsigned char *val;

		a1 = argv[c];
		val = UC strchr((char *)a1, '=');
		if (val) {
			*val++ = '\0';
			if (pflag || (doglobal == 0 && dolocal == 0)) {
				if (!ab_push(tab,
						(char *)make(a1),
						(char *)make(val),
						aflags)) {
					failure(a1, "cannot push alias");
				}

			} else {
				if (!ab_insert(tab,
						(char *)make(a1),
						(char *)make(val),
						aflags)) {
					failure(a1, "cannot define alias");
				}
			}
		} else {
			if (!ab_list(tab, (char *)a1, 1, lflags | pflags))
				failure(a1, "alias not found");
		}
	}
}

void
sysunalias(argc, argv)
	int	argc;
	unsigned char	**argv;
{
	struct optv optv;
	int	c;
	int	ret = 1;
	int	badflag = 0;	/* -g/-l with {local!global}aliases disabled */
	int	allflag = 0;	/* -a remove all aliases */
	int	doglobal = 0;	/* -g persistent global aliases */
	int	dolocal = 0;	/* -l persistent local aliases */
	int	pflag = 0;	/* -p pop all (non-persistent) */

	abidx_t	tab;
	unsigned char	*a1;
	unsigned char	o[3];

	optinit(&optv);
	o[0] = '-';
	o[2] = '\0';
	while ((c = optget(argc, argv, &optv, "aglp")) != -1) {
		switch (c) {
		case 'a':
			allflag++;
			break;
		case 'g':
			if ((flags2 & globalaliasflg) == 0) {
				o[1] = c;
				badflag++;
				goto err;
			}
			dolocal = 0;
			doglobal++;
			break;
		case 'l':
			if ((flags2 & localaliasflg) == 0) {
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
	c = optv.optind;
err:
	if (badflag) {
		failure(o, badopt);
		return;
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
		BOOL	r;

		a1 = argv[c];
		if (pflag || (doglobal == 0 && dolocal == 0))
			r = ab_delete(tab, (char *)a1, AB_POP | AB_POPALL);
		else
			r = ab_delete(tab, (char *)a1, 0);
		if (!r)
			failure(a1, "alias not found");
	}
}
