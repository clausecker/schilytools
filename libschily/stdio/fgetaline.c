/* @(#)fgetaline.c	1.1 11/10/26 Copyright 2011 J. Schilling */
/*
 *	Copyright (c) 2011 J. Schilling
 *
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

#include "schilyio.h"
#include <schily/stdlib.h>
#include <schily/string.h>

EXPORT	ssize_t	fgetaline	__PR((FILE *, char **, size_t *));
EXPORT 	ssize_t	getaline	__PR((char **, size_t *));

#define	DEF_LINE_SIZE	128

EXPORT ssize_t
fgetaline(f, bufp, lenp)
	register	FILE	*f;
			char	**bufp;
	register	size_t	*lenp;
{
#ifdef	HAVE_GETDELIM
	return (getdelim(bufp, lenp, '\n', f));
#else
	int	eof;
	register size_t used = 0;
	register size_t line_size;
	register char	*line;

	if (bufp == NULL || lenp == NULL) {
		seterrno(EINVAL);
		return (-1);
	} 

	line_size = *lenp;
	line = *bufp;
	if (line == NULL || line_size == 0) {
		line_size = DEF_LINE_SIZE;
		line = (char *) malloc(line_size);
		if (line == NULL)
			return (-1);
	}
	/* read until EOF or newline encountered */
	line[0] = '\0';
	do {
		line[line_size - 1] = '\t';	/* arbitrary non-zero char */
		line[line_size - 2] = ' ';	/* arbitrary non-newline char */
		if (!(eof = (fgets(line+used,
				    line_size-used,
				    f) == NULL))) {

			if (line[line_size - 1] != '\0' ||
			    line[line_size - 2] == '\n')
				break;

			used = line_size - 1;

			line_size += DEF_LINE_SIZE;
			line = (char *) realloc(line, line_size);
			if (line == NULL)
				return (-1);
		}
	} while (!eof);
	used += strlen(&line[used]);
	*bufp = line;
	*lenp = line_size;
	if (eof && (used == 0))
		return (-1);
	return (used);
#endif
}

EXPORT ssize_t
getaline(bufp, lenp)
	char	**bufp;
	size_t	*lenp;
{
	return (fgetaline(stdin, bufp, lenp));
}
