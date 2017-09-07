/* @(#)builtin.c	1.10 17/09/01 Copyright 2015-2017 J. Schilling */
#include <schily/mconfig.h>
static	UConst char sccsid[] =
	"@(#)builtin.c	1.10 17/09/01 Copyright 2015-2017 J. Schilling";
#ifdef DO_SYSBUILTIN
/*
 *	builtlin builtin
 *
 *	Copyright (c) 2015-2017 J. Schilling
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

struct sysnod2 *sh_findbuiltin	__PR((char *name));
int		sh_addbuiltin	__PR((char *name, bftype func));

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
		void	*lh;
#ifdef	HAVE_LOADABLE_LIBS
		lh = dlopen(farg, RTLD_LAZY);
		printf("lh %p\n", lh);		/* XXX avoid printf */
#else
		failure(argv[0], "-f not supported on this platform");
#endif
	}
	/*
	 * Add or delete builtin
	 */
#ifdef	BUILTIN_DEBUG				/* Optional until ready */
	for (; optv.optind < argc; optv.optind++) {
		/* XXX avoid printf */
		printf("arg[%d] '%s'\n", optv.optind, argv[optv.optind]);
	}
#endif
}

struct sysnod2 *
sh_findbuiltin(name)
	char	*name;
{
	return (0);
}

int
sh_addbuiltin(name, func)
	char	*name;
	bftype	func;
{
	return (0);
}

#endif /* DO_SYSBUILTIN */
