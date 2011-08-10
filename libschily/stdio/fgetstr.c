/* @(#)fgetstr.c	1.10 11/08/09 Copyright 1986, 1996-2011 J. Schilling */
/*
 *	Copyright (c) 1986, 1996-2011 J. Schilling
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

/*
 * XXX should we check if HAVE_USG_STDIO is defined and
 * XXX use something line memccpy to speed things up ???
 */

#if !defined(getc)
#include <schily/string.h>

EXPORT int
fgetstr(f, buf, len)
	register	FILE	*f;
			char	*buf;
	register	int	len;
{
	char	*bp = fgets(buf, len, f);

	if (bp) {
		return (strlen(bp));
	}
	buf[0] = '\0';
	return (-1);
}

#else

EXPORT int
fgetstr(f, buf, len)
	register	FILE	*f;
			char	*buf;
	register	int	len;
{
	register int	c	= '\0';
	register char	*bp	= buf;
	register int	nl	= '\n';

	down2(f, _IOREAD, _IORW);

	for (;;) {
		if ((c = getc(f)) < 0)
			break;
		if (--len > 0)
			*bp++ = (char)c;
		if (c == nl)
			break;
	}
	*bp = '\0';
	/*
	 * If buffer is empty and we hit EOF, return EOF
	 */
	if (c < 0 && bp == buf)
		return (c);

	return (bp - buf);
}

#endif

EXPORT int
getstr(buf, len)
	char	*buf;
	int	len;
{
	return (fgetstr(stdin, buf, len));
}
