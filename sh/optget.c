/* @(#)optget.c	1.9 16/06/19 Copyright 2015-2016 J. Schilling */
#include <schily/mconfig.h>
static	UConst char sccsid[] =
	"@(#)optget.c	1.9 16/06/19 Copyright 2015-2016 J. Schilling";
/*
 *	A version of getopt() that maintains state
 *	so it can be used from witin a shell builtin
 *	without being in conflict with getopts(1).
 *
 *	Copyright (c) 2015-2016 J. Schilling
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
#ifdef	__never__
	optv->opterr = 1;
#else
	optv->opterr = 0;	/* In the shell, like getopt() to be quiet */
#endif
	optv->optind = 1;
	optv->optopt = 0;
	optv->opt_sp = 1;
	optv->optret = 0;
	optv->ooptind = 1;
	optv->optflag = 0;
	optv->optarg = 0;
}

/*
 * Wrapper around getopt() that helps to preserve the state of the
 * getopts(1) builtin. This only works with the AT&T getopt() extensions
 * introduced in 1989 for the Bourne Shell (the state variable _sp).
 */
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

	optv->ooptind = optind;
	ret = getopt(argc, (char **)argv, optstring);

	optv->opterr = opterr;
	optv->optind = optind;
	optv->optopt = optopt;
	optv->opt_sp = _sp;
	optv->optret = ret;
	optv->optarg = optarg;

	opterr = savopterr;
	optind = savoptind;
	optopt = savoptopt;
	_sp    = savopt_sp;
	optarg = savoptarg;

	return (ret);
}

#ifndef	NO_OPTNEXT
/*
 * Routine to complain about bad option arguments.
 */
void
optbad(argc, argv, optv)
	int		argc;
	unsigned char	**argv;
	struct optv	*optv;
{
	unsigned char	*p;
	unsigned char	opt[4];

	if (argc > optv->ooptind) {
		/*
		 * This may be a long option, so print the whole argument.
		 */
		p = locstak();
		*p = ' ';
		movstrstak(argv[optv->ooptind], &p[1]);
		p += stakbot - p;
	} else {
		opt[0] = ' '; opt[1] = '-',
		opt[2] = optv->optopt; opt[3] = '\0';
		p = opt;
	}
	if (optv->optret == ':')
		bfailure(argv[0], "option requires argument", p);
	else
		bfailure(argv[0], badopt, p);

	if ((optv->optflag & OPT_SPC) && !(flags & noexit))
		exitsh(ERROR);
}

/*
 * A variant of optget() that supports the -help option.
 * Make sure not to use 999 as a long only option identifier in optstring.
 * Use the null string ("") if no options besides -help are supported.
 *
 * Returns:
 *		-1	End of args
 *		0	-help or bad option
 *		> 0	Option character or option identifier
 */
int
optnext(argc, argv, optv, optstring, use)
	int	argc;
	unsigned char	**argv;
	struct optv	*optv;
	const	 char	*optstring;
	const	 char	*use;
{
	struct optv soptv;
	int c;

	soptv = *optv;
	if ((c = optget(argc, argv, optv, optstring)) != '?') {
		/* EMPTY */
		;
	} else if ((c = optget(argc, argv, &soptv, "()?999?(help)")) != '?') {
		*optv = soptv;
	}

	switch (c) {

	default:
		break;

	case ':':	/* Option requires argument	*/
	case '?':	/* Bad option			*/
		if (optv->optflag & OPT_NOFAIL)
			return (c);
		optbad(argc, argv, optv);
		/* FALLTHROUGH */

	case 999:
		if (use)
			gfailure((unsigned char *)usage, use);
		return (0);
	}
	return (c);
}

/*
 * This is the routine to enable POSIX untility syntax guidelines for builtins
 * that do not implement options. We always implement "-help" and complain if
 * any other (unknown) option was specified.
 *
 * Returns:
 *		-1	-help or bad option
 *		> 0	Arg index for next file type argument.
 */
int
optskip(argc, argv, use)
	int	argc;
	unsigned char	**argv;
	const	 char	*use;
{
	struct optv optv;
	int c;

	optinit(&optv);

	while ((c = optnext(argc, argv, &optv, nullstr, use)) != -1) {
		if (c == 0)
			return (-1);
	}
	return (optv.optind);
}
#endif
