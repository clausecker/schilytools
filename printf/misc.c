/* @(#)misc.c	1.4 19/01/07 Copyright 2017-2019 J. Schilling */
#include <schily/mconfig.h>
/*
 *	Functions for using the printf Bourne Shell builtin as standalone
 *
 *	Copyright (c) 2017-2019 J. Schilling
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

static	UConst char sccsid[] =
	"@(#)misc.c	1.4 19/01/07 Copyright 2017-2019 J. Schilling";

#include "defs.h"

int	exitval;

unsigned char *
escape_char(cp, res, echomode)
	unsigned char	*cp;
	unsigned char	*res;
	int		echomode;	/* echo mode vs. C mode */
{
	int		j;
	int		wd;
	unsigned char	c;

	switch (*++cp) {
#if	defined(DO_SYSPRINTF) || defined(DO_ECHO_A)
	case 'a':	c = ALERT; break;
#endif
	case 'b':	c = '\b'; break;
	case 'c':	if (echomode)
				return (NULL);
			goto norm;
	case 'f':	c = '\f'; break;
	case 'n':	c = '\n'; break;
	case 'r':	c = '\r'; break;
	case 't':	c = '\t'; break;
	case 'v':	c = '\v'; break;
	case '\\':	c = '\\'; break;

	case '0':
		j = wd = 0;
		if (!echomode)		/* '\0123' must be '\n3' */
			j = 1;
	oct:
		while ((*++cp >= '0' &&
		    *cp <= '7') && j++ < 3) {
			wd <<= 3;
			wd |= (*cp - '0');
		}
		c = wd;
		--cp;
		break;

	case '1': case '2': case '3': case '4':
	case '5': case '6': case '7':
		if (!echomode) {
			j = 1;
			wd = (*cp - '0');
			goto oct;
		}
		/* FALLTHRU */
	default:
	norm:
		c = *--cp;
	}
	*res = c;
	return (cp);
}

#undef	BRKINCR
#define	BRKINCR 1024

unsigned brkincr = BRKINCR;

unsigned char	*stakbot;
unsigned char	*staktop;
unsigned char	*brkend;

/*
 * Grow stak so "newtop" becomes part of the valid stak.
 * Return new corrected address for "newtop".
 */
unsigned char *
growstak(newtop)
	unsigned char	*newtop;
{
	UIntptr_t	incr;
	UIntptr_t	newoff = newtop - stakbot;
	UIntptr_t	curoff = staktop - stakbot;
	int		staklen;
	unsigned char	*new;

	incr = (UIntptr_t)round(newtop - brkend + 1, BYTESPERWORD);
	if (brkincr > incr)
		incr = brkincr;		/* Grow at least by brkincr */

	staklen = brkend - stakbot;

	new = realloc(stakbot, staklen + incr);
	if (new) {
		staklen += incr;
		stakbot = new;
		staktop = stakbot + curoff;
		brkend = stakbot + staklen;
	} else {
		(void) fprintf(stderr, _("cannot allocate memory"));
		exit(1);
	}

	return (stakbot + newoff);	/* New value for newtop */
}

/*
 * Append the string in "a" to the string pointed to by "b".
 * "b" must be on the current local stack.
 * Return the address of the nul character at the end of the new string.
 *
 * The stack is kept growable.
 */
unsigned char *
movstrstak(a, b)
	unsigned char	*a;
	unsigned char	*b;
{
	do {
		if (b >= brkend)
			b = growstak(b);
	} while ((*b++ = *a++) != '\0');
	return (--b);
}

/*
 * This is the routine to enable POSIX untility syntax guidelines for builtins
 * that do not implement options.
 *
 * Returns:
 *		> 0	Arg index for next file type argument.
 */
int
optskip(argc, argv, use)
	int	argc;
	unsigned char	**argv;
	const	 char	*use;
{
	int	opt_ind = 1;

	if (argc > 1 &&
	    strcmp(C argv[1], "--") == 0) {	/* XCU4 "--" handling */
		opt_ind++;
		argv++;
	}
	return (opt_ind);
}
