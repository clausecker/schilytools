/* @(#)builtin.c	1.13 19/01/13 Copyright 2015-2019 J. Schilling */
#include <schily/mconfig.h>
static	UConst char sccsid[] =
	"@(#)builtin.c	1.13 19/01/13 Copyright 2015-2019 J. Schilling";
#ifdef DO_SYSBUILTIN
/*
 *	builtlin builtin
 *
 *	Copyright (c) 2015-2019 J. Schilling
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
#include <schily/dlfcn.h>

#define	LOCAL	static

struct libs {
	void		*lib;
	struct libs	*next;
};

	struct sysnod2 *sh_findbuiltin	__PR((Uchar *name));
LOCAL	int		sh_addbuiltin	__PR((Uchar *name, bftype func));
LOCAL	void		sh_rmbuiltin	__PR((Uchar *name));

LOCAL	struct sysnod2	*bltins;	/* List of active builtins */
LOCAL	void		*libs;		/* List of active libraries */

void
sysbuiltin(argc, argv)
	int	argc;
	unsigned char	**argv;
{
	struct optv optv;
	int del;
	UInt16_t	mask;
	int c;
	char *farg;
	const struct sysnod	*sp = commands;
	int		i;

	optinit(&optv);
	del = 0;
	mask = 0;
	farg = NULL;

	while ((c = optget(argc, argv, &optv, "df:is")) != -1) {
		switch (c) {
		case 'd':
			del++;
			continue;
		case 'f':
			farg = optv.optarg;
			continue;
		case 'i':
			mask |= BLT_INT;
			continue;
		case 's':
			mask |= BLT_SPC;
			continue;

		default:
			break;

		case '?':
			gfailure((unsigned char *)usage, builtinuse);
			return;
		}
	}

	/*
	 * If no arguments, just print the builtin commands
	 */
	if (optv.optind == argc && !del && !farg) {
		for (i = 0; i < no_commands; i++) {
			if (sp[i].sysflg & BLT_DEL)
				continue;
			if (mask && (sp[i].sysflg & mask) == 0)
				continue;
			prs_buff(UC sp[i].sysnam);
			prc_buff(NL);
		}
	} else if (flags & rshflg) {	/* Managing builtins is restricted */
		/*
		 * For security reasons, abort scripts that try to use
		 * restricted features in a restricted shell.
		 */
		failed(argv[0], restricted);
	} else if (farg) {		/* Add shared library */
#ifdef	HAVE_LOADABLE_LIBS
		void		*lh;
		struct libs	*lp;

		lh = dlopen(farg, RTLD_LAZY);
		if (lh == NULL) {
			failure(argv[0], dlerror());
			return;
		}
		lp = alloc(sizeof (*lp));
		if (lp == NULL)
			return;
		lp->lib = lh;
		lp->next = libs;
		libs = lp;
#else
		failure(argv[0], "-f not supported on this platform");
		return;
#endif
	}
	/*
	 * Add or delete builtin
	 */
#ifdef	HAVE_LOADABLE_LIBS
	if (libs == NULL && optv.optind < argc) {
		failure(argv[0], "no libraries");
		return;
	}
	for (; optv.optind < argc; optv.optind++) {
		struct libs	*lp = libs;
		void		*func;
		unsigned char	*name = argv[optv.optind];

#ifdef	BUILTIN_DEBUG				/* Optional until ready */
		/* XXX avoid printf */
		printf("arg[%d] '%s'\n", optv.optind, argv[optv.optind]);
#endif
		do {
			func = dlsym(lp->lib, C name);
			lp = lp->next;
		} while (func == NULL && lp);
		if (func == NULL) {
			failure(name, notfound);
			continue;
		}
		if (del) {
			sh_rmbuiltin(name);
		} else {
			if (sh_findbuiltin(name))
				failure(name, &notfound[4]);
			sh_addbuiltin(name, (bftype) func);
		}
	}
#endif
}

/*
 * Return sysnod2 ptr for active loadable builtin.
 */
struct sysnod2 *
sh_findbuiltin(name)
	unsigned char	*name;
{
	struct sysnod2	*bp;

	for (bp = bltins; bp; bp = bp->snext) {
		if (eq(name, bp->sysnam))
			return (bp);
	}
	return (0);
}

/*
 * Add new loadable builtin function with command name to active list.
 */
LOCAL int
sh_addbuiltin(name, func)
	unsigned char	*name;
	bftype		func;
{
	struct sysnod2  *bp = alloc(sizeof (struct sysnod2));

	if (bp == NULL)
		return (1);

	bp->sysnam = C make(name);
	bp->sysval = 0;
	bp->sysflg = 0;
	bp->sysptr = func;
	bp->snext = bltins;
	bltins = bp;
	return (0);
}

/*
 * Remove loadable builtin function from active list.
 */
LOCAL void
sh_rmbuiltin(name)
	unsigned char	*name;
{
	struct sysnod2  *bp;
	struct sysnod2  *obp;

	for (obp = bp = bltins; bp; obp = bp, bp = bp->snext) {
		if (!eq(name, bp->sysnam))
			continue;

		if (bp == bltins)
			bltins = bp->snext;
		else
			obp->snext = bp->snext;
		free(bp->sysnam);
		free(bp);		/* invalidates use of bp->snext */
		/*
		 * We only allow one element with the same name and we
		 * would need a different method than using obp.
		 */
		break;
	}
}

#endif /* DO_SYSBUILTIN */
