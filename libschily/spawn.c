/* @(#)spawn.c	1.22 09/01/05 Copyright 1985, 1989, 1995-2008 J. Schilling */
/*
 *	Spawn another process/ wait for child process
 *
 *	Copyright (c) 1985, 1989, 1995-2008 J. Schilling
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
#include <stdio.h>
#include <schily/standard.h>
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/varargs.h>
#include <schily/wait.h>
#include <schily/errno.h>
#include <schily/schily.h>

#define	MAX_F_ARGS	16

EXPORT int
fspawnv(in, out, err, argc, argv)
	FILE	*in;
	FILE	*out;
	FILE	*err;
	int	argc;
	char	* const argv[];
{
	int	pid;

	if ((pid = fspawnv_nowait(in, out, err, argv[0], argc, argv)) < 0)
		return (pid);

	return (wait_chld(pid));
}

/* VARARGS3 */
#ifdef	PROTOTYPES
EXPORT int
fspawnl(FILE *in, FILE *out, FILE *err, const char *arg0, ...)
#else
EXPORT int
fspawnl(in, out, err, arg0, va_alist)
	FILE		*in;
	FILE		*out;
	FILE		*err;
	const char	*arg0;
	va_dcl
#endif
{
	va_list	args;
	int	ac = 0;
	char	*xav[MAX_F_ARGS+1];
	char	**av;
const	char	**pav;
	char	*p;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, arg0);
#else
	va_start(args);
#endif
	if (arg0) {
		ac++;
		while (va_arg(args, char *) != NULL)
			ac++;
	}
	va_end(args);

	if (ac <= MAX_F_ARGS) {
		av = xav;
	} else {
		av = (char **)malloc((ac+1)*sizeof (char *));
		if (av == 0)
			return (-1);
	}
	pav = (const char **)av;

#ifdef	PROTOTYPES
	va_start(args, arg0);
#else
	va_start(args);
#endif
	*pav++ = arg0;
	if (arg0) do {
		p = va_arg(args, char *);
		*pav++ = p;
	} while (p != NULL);
	va_end(args);

	ret =  fspawnv(in, out, err, ac, av);
	if (av != xav)
		free(av);
	return (ret);
}

EXPORT int
fspawnv_nowait(in, out, err, name, argc, argv)
	FILE		*in;
	FILE		*out;
	FILE		*err;
	const char	*name;
	int		argc;
	char		* const argv[];
{
	int	pid = -1;	/* Initialization needed to make GCC happy */
	int	i;

	for (i = 1; i < 64; i *= 2) {
		if ((pid = fork()) >= 0)
			break;
		sleep(i);
	}
	if (pid != 0)
		return (pid);
				/*
				 * silly: fexecv must set av[ac] = NULL
				 * so we have to cast argv tp (char **)
				 */
	fexecv(name, in, out, err, argc, (char **)argv);
	exit(geterrno());
	/* NOTREACHED */
#ifndef	lint
	return (0);		/* keep gnu compiler happy */
#endif
}

EXPORT int
wait_chld(pid)
	int	pid;
{
	int	died;
	WAIT_T	status;

	do {
		do {
			died = wait(&status);
		} while (died < 0 && geterrno() == EINTR);
		if (died < 0)
			return (died);
	} while (died != pid);

	if (WCOREDUMP(status))
		unlink("core");

	return (WEXITSTATUS(status));
}
