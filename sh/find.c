/* @(#)find.c	1.3 17/11/02 Copyright 2014-2017 J. Schilling */
#include <schily/mconfig.h>
/*
 *	find builtin
 *
 *	Copyright (c) 2014-2017 J. Schilling
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
#ifdef DO_SYSFIND

static	UConst char sccsid[] =
	"@(#)find.c	1.3 17/11/02 Copyright 2014-2017 J. Schilling";

#include	<schily/walk.h>
#include	<schily/find.h>


#define	LOCAL	static

LOCAL	int	quitfun	__PR((void *arg));

LOCAL int
quitfun(arg)
	void	*arg;
{
	return (*((int *)arg) != 0);
}

void
sysfind(argc, argv)
	int	argc;
	unsigned char	**argv;
{
	FILE		*std[3];
	squit_t		quit;
	unsigned char	**xecenv;

	std[0] = stdin;
	std[1] = stdout;
	std[2] = stderr;

	quit.quitfun = quitfun;
	quit.qfarg = &intrcnt;

	xecenv = local_setenv(ENV_NOFREE);
	exitval = find_main(argc, (char **)argv, (char **)xecenv, std, &quit);

	fflush(stdin);
	fflush(stdout);
	fflush(stderr);
}

/*
 * find_main() disables file raising for stdin/stdout/stderr, but libschily
 * would still include references (not calls) to raisecond(). Provide a dummy
 * to avoid linking against raisecond() from libschily.
 */
void
raisecond(n, a)
	const	char	*n;
	long		a;
{
}

#endif /* DO_SYSFIND */
