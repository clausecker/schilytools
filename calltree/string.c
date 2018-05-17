/* @(#)string.c	1.17 09/07/11 Copyright 1985, 1999-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)string.c	1.17 09/07/11 Copyright 1985, 1999-2009 J. Schilling";
#endif
/*
 *	A program to produce a static calltree for C-functions
 *
 *	string handling and allocation
 *
 *	Copyright (c) 1985, 1999-2009 J. Schilling
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

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/varargs.h>
#include <schily/schily.h>
#include "strsubs.h"

EXPORT	char	*xmalloc	__PR((unsigned int amt, char *txt));
EXPORT	char	*concat		__PR((char *first, ...));

EXPORT char *
xmalloc(amt, txt)
	unsigned int	amt;
	char		*txt;
{
	char	*ret;

	if ((ret = (char *) malloc(amt)) == 0)
		comerr("Can't alloc %d bytes for %s.\n", amt, txt);
	return (ret);
}

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT char *
concat(char *first, ...)
#else
EXPORT char *
concat(first, va_alist)
	char	*first;
	va_dcl
#endif
{
	va_list		args;
	char		*ret;
	register int	i;
	register unsigned len;
	register int	nstr;
	register char	*p1;
	register char	*p2;

#ifdef	PROTOTYPES
	va_start(args, first);
#else
	va_start(args);
#endif
	p1 = first;
	for (nstr = 0, len = 0; p1 != NULL; nstr++, p1 = va_arg(args, char *)) {
		len += strlen(p1);
	}
	va_end(args);

	p2 = ret = xmalloc(len + 1, "string concat");

#ifdef	PROTOTYPES
	va_start(args, first);
#else
	va_start(args);
#endif
	p1 = first;
	for (i = 0; i < nstr; i++) {
		for (; (*p2 = *p1++) != '\0'; p2++);
		p1 = va_arg(args, char *);
	}
	*p2 = '\0';
	va_end(args);
	return (ret);
}
