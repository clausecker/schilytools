/* @(#)find.c	1.7 18/08/01 Copyright 2014-2018 J. Schilling */
#include <schily/mconfig.h>
/*
 *	find builtin
 *
 *	Copyright (c) 2014-2018 J. Schilling
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
	"@(#)find.c	1.7 18/08/01 Copyright 2014-2018 J. Schilling";

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

int
sysfind(argc, argv, boshp)
	int	argc;
	unsigned char	**argv;
	bosh_t	*boshp;
{
	FILE		*std[3];
	squit_t		quit;
	unsigned char	**xecenv;
	int		exval;
	unsigned long	oflags2 = *boshp->flagsp2;

	std[0] = stdin;
	std[1] = stdout;
	std[2] = stderr;

	find_sqinit(&quit);
	quit.quitfun = quitfun;
	quit.qfarg = &boshp->intrcnt;
	quit.flags = SQ_CALL;
	quit.callfun = boshp->callsh;

	xecenv = boshp->get_envptr();
	*boshp->flagsp2 &= ~timeflg;
	exval = find_main(argc, (char **)argv, (char **)xecenv, std, &quit);
	*boshp->flagsp2 = oflags2;

	fflush(stdin);
	fflush(stdout);
	fflush(stderr);

	return (exval);
}

/*
 * find_main() disables file raising for stdin/stdout/stderr, but libschily
 * would still include references (but not real calls) to raisecond(). Provide
 * a dummy to avoid linking against raisecond() from a static libschily.
 */
void
raisecond(n, a)
	const	char	*n;
	long		a;
{
}

#endif /* DO_SYSFIND */
