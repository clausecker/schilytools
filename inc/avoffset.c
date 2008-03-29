/* @(#)avoffset.c	1.28 08/03/18 Copyright 1987, 1995-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)avoffset.c	1.28 08/03/18 Copyright 1987, 1995-2004 J. Schilling";
#endif
/*
 * This program is a tool to generate the file "avoffset.h".
 * It is used by functions that trace the stack to get to the top of the stack.
 *
 * It generates two defines:
 *	AV_OFFSET	- offset of argv relative to the main() frame pointer
 *	FP_INDIR	- number of stack frames above main()
 *			  before encountering a NULL pointer.
 *
 *	Copyright (c) 1987, 1995-2004 J. Schilling
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
#include <schily/schily.h>
#include <schily/stdlib.h>
#include <signal.h>

#ifdef	HAVE_SCANSTACK
#	include <schily/stkframe.h>
#endif

LOCAL	RETSIGTYPE	handler 	__PR((int signo));
EXPORT	int		main		__PR((int ac, char **av));
LOCAL	int		stack_direction	__PR((long *lp));

LOCAL	RETSIGTYPE
handler(signo)
	int	signo;
{
	fprintf(stderr, "Warning: Cannot scan stack on this environment.\n");
	exit(0);
}


int
main(ac, av)
	int	ac;
	char	**av;
{
	int		stdir;
#ifdef	HAVE_SCANSTACK
	register struct frame *fp;
	register int	i = 0;
	register int	o = 0;

	/*
	 * As the SCO OpenServer C-Compiler has a bug that may cause
	 * the first function call to getfp() been done before the
	 * new stack frame is created, we call getfp() twice.
	 */
	(void) getfp();
	fp = (struct frame *)getfp();
#endif

#ifdef	SIGBUS
	signal(SIGBUS, handler);
#endif
	signal(SIGSEGV, handler);
#ifdef	SIGILL
	signal(SIGILL, handler);	/* For gcc -m64/sparc on FreeBSD */
#endif

	printf("/*\n");
	printf(" * This file has been generated automatically\n");
	printf(" * by %s\n", sccsid);
	printf(" * do not edit by hand.\n");
	printf(" *\n");
	printf(" * This file includes definitions for AV_OFFSET and FP_INDIR.\n");
	printf(" * FP_INDIR is the number of fp chain elements above 'main'.\n");
	printf(" * AV_OFFSET is the offset of &av[0] relative to the frame pointer in 'main'.\n");
	printf(" *\n");
	printf(" * If getav0() does not work on a specific architecture\n");
	printf(" * the program which generated this include file may dump core.\n");
	printf(" * In this case, the generated include file does not include\n");
	printf(" * definitions for AV_OFFSET and FP_INDIR but ends after this comment.\n");
	printf(" * If AV_OFFSET or FP_INDIR are missing in this file, all programs\n");
	printf(" * which use the definitions are automatically disabled.\n");
	printf(" */\n");
	stdir = stack_direction(0);
	printf("#define	STACK_DIRECTION	%d\n", stdir);
	fflush(stdout);

#ifdef	HAVE_SCANSTACK
	/*
	 * Note: Scanning the stack to look for argc/argv
	 *	 works only in the main thread.
	 */
	while (fp->fr_savfp) {
		if (fp->fr_savpc == 0)
			break;

		fp = (struct frame *)fp->fr_savfp;

		i++;
	}
	/*
	 * Do not add any printf()'s before this line to allow avoffset
	 * to abort without printing more than the comment above.
	 */
	fp = (struct frame *)getfp();
	o = ((char *)av) - ((char *)fp);
	if ((o % sizeof (char *)) != 0) {
		fprintf(stderr, "AV_OFFSET value (%d) not a multiple of pointer size.\n", o);
		fprintf(stderr, "Disabling scanning the stack.\n");
		exit(0);
	}
	if (o < -1000 || o > 1000) {
		fprintf(stderr, "AV_OFFSET value (%d) does not look reasonable.\n", o);
		fprintf(stderr, "Disabling scanning the stack.\n");
		exit(0);
	}
	printf("#define	AV_OFFSET	%d\n", o);
	printf("#define	FP_INDIR	%d\n", i);
#endif
	exit(0);
	return (0);	/* keep lint happy */
}

LOCAL int
stack_direction(lp)
	long	*lp;
{
	auto long	*dummy[4];
	int		i;

	for (i = 0; i < 4; i++)
		dummy[i] = lp;

	if (lp == 0) {
		return (stack_direction((long *)dummy));
	} else {
		if ((long *)dummy == lp)
			return (0);
		return (((long *)dummy > lp) ? 1 : -1);
	}
}

#ifdef	HAVE_SCANSTACK
#define	IS_AVOFFSET
#include "getfp.c"
#endif
