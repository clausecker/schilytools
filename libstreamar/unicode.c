/* @(#)unicode.c	1.12 17/02/15 Copyright 2001-2017 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)unicode.c	1.12 17/02/15 Copyright 2001-2017 J. Schilling";
#endif
/*
 *	Routines to convert from/to UNICODE
 *
 *	This is currently a very simple implementation that only
 *	handles ISO-8859-1 coding. There should be a better solution
 *	in the future.
 *
 *	Copyright (c) 2001-2017 J. Schilling
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
#include <schily/utypes.h>
#include <schily/standard.h>
#include <schily/schily.h>

EXPORT	int	to_utf8		__PR((Uchar *to, Uchar *from));
EXPORT	int	to_utf8l	__PR((Uchar *to, Uchar *from, int len));
EXPORT	BOOL	from_utf8	__PR((Uchar *to, Uchar *from));
EXPORT	BOOL	from_utf8l	__PR((Uchar *to, Uchar *from, int *len));

EXPORT int
to_utf8(to, from)
	register Uchar	*to;
	register Uchar	*from;
{
	register Uchar	*oto = to;
	register Uchar	c;

	while ((c = *from++) != '\0') {
		if (c <= 0x7F) {
			*to++ = c;
		} else if (c <= 0xBF) {
			*to++ = 0xC2;
			*to++ = c;
		} else { /* c <= 0xFF */
			*to++ = 0xC3;
			*to++ = c & 0xBF;
		}
	}
	*to = '\0';
	return (to - oto);
}

EXPORT int
to_utf8l(to, from, len)
	register Uchar	*to;
	register Uchar	*from;
	register int	len;
{
	register Uchar	*oto = to;
	register Uchar	c;

	while (--len >= 0) {
		c = *from++;
		if (c <= 0x7F) {
			*to++ = c;
		} else if (c <= 0xBF) {
			*to++ = 0xC2;
			*to++ = c;
		} else { /* c <= 0xFF */
			*to++ = 0xC3;
			*to++ = c & 0xBF;
		}
	}
	*to = '\0';
	return (to - oto);
}

EXPORT BOOL
from_utf8(to, from)
	register Uchar	*to;
	register Uchar	*from;
{
	register Uchar	c;
	register BOOL	ret = TRUE;

	while ((c = *from++) != '\0') {
		if (c <= 0x7F) {
			*to++ = c;
		} else if (c == 0xC0) {
			*to++ = *from++ & 0x7F;
		} else if (c == 0xC1) {
			*to++ = (*from++ | 0x40) & 0x7F;
		} else if (c == 0xC2) {
			*to++ = *from++;
		} else if (c == 0xC3) {
			*to++ = *from++ | 0x40;
		} else {
			ret = FALSE;		/* unknown/illegal UTF-8 char */
			*to++ = '_';		/* use default character    */
			if (c < 0xE0) {
				from++;		/* 2 bytes in total */
			} else if (c < 0xF0) {
				from += 2;	/* 3 bytes in total */
			} else if (c < 0xF8) {
				from += 3;	/* 4 bytes in total */
			} else if (c < 0xFC) {
				from += 4;	/* 5 bytes in total */
			} else if (c < 0xFE) {
				from += 5;	/* 6 bytes in total */
			} else {
				while ((c = *from) != '\0') {
					/*
					 * Test for 7 bit ASCII + non prefix
					 */
					if (c <= 0xBF)
						break;
					from++;
				}
			}
		}
	}
	*to = '\0';
	return (ret);
}

EXPORT BOOL
from_utf8l(to, from, lenp)
	register Uchar	*to;
	register Uchar	*from;
		int	*lenp;
{
	register Uchar	*oto = to;
	register Uchar	c;
	register BOOL	ret = TRUE;
	register int	len = *lenp;

	while (--len >= 0) {
		c = *from++;
		if (c <= 0x7F) {
			*to++ = c;
		} else if (c == 0xC0) {
			*to++ = *from++ & 0x7F;
			len--;
		} else if (c == 0xC1) {
			*to++ = (*from++ | 0x40) & 0x7F;
			len--;
		} else if (c == 0xC2) {
			*to++ = *from++;
			len--;
		} else if (c == 0xC3) {
			*to++ = *from++ | 0x40;
			len--;
		} else {
			ret = FALSE;		/* unknown/illegal UTF-8 char */
			*to++ = '_';		/* use default character    */
			if (c < 0xE0) {
				from++;		/* 2 bytes in total */
				len--;
			} else if (c < 0xF0) {
				from += 2;	/* 3 bytes in total */
				len -= 2;
			} else if (c < 0xF8) {
				from += 3;	/* 4 bytes in total */
				len -= 3;
			} else if (c < 0xFC) {
				from += 4;	/* 5 bytes in total */
				len -= 4;
			} else if (c < 0xFE) {
				from += 5;	/* 6 bytes in total */
				len -= 5;
			} else {
				while (len > 0) {
					c = *from;
					/*
					 * Test for 7 bit ASCII + non prefix
					 */
					if (c <= 0xBF)
						break;
					from++;
					len--;
				}
			}
		}
	}
	*to = '\0';
	*lenp = (to - oto);
	return (ret);
}
