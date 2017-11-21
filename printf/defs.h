/* @(#)defs.h	1.2 17/11/16 Copyright 2017 J. Schilling */
/*
 *	Definitions for using the printf Bourne Shell builtin as standalone
 *
 *	Copyright (c) 2017 J. Schilling
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

#include <schily/types.h>
#include <schily/utypes.h>
#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/locale.h>
#include <schily/nlsdefs.h>

#define	DO_SYSPRINTF

#define	C				(char *)
#define	UC				(unsigned char *)
#define	CP				(char **)
#define	UCP				(unsigned char **)
#define	nullstr				""
#define	ERROR				1
#define	usage				"Usage"
#define	exitval				0
#define	flushb()			fflush(stdout)
#define	prc_buff			putchar
#define	bprintf				printf

#ifndef	HAVE_REALLOC_NULL
#define	realloc(p, s)			___realloc(p, s, "string stack")
#endif

#define	bfailure(p1, p2, p3)		fprintf(stderr, "%s: %s%s\n", \
						p1, _(p2), p3)
#define	gfailure(p1, p2)		fprintf(stderr, "%s: %s\n", \
						_(C p1), _(p2))


extern int	main			__PR((int argc, char **argv));
extern int	sysprintf		__PR((int argc, unsigned char **argv));

extern unsigned char *escape_char	__PR((unsigned char *cp,
						unsigned char *res,
						int echomode));

extern int	optskip			__PR((int argc, unsigned char **argv,
						const char *use));

extern unsigned char *growstak		__PR((unsigned char *newtop));
extern unsigned char *movstrstak	__PR((unsigned char *a,
							unsigned char *b));

extern unsigned char	*stakbot;
extern unsigned char	*staktop;
extern unsigned char	*brkend;

#define	Rcheat(a)	((Intptr_t)(a))
#define	locstak()	stakbot
#define	relstak()	(staktop-stakbot)
#define	absstak(x)	(stakbot+Rcheat(x))
#define	pushstak(c)	(*staktop++ = (c))

#define	GROWSTAKTOP()	if (staktop >= brkend) \
				(void) growstak(staktop);

#define	round(a, b)	(((Intptr_t)(((char *)(a)+b)-1))&~((b)-1))
#define	BYTESPERWORD	(sizeof (char *))
