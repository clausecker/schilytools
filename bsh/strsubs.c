/* @(#)strsubs.c	1.28 18/04/20 Copyright 1985-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)strsubs.c	1.28 18/04/20 Copyright 1985-2018 J. Schilling";
#endif
/*
 *	Useful string functions
 *
 *	Copyright (c) 1985-2018 J. Schilling
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
#include <schily/varargs.h>
#include <schily/utypes.h>
#include "bsh.h"
#include "strsubs.h"
#include <schily/string.h>
#include <schily/stdlib.h>
#include "ctype.h"

EXPORT	char	*makestr	__PR((char *s));
EXPORT	char	*concat		__PR((char *, ...));
EXPORT	char	*concatv	__PR((char **s));
EXPORT	int	streql		__PR((const char *s1, const char *s2));
EXPORT	int	streqln		__PR((char *s1, char *s2, int n));
EXPORT	char	*strindex	__PR((char *x, char *y));
EXPORT	int	strbeg		__PR((char *x, char *y));
EXPORT	int	wordeql		__PR((char *s1, char *s2));
EXPORT	char	*quote_string	__PR((char *s, char *spec));
EXPORT	char	*pretty_string	__PR((unsigned char *s));
EXPORT	char	*fbasename	__PR((char *n));

EXPORT char *
makestr(s)
	register	char *s;
{
			char	*tmp;
	register	char	*s1;

	if ((tmp = malloc((size_t)strlen(s)+1)) == NULL) {
		raisecond("makestr", (long)NULL);
		return (0);
	}
	for (s1 = tmp; (*s1++ = *s++) != '\0'; );
	return (tmp);
}

EXPORT char *
concatv(s)
	char	**s;
{
	register	char	**argv;
			char	*ret;
	register	char	*op;
	register	char	*rp;
	register	int	i;
	register	size_t	len;
	register	int	argc;

	argv = s;
	for (argc = 0; argv[argc]; argc++)
		;

	for (i = 0, len = 0; i < argc; i++)
		len += strlen(argv[i]);

	if ((ret = rp = malloc(len + 1)) == NULL) {
		raisecond("concat", (long)NULL);
		return (NULL);
	}

	for (i = 0; i < argc; i++) {
		for (op = argv[i]; (*rp = *op++) != '\0'; rp++)
			;
	}
	*rp = '\0';
	return (ret);
}

#ifdef	lint
/* VARARGS1 */
EXPORT char *
concat(args)
	char	*args;
{
	return (args);
}
#else
#ifdef	PROTOTYPES
/* VARARGS1 */
EXPORT char *
concat(char *s, ...)
#else
/* VARARGS1 */
EXPORT char *
concat(s, va_alist)
	char	*s;
	va_dcl
#endif
{
	va_list	args;
	char	*ret;

	register int	i;
	register size_t	len;
	register int	argc;
	register char	*p;
	register char	*rp;

#ifdef	PROTOTYPES
	va_start(args, s);
#else
	va_start(args);
#endif
	p = s;
	for (argc = 0, len = 0; p != NULL; argc++, p = va_arg(args, char *)) {
		len += strlen(p);
	}
	va_end(args);

	if ((ret = rp = malloc(len + 1)) == NULL) {
		raisecond("concat", (long)NULL);
		return (NULL);
	}

#ifdef	PROTOTYPES
	va_start(args, s);
#else
	va_start(args);
#endif

	p = s;
	for (i = 0; i < argc; i++, p = va_arg(args, char *)) {
		for (; (*rp = *p++) != '\0'; rp++)
			;
	}
	*rp = '\0';
	va_end(args);
	return (ret);
}
#endif /* lint */

#ifdef	used
/* not longer used */
EXPORT char *
find(c, s)
	register char c;
	register char *s;
{
	do {
		if (*s == c)
			return (s);
	} while (*s++ != '\0');
	return (NULL);
}

EXPORT char *
rfind(c, s)
	register char	c;
	register char	*s;
{
	register char	*p = NULL;

	do {
		if (*s == c)
			p = s;
	} while (*s++ != '\0');
	return (p);
}
#endif

EXPORT BOOL
streql(s1, s2)
#ifdef	__STDC__
	const char	*s1;
	const char	*s2;
#else
		char	*s1;
		char	*s2;
#endif
{
	for (; *s1 == *s2; s1++, s2++)
		if (*s1 == '\0')
			return (TRUE);
	return (FALSE);
}

/*
 * Check if two strings equal to n chars
 */
EXPORT BOOL
streqln(s1, s2, n)
	register char	*s1;
	register char	*s2;
	register int	n;
{
	for (; n-- > 0; )
		if (*s1++ != *s2++)
			return (FALSE);
	return (TRUE);
}

EXPORT char *
strindex(x, y)
		char	*x;
	register char	*y;
{
	register char	*a;
	register char	*b;

	for (; *y != '\0'; y++) {
		for (a = x, b = y; *a == *b++; ) {
			if (*a++ == '\0')
				return (y);
		}
		if (*a == '\0')
			return (y);
	}
	return (NULL);
}

EXPORT BOOL
strbeg(x, y)
	char	*x;
	char	*y;
{
	return (strindex(x, y) == y);
}

EXPORT BOOL
wordeql(s1, s2)
	register char	*s1;
	register char	*s2;
{
	register int len = strlen(s2);

	if (strlen(s1) < len)
		return (FALSE);
	if (s1[len] && !strchr(" \t\n\377", s1[len]))
		return (FALSE);

	return (streqln(s1, s2, len));
}


EXPORT char *
quote_string(s, spec)
	register char	*s;
	register char	*spec;
{
	static	 char	buf[16];
	static	 char	*str = 0;
	register char	*s1  = 0;
	register int	len;

	if (str && str != buf)
		free(str);
	len = 2 * strlen(s) + 1;
	if (len > sizeof (buf))
		s1 = str = malloc(len);

	if (s1 == 0) {
		len = sizeof (buf);
		s1 = str = buf;
	}
	while (*s && --len > 0) {
		if (strchr(spec, *s)) {
			*s1++ = '\\';
			len--;
		}
		*s1++ = *s++;
	}
	*s1 = '\0';
	return (str);
}

EXPORT char *
pretty_string(s)
	register Uchar	*s;
{
	static	 Uchar	buf[16];
	static	 Uchar	*str = 0;
	register Uchar	*s1  = 0;
	register int	len;

	if (str && str != buf)
		free(str);

	len = 3 *(unsigned)strlen((char *)s) + 1;
	if (len > sizeof (buf))
		s1 = str = (Uchar *)malloc(len);

	if (s1 == 0) {
		len = sizeof (buf);
		s1 = str = buf;
	}
	while (*s && --len > 0) {
		if (isprint(*s)) {
			*s1++ = *s++;
			continue;
		}
		if (*s & 0x80) {
			*s1++ = '~';
			len--;
		}
		if (*s != 127 && *s & 0x60) {
			*s1++ = *s++ & 0x7F;
		} else {
			*s1++ = '^';
			*s1++ = (*s++ & 0x7F) ^ 0100;
			len--;
		}
	}
	*s1 = '\0';
	return ((char *)str);
}

EXPORT char *
fbasename(n)
	register char	*n;
{
	register char	*bn = n;

	while (*n)
		if (*n++ == '/')
			bn = n;
	return (bn);
}
