/* @(#)optget.c	1.3 15/12/07 Copyright 2015 J. Schilling */
#include <schily/mconfig.h>
static	UConst char sccsid[] =
	"@(#)optget.c	1.3 15/12/07 Copyright 2015 J. Schilling";
/*
 *	A version of getopt() that maintains state
 *	so it can be used from witin a shell builtin
 *	without being in conflict with getopts(1).
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

void
optinit(optv)
	struct optv	*optv;
{
	optv->opterr = 1;
	optv->optind = 1;
	optv->optopt = 0;
	optv->opt_sp = 1;
	optv->optarg = 0;
}

int
optget(argc, argv, optv, optstring)
	int		argc;
	unsigned char	**argv;
	struct optv	*optv;
	const char	*optstring;
{
	int	ret;
	int	savopterr;
	int	savoptind;
	int	savoptopt;
	int	savopt_sp;
	char	*savoptarg;

	savopterr = opterr;
	savoptind = optind;
	savoptopt = optopt;
	savopt_sp = _sp;
	savoptarg = optarg;

	opterr = optv->opterr;
	optind = optv->optind;
	optopt = optv->optopt;
	_sp    = optv->opt_sp;
	optarg = optv->optarg;

	ret = getopt(argc, (char **)argv, optstring);

	optv->opterr = opterr;
	optv->optind = optind;
	optv->optopt = optopt;
	optv->opt_sp = _sp;
	optv->optarg = optarg;

	opterr = savopterr;
	optind = savoptind;
	optopt = savoptopt;
	_sp    = savopt_sp;
	optarg = savoptarg;

	return (ret);
}

void
optbad(argc, argv, optv)
	int		argc;
	unsigned char	**argv;
	struct optv	*optv;
{
	unsigned char	*p;
	unsigned char	opt[4];

	if (optv->optopt == '-' && argc >= optv->optind) {
		p = locstak();
		*p = ' ';
		movstrstak(argv[optv->optind-1], &p[1]);
		p += stakbot - p;
	} else {
		opt[0] = ' '; opt[1] = '-',
		opt[2] = optv->optopt; opt[3] = '\0';
		p = opt;
	}
	bfailure(argv[0], badopt, p);
}

int
optskip(argc, argv, use)
	int	argc;
	unsigned char	**argv;
	const	 char	*use;
{
	struct optv optv;
	int c;

	optinit(&optv);
	optv.opterr = 0;

	while ((c = optget(argc, argv, &optv, ":?1000?(help)")) != -1) {
		switch (c) {

		default:
			break;

		case '?':
			optbad(argc, argv, &optv);
			/* FALLTHROUGH */

		case 1000:
			if (use)
				gfailure((unsigned char *)usage, use);
			return (-1);
		}
	}
	return (optv.optind);
}
