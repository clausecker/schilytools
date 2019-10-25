/* @(#)wcastoi.c	1.1 19/10/22 Copyright 1985, 1995-2019 J. Schilling */
/*
 *	wcastoi() converts a string to int
 *	wcastol() converts a string to long
 *
 *	Leading tabs and spaces are ignored.
 *	Both return pointer to the first char that has not been used.
 *	Caller must check if this means a bad conversion.
 *
 *	leading "+" is ignored
 *	leading "0"  makes conversion octal (base 8)
 *	leading "0x" makes conversion hex   (base 16)
 *
 *	Copyright (c) 1985, 1995-2019 J. Schilling
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

#include <schily/standard.h>
#include <schily/schily.h>
#include <schily/errno.h>
#include <schily/wchar.h>

#define	is_space(c)	 ((c) == L' ' || (c) == L'\t')
#define	is_digit(c)	 ((c) >= L'0' && (c) <= L'9')
#define	is_hex(c)	(\
			((c) >= L'a' && (c) <= L'f') || \
			((c) >= L'A' && (c) <= L'F'))

#define	is_lower(c)	((c) >= L'a' && (c) <= L'z')
#define	is_upper(c)	((c) >= L'A' && (c) <= L'Z')
#define	to_lower(c)	(((c) >= L'A' && (c) <= L'Z') ? (c) - L'A'+L'a' : (c))

#if	(L'i' + 1) < L'j'
#define	BASE_MAX	(L'i' - L'a' + 10 + 1)	/* This is EBCDIC */
#else
#define	BASE_MAX	(L'z' - L'a' + 10 + 1)	/* This is ASCII */
#endif

#ifndef	HAVE_WCSTOL
EXPORT	long	wcstol		__PR((const wchar_t *nptr,
					wchar_t **endptr,
					int base));
#endif
EXPORT	wchar_t	*wcastoi	__PR((const wchar_t *s, int *l));
EXPORT	wchar_t	*wcastol	__PR((const wchar_t *s, long *l));
EXPORT	wchar_t	*wcastolb	__PR((const wchar_t *s, long *l, int base));

#ifndef	HAVE_WCSTOL
EXPORT long
wcstol(nptr, endptr, base)
	const wchar_t	*nptr;
	wchar_t		**endptr;
	int		base;
{
	long	l;
	wchar_t	*ep;

	ep = wcastolb(nptr, &l, base);
	if (endptr)
		*endptr = ep;
	return (l);
}
#endif	/* HAVE_WCSTOL */

EXPORT wchar_t *
wcastoi(s, i)
	const wchar_t *s;
	int *i;
{
	long	l;
	wchar_t	*ret;

	ret = wcastol(s, &l);
	*i = l;
#if	SIZEOF_INT != SIZEOF_LONG_INT
	if (*i != l) {
		if (l < 0) {
			*i = TYPE_MINVAL(int);
		} else {
			*i = TYPE_MAXVAL(int);
		}
		seterrno(ERANGE);
	}
#endif
	return (ret);
}

EXPORT wchar_t *
wcastol(s, l)
	register const wchar_t *s;
	long *l;
{
	return (wcastolb(s, l, 0));
}

EXPORT wchar_t *
wcastolb(s, l, base)
	register const wchar_t *s;
	long *l;
	register int base;
{
	int neg = 0;
	register unsigned long ret = 0L;
		unsigned long maxmult;
		unsigned long maxval;
	register int digit;
	register wchar_t c;

	if (base > BASE_MAX || base == 1 || base < 0) {
		seterrno(EINVAL);
		return ((wchar_t *)s);
	}

	while (is_space(*s))
		s++;

	if (*s == L'+') {
		s++;
	} else if (*s == L'-') {
		s++;
		neg++;
	}

	if (base == 0) {
		if (*s == L'0') {
			base = 8;
			s++;
			if (*s == L'x' || *s == L'X') {
				s++;
				base = 16;
			}
		} else {
			base = 10;
		}
	}
	if (neg) {
		/*
		 * Portable way to compute the positive value of "min-long"
		 * as -TYPE_MINVAL(long) does not work.
		 */
		maxval = ((unsigned long)(-1 * (TYPE_MINVAL(long)+1))) + 1;
	} else {
		maxval = TYPE_MAXVAL(long);
	}
	maxmult = maxval / base;
	for (; (c = *s) != 0; s++) {

		if (is_digit(c)) {
			digit = c - L'0';
		} else if (is_lower(c)) {
			digit = c - L'a' + 10;
		} else if (is_upper(c)) {
			digit = c - L'A' + 10;
		} else {
			break;
		}

		if (digit < base) {
			if (ret > maxmult)
				goto overflow;
			ret *= base;
			if (maxval - ret < digit)
				goto overflow;
			ret += digit;
		} else {
			break;
		}
	}
	if (neg) {
		*l = (Llong)-1 * ret;
	} else {
		*l = (Llong)ret;
	}
	return ((wchar_t *)s);
overflow:
	for (; (c = *s) != 0; s++) {

		if (is_digit(c)) {
			digit = c - L'0';
		} else if (is_lower(c)) {
			digit = c - L'a' + 10;
		} else if (is_upper(c)) {
			digit = c - L'A' + 10;
		} else {
			break;
		}
		if (digit >= base)
			break;
	}
	if (neg) {
		*l = TYPE_MINVAL(long);
	} else {
		*l = TYPE_MAXVAL(long);
	}
	seterrno(ERANGE);
	return ((wchar_t *)s);
}
