/* @(#)builtin.c	1.4 15/07/24 Copyright 2015 J. Schilling */
#include <schily/mconfig.h>
static	UConst char sccsid[] =
	"@(#)builtin.c	1.4 15/07/24 Copyright 2015 J. Schilling";
#ifdef DO_SYSBUILTIN
/*
 *	builtlin builtin
 *
 *	Copyright (c) 2015 J. Schilling
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
	int savoptind;
	int savopterr;
	int savsp;
	char *savoptarg;
	int del;
	UInt16_t	mask;
	int c;
	char *farg;
	const struct sysnod	*sp = commands;
	int		i;

	savoptind = optind;
	savopterr = opterr;
	savsp = _sp;
	savoptarg = optarg;
	optind = 1;
	_sp = 1;
	opterr = 0;
	optarg = NULL;
	del = 0;
	mask = 0;
	farg = NULL;

	while ((c = getopt(argc, (char **)argv, "df:is")) != -1) {
		switch (c) {
		case 'd':
			del++;
			continue;
		case 'f':
			farg = optarg;
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
			goto err;
		}
	}

	/*
	 * If no arguments, just print the builtin commands
	 */
	if (optind == argc && !del && !farg) {
		for (i = 0; i < no_commands; i++) {
			if (sp[i].sysflg & BLT_DEL)
				continue;
			if (mask && (sp[i].sysflg & mask) == 0)
				continue;
			prs_buff(UC sp[i].sysnam);
			prc_buff(NL);
		}
	} else if (flags & rshflg) {	/* Managing builtins is restricted */
		failed(argv[0], restricted);
	} else if (farg) {		/* Add shared library */
		void	*lh;
#ifdef	HAVE_LOADABLE_LIBS
		lh = dlopen(farg, RTLD_LAZY);
		printf("lh %p\n", lh);
#else
		failed(argv[0], "-f not supported on this platform");
#endif
	}
	/*
	 * Add or delete builtin
	 */
	for (; optind < argc; optind++) {
		printf("arg[%d] '%s'\n", optind, argv[optind]);
	}

err:
	optind = savoptind;
	opterr = savopterr;
	_sp = savsp;
	optarg = savoptarg;
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
