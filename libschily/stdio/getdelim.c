/* @(#)getdelim.c	1.1 15/05/09 Copyright 2015 J. Schilling */
/*
 *	Copyright (c) 2015 J. Schilling
 *
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

#include "schilyio.h"
#include <schily/stdlib.h>

#ifndef	HAVE_GETDELIM
EXPORT	ssize_t	getdelim	__PR((char **, size_t *, int, FILE *));

#define	DEF_LINE_SIZE	128

EXPORT ssize_t
getdelim(bufp, lenp, delim, f)
			char	**bufp;
			size_t	*lenp;
	register	int	delim;
	register	FILE	*f;
{
	register int	c;
	register char	*lp;
	register char	*ep;
	register size_t line_size;
	register char	*line;

	if (bufp == NULL || lenp == NULL) {
		seterrno(EINVAL);
		return (-1);
	}

	line_size = *lenp;
	line = *bufp;
	if (line == NULL || line_size == 0) {
		if (line_size == 0)
			line_size = DEF_LINE_SIZE;
		if (line == NULL)
			line = (char *) malloc(line_size);
		if (line == NULL)
			return (-1);
		*bufp = line;
		*lenp = line_size;
	}
	/* read until EOF or delim encountered */
	lp = line;
	ep = &line[line_size - 1];
	do {
		c = getc(f);
		if (c == EOF)
			break;
		*lp++ = c;
		if (lp >= ep) {
			ep = line;
			line_size += DEF_LINE_SIZE;
			line = (char *) realloc(line, line_size);
			if (line == NULL) {
				*lp = '\0';
				return (-1);
			}
			lp = line + (lp - ep);
			ep = &line[line_size - 1];
			*bufp = line;
			*lenp = line_size;
		}
	} while (c != delim);
	*lp = '\0';
	line_size = lp - line;
	if (line_size > SSIZE_T_MAX) {
#ifdef	EOVERFLOW
		seterrno(EOVERFLOW);
#else
		seterrno(EINVAL);
#endif
		return (-1);
	}
	return (line_size ? line_size : -1);
}
#endif	/* HAVE_GETDELIM */
