/* @(#)fconv.c	1.36 06/10/29 Copyright 1985, 1995-2003 J. Schilling */
/*
 *	Convert floating point numbers to strings for format.c
 *	Should rather use the MT-safe routines [efg]convert()
 *
 *	Copyright (c) 1985, 1995-2003 J. Schilling
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

#include <schily/mconfig.h>	/* <- may define NO_FLOATINGPOINT */
#ifndef	NO_FLOATINGPOINT

#include <schily/stdlib.h>
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/schily.h>

#if	!defined(HAVE_STDLIB_H) || defined(HAVE_DTOA)
extern	char	*ecvt __PR((double, int, int *, int *));
extern	char	*fcvt __PR((double, int, int *, int *));
#endif

#if	defined(HAVE_ISNAN) && defined(HAVE_ISINF)
/*
 * *BSD alike libc
 */
#define	FOUND_ISXX
#endif

#include <math.h>

#if	defined(HAVE_C99_ISNAN) && defined(HAVE_C99_ISINF)
#define	FOUND_ISXX
#define	FOUND_C99_ISXX
#endif

#if	defined(HAVE_FP_H) && !defined(FOUND_ISXX)
/*
 * WAS:
 * #if	defined(__osf__) || defined(_IBMR2) || defined(_AIX)
 */
/*
 * Moved before HAVE_IEEEFP_H for True64 due to a hint
 * from Bert De Knuydt <Bert.Deknuydt@esat.kuleuven.ac.be>
 *
 * True64 has both fp.h & ieeefp.h but the functions
 * isnand() & finite() seem to be erreneously not implemented
 * as a macro and the function lives in libm.
 * Let's hope that we will not get problems with the new order.
 */
#include <fp.h>
#ifndef	isnan
#define	isnan	IS_NAN
#endif
#ifndef	isinf
#define	isinf	!FINITE
/*#define	isinf	IS_INF*/
#endif
#define	FOUND_ISXX
#endif

#if	defined(HAVE_IEEEFP_H) && !defined(FOUND_ISXX)
/*
 * SVR4
 */
#include <ieeefp.h>
#ifndef	isnan
#define	isnan	isnand
#endif
#ifndef	isinf
#define	isinf	!finite
#endif
#define	FOUND_ISXX
#endif

/*
 * WAS:
 * #if	defined(__hpux) || defined(VMS) || defined(_SCO_DS) || defined(__QNX__)
 */
#if	defined(__hpux) || defined(__QNX__) || defined(__DJGPP__)
#ifndef	FOUND_C99_ISXX
#undef	isnan
#undef	isinf
#endif
#endif

#if	!defined(isnan) && !defined(HAVE_ISNAN) && !defined(HAVE_C99_ISNAN)
#define	isnan(val)	(0)
#endif
#if	!defined(isinf) && !defined(HAVE_ISINF) && !defined(HAVE_C99_ISINF)
#define	isinf(val)	(0)
#endif

#if !defined(HAVE_ECVT) || !defined(HAVE_FCVT) || !defined(HAVE_GCVT)
#include "cvt.c"
#endif

static	char	_js_nan[] = "(NaN)";
static	char	_js_inf[] = "(Infinity)";

static	int	_ferr __PR((char *, double));

#ifdef	abs
#	undef	abs
#endif
#define	abs(i)	((i) < 0 ? -(i) : (i))

EXPORT int
ftoes(s, val, fieldwidth, ndigits)
	register	char 	*s;
			double	val;
	register	int	fieldwidth;
	register	int	ndigits;
{
	register	char	*b;
	register	char	*rs;
	register	int	len;
	register	int	rdecpt;
			int 	decpt;
			int	sign;

	if ((len = _ferr(s, val)) > 0)
		return (len);
	rs = s;
#ifdef	V7_FLOATSTYLE
	b = ecvt(val, ndigits, &decpt, &sign);
	rdecpt = decpt;
#else
	b = ecvt(val, ndigits+1, &decpt, &sign);
	rdecpt = decpt-1;
#endif
	len = ndigits + 6;			/* Punkt e +/- nnn */
	if (sign)
		len++;
	if (fieldwidth > len)
		while (fieldwidth-- > len)
			*rs++ = ' ';
	if (sign)
		*rs++ = '-';
#ifndef	V7_FLOATSTYLE
	if (*b)
		*rs++ = *b++;
#endif
	*rs++ = '.';
	while (*b && ndigits-- > 0)
		*rs++ = *b++;
	*rs++ = 'e';
	*rs++ = rdecpt >= 0 ? '+' : '-';
	rdecpt = abs(rdecpt);
#ifndef	V7_FLOATSTYLE
	if (rdecpt >= 100)
#endif
	{
		*rs++ = rdecpt / 100 + '0';
		rdecpt %= 100;
	}
	*rs++ = rdecpt / 10 + '0';
	*rs++ = rdecpt % 10 + '0';
	*rs = '\0';
	return (rs - s);
}

/*
 * fcvt() from Cygwin32 is buggy.
 */
#if	!defined(HAVE_FCVT) && defined(HAVE_ECVT)
#define	USE_ECVT
#endif

EXPORT int
ftofs(s, val, fieldwidth, ndigits)
	register	char 	*s;
			double	val;
	register	int	fieldwidth;
	register	int	ndigits;
{
	register	char	*b;
	register	char	*rs;
	register	int	len;
	register	int	rdecpt;
			int 	decpt;
			int	sign;

	if ((len = _ferr(s, val)) > 0)
		return (len);
	rs = s;
#ifdef	USE_ECVT
	/*
	 * Needed on systems with broken fcvt() implementation
	 * (e.g. Cygwin32)
	 */
	b = ecvt(val, ndigits, &decpt, &sign);
	/*
	 * The next call is needed to force higher precision.
	 */
	if (decpt > 0)
		b = ecvt(val, ndigits+decpt, &decpt, &sign);
#else
	b = fcvt(val, ndigits, &decpt, &sign);
#endif
	rdecpt = decpt;
	len = rdecpt + ndigits + 1;
	if (rdecpt < 0)
		len -= rdecpt;
	if (sign)
		len++;
	if (fieldwidth > len)
		while (fieldwidth-- > len)
			*rs++ = ' ';
	if (sign)
		*rs++ = '-';
	if (rdecpt > 0) {
		len = rdecpt;
		while (*b && len-- > 0)
			*rs++ = *b++;
#ifdef	USE_ECVT
		while (len-- > 0)
			*rs++ = '0';
#endif
	}
#ifndef	V7_FLOATSTYLE
	else {
		*rs++ = '0';
	}
#endif
	*rs++ = '.';
	if (rdecpt < 0) {
		len = rdecpt;
		while (len++ < 0 && ndigits-- > 0)
			*rs++ = '0';
	}
	while (*b && ndigits-- > 0)
		*rs++ = *b++;
#ifdef	USE_ECVT
	while (ndigits-- > 0)
		*rs++ = '0';
#endif
	*rs = '\0';
	return (rs - s);
}

LOCAL int
_ferr(s, val)
	char	*s;
	double	val;
{
	if (isnan(val)) {
		strcpy(s, _js_nan);
		return (sizeof (_js_nan) - 1);
	}

	/*
	 * Check first for NaN because finite() will return 1 on Nan too.
	 */
	if (isinf(val)) {
		strcpy(s, _js_inf);
		return (sizeof (_js_inf) - 1);
	}
	return (0);
}
#endif	/* NO_FLOATINGPOINT */
