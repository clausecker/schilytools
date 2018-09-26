/* @(#)fconv.c	1.59 18/09/10 Copyright 1985, 1995-2018 J. Schilling */
/*
 *	Convert floating point numbers to strings for format.c
 *	Should rather use the MT-safe routines [efg]convert()
 *
 *	Copyright (c) 1985, 1995-2018 J. Schilling
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

#include <schily/mconfig.h>	/* <- may define NO_FLOATINGPOINT */
#ifndef	NO_FLOATINGPOINT

#ifndef	__DO_LONG_DOUBLE__
#include <schily/stdlib.h>
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/math.h>	/* The default place for isinf()/isnan() */
#include <schily/nlsdefs.h>
#include <schily/ctype.h>
#include "format.h"

#if	!defined(HAVE_STDLIB_H) || defined(HAVE_DTOA)
extern	char	*ecvt __PR((double, int, int *, int *));
extern	char	*fcvt __PR((double, int, int *, int *));
#endif

#if	defined(HAVE_ISNAN) && defined(HAVE_ISINF)
/*
 * *BSD alike libc
 */
#define	FOUND_ISNAN
#define	FOUND_ISINF
#define	FOUND_ISXX
#endif

#if	defined(HAVE_C99_ISNAN) && defined(HAVE_C99_ISINF)
#ifndef	FOUND_ISXX
#define	FOUND_ISXX
#endif
#define	FOUND_C99_ISNAN
#define	FOUND_C99_ISINF
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
#if	!defined(isnan) && defined(IS_NAN)
#define	isnan	IS_NAN
#define	FOUND_ISNAN
#endif
#if	!defined(isinf) && defined(FINITE)
#define	isinf	!FINITE
#define	FOUND_ISINF
#endif
#if	defined(FOUND_ISNAN) && defined(FOUND_ISINF)
#define	FOUND_ISXX
#endif
#endif

#if	defined(HAVE_IEEEFP_H) && !defined(FOUND_ISXX) && \
	!defined(FOUND_C99_ISXX)
/*
 * SVR4
 */
#include <ieeefp.h>
#ifdef	HAVE_ISNAND
#ifndef	isnan
#define	isnan	isnand
#define	FOUND_ISNAN
#endif
#endif
#ifdef	HAVE_FINITE
#ifndef	isinf
#define	isinf	!finite
#define	FOUND_ISINF
#endif
#endif
#if	defined(FOUND_ISNAN) && defined(FOUND_ISINF)
#define	FOUND_ISXX
#endif
#endif

/*
 * WAS:
 * #if	defined(__hpux) || defined(VMS) || defined(_SCO_DS) || defined(__QNX__)
 */
#ifdef	__nneded__
#if	defined(__hpux) || defined(__QNX__) || defined(__DJGPP__)
#ifndef	FOUND_C99_ISXX
#undef	isnan
#undef	isinf
#endif
#endif
#endif	/* __needed__ */

/*
 * As we no longer check for defined(isnan)/defined(isinf), the next block
 * should also handle the problems with DJGPP, HP-UX, QNX and VMS.
 */
#if	!defined(FOUND_ISNAN) && !defined(HAVE_C99_ISNAN)
#undef	isnan
#define	isnan(val)	(0)
#define	NO_ISNAN
#endif
#if	!defined(FOUND_ISINF) && !defined(HAVE_C99_ISINF)
#undef	isinf
#define	isinf(val)	(0)
#define	NO_ISINF
#endif

#if	defined(NO_ISNAN) || defined(NO_ISINF)
#include <schily/float.h>	/* For values.h */
#if	(_IEEE - 0) > 0		/* We know that there is IEEE FP */
/*
 * Note that older HP-UX versions have different #defines for MAXINT in
 * values.h and sys/param.h
 */
#include <schily/utypes.h>
#include <schily/btorder.h>

#ifdef	WORDS_BIGENDIAN
#define	fpw_high(x)	((UInt32_t *)&x)[0]
#define	fpw_low(x)	((UInt32_t *)&x)[1]
#else
#define	fpw_high(x)	((UInt32_t *)&x)[1]
#define	fpw_low(x)	((UInt32_t *)&x)[0]
#endif
#define	FP_EXP		0x7FF00000
#define	fp_exp(x)	(fpw_high(x) & FP_EXP)
#define	fp_exc(x)	(fp_exp(x) == FP_EXP)

#ifdef	NO_ISNAN
#undef	isnan
#define	isnan(val)	(fp_exc(val) && \
			(fpw_low(val) != 0 || (fpw_high(val) & 0xFFFFF) != 0))
#endif
#ifdef	NO_ISINF
#undef	isinf
#define	isinf(val)	(fp_exc(val) && \
			fpw_low(val) == 0 && (fpw_high(val) & 0xFFFFF) == 0)
#endif
#endif	/* We know that there is IEEE FP */
#endif	/* defined(NO_ISNAN) || defined(NO_ISINF) */


#if !defined(HAVE_ECVT) || !defined(HAVE_FCVT) || !defined(HAVE_GCVT)

#ifdef	NO_USER_XCVT
	/*
	 * We cannot define our own ecvt()/fcvt()/gcvt() so we need to use
	 * local names instead.
	 */
#ifndef	HAVE_ECVT
#define	ecvt	js_ecvt
#endif
#ifndef	HAVE_FCVT
#define	fcvt	js_fcvt
#endif
#ifndef	HAVE_GCVT
#define	gcvt	js_gcvt
#endif
#endif

#include "cvt.c"
#endif

#ifdef	JOS_COMPAT
static	char	_js_nan[] = "(NaN)";
static	char	_js_inf[] = "(Infinity)";
#else
static	char	_js_nan[] = "nan";
static	char	_js_inf[] = "inf";
#endif

static	int	_ferr __PR((char *, double, int));

static	int	_ecv __PR((char *s, char *b, int fieldwidth, int ndigits,
				int decpt, int sign, int flags));
static	int	_fcv __PR((char *s, char *b, int fieldwidth, int ndigits,
				int decpt, int sign, int flags));

#endif	/* __DO_LONG_DOUBLE__ */

#ifdef	__DO_LONG_DOUBLE__
#undef	MDOUBLE
#define	MDOUBLE	long double
#else
#undef	MDOUBLE
#define	MDOUBLE	double
#endif

#ifdef	abs
#undef	abs
#endif
#define	abs(i)	((i) < 0 ? -(i) : (i))

EXPORT int
ftoes(s, val, fieldwidth, ndigits)
	register	char 	*s;
			MDOUBLE	val;
	register	int	fieldwidth;
	register	int	ndigits;
{
	int	flags = 0;

	if (ndigits < 0) {
		ndigits = -ndigits;
		flags |= UPPERFLG;
	}
	return (_ftoes(s, val, fieldwidth, ndigits, flags));
}

EXPORT int
_ftoes(s, val, fieldwidth, ndigits, flags)
	register	char 	*s;
			MDOUBLE	val;
	register	int	fieldwidth;
	register	int	ndigits;
			int	flags;
{
	register	char	*b;
	register	int	len;
	register	int	rdecpt;
			int 	decpt;
			int	sign;
#ifdef	__DO_LONG_DOUBLE__
			int	Efmt = FALSE;

	if (flags & UPPERFLG)
		Efmt = TRUE;
#endif
#ifndef	__DO_LONG_DOUBLE__
	if ((len = _ferr(s, val, flags)) > 0)
		return (len);
#endif
#ifdef	V7_FLOATSTYLE
	b = ecvt(val, ndigits, &decpt, &sign);
	rdecpt = decpt;
#else
	b = ecvt(val, ndigits+1, &decpt, &sign);
	rdecpt = decpt-1;
#endif
#ifdef	__DO_LONG_DOUBLE__
	len = *b;
	if (len == '-' || len == '+')
		len = b[1];
	if (len < '0' || len > '9') {		/* Inf/NaN */
		char	*p;

		for (p = b; *p; p++)
			*s++ = Efmt ? toupper(*p) : *p;
		*s = '\0';
		return (strlen(b));
	}
#endif
	return (_ecv(s, b, fieldwidth, ndigits, rdecpt, sign, flags));
}

#ifndef	__DO_LONG_DOUBLE__
int
_ecv(s, b, fieldwidth, ndigits, rdecpt, sign, flags)
	register char	*s;
	register char	*b;
	register int	fieldwidth;
	register int	ndigits;
	register int	rdecpt;
	int	sign;
	int	flags;
{
	register int	len;
	register char	*rs = s;
		char	dpoint;
		char	*pp;

#if defined(HAVE_LOCALECONV) && defined(USE_LOCALE)
	dpoint = *(localeconv()->decimal_point);
#else
	dpoint = '.';
#endif
	len = ndigits + 6;			/* Punkt e +/- nnn */
	if (sign)
		len++;
	else if (flags & PLUSFLG)
		len++;
	else if (flags & SPACEFLG)
		len++;
	if ((flags & PADZERO) == 0 && fieldwidth > len) {
		while (fieldwidth-- > len)
			*rs++ = ' ';
	}
	if (sign)
		*rs++ = '-';
	else if (flags & PLUSFLG)
		*rs++ = '+';
	else if (flags & SPACEFLG)
		*rs++ = ' ';
	if ((flags & PADZERO) && fieldwidth > len) {
		while (fieldwidth-- > len)
			*rs++ = '0';
	}
#ifndef	V7_FLOATSTYLE
	if (*b)
		*rs++ = *b++;
#endif
	pp = rs;
	if ((flags & HASHFLG) || ndigits != 0)	/* Add '.'? */
		*rs++ = dpoint;

	while (*b && ndigits-- > 0)
		*rs++ = *b++;
	if (flags & STRIPZERO) {
		register char	*rpp = pp;

		while (--rs > rpp) {
			if (*rs != '0')
				break;
		}
		if (*rs == dpoint)
			rs--;
		rs++;
	}
	if (flags & UPPERFLG)
		*rs++ = 'E';
	else
		*rs++ = 'e';
	*rs++ = rdecpt >= 0 ? '+' : '-';
	rdecpt = abs(rdecpt);
#ifdef	HAVE_LONGDOUBLE
	if (rdecpt >= 1000) {			/* Max-Exp is > 4000 */
		*rs++ = rdecpt / 1000 + '0';
		rdecpt %= 1000;
	}
#endif
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
#endif	/* __DO_LONG_DOUBLE__ */


/*
 * fcvt() from Cygwin32 is buggy.
 */
#if	!defined(HAVE_FCVT) && defined(HAVE_ECVT)
#define	USE_ECVT
#endif

EXPORT int
ftofs(s, val, fieldwidth, ndigits)
	register	char 	*s;
			MDOUBLE	val;
	register	int	fieldwidth;
	register	int	ndigits;
{
	int	flags = 0;

	if (ndigits < 0) {
		ndigits = -ndigits;
		flags |= UPPERFLG;
	}
	return (_ftofs(s, val, fieldwidth, ndigits, flags));
}

EXPORT int
_ftofs(s, val, fieldwidth, ndigits, flags)
	register	char 	*s;
			MDOUBLE	val;
	register	int	fieldwidth;
	register	int	ndigits;
			int	flags;
{
	register	char	*b;
	register	int	len;
			int 	decpt;
			int	sign;
#ifdef	__DO_LONG_DOUBLE__
			int	Ffmt = FALSE;

	if (flags & UPPERFLG)
		Ffmt = TRUE;
#endif
#ifndef	__DO_LONG_DOUBLE__
	if ((len = _ferr(s, val, flags)) > 0)
		return (len);
#endif
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
#ifdef	__DO_LONG_DOUBLE__
	len = *b;
	if (len == '-' || len == '+')
		len = b[1];
	if (len < '0' || len > '9') {		/* Inf/NaN */
		char	*p;

		for (p = b; *p; p++)
			*s++ = Ffmt ? toupper(*p) : *p;
		*s = '\0';
		return (strlen(b));
	}
#endif
	return (_fcv(s, b, fieldwidth, ndigits, decpt, sign, flags));
}

#ifndef	__DO_LONG_DOUBLE__
LOCAL int
_fcv(s, b, fieldwidth, ndigits, rdecpt, sign, flags)
	register char	*s;
	register char	*b;
	register int	fieldwidth;
	register int	ndigits;
	register int	rdecpt;
	int	sign;
	int	flags;
{
	register int	len;
	register char	*rs = s;
		char	dpoint;
		char	*pp;

#if defined(HAVE_LOCALECONV) && defined(USE_LOCALE)
	dpoint = *(localeconv()->decimal_point);
#else
	dpoint = '.';
#endif

	len = rdecpt + ndigits + 1;
	if (rdecpt < 0)
		len -= rdecpt;
	if (sign)
		len++;
	else if (flags & PLUSFLG)
		len++;
	else if (flags & SPACEFLG)
		len++;
	if ((flags & PADZERO) == 0 && fieldwidth > len) {
		while (fieldwidth-- > len)
			*rs++ = ' ';
	}
	if (sign)
		*rs++ = '-';
	else if (flags & PLUSFLG)
		*rs++ = '+';
	else if (flags & SPACEFLG)
		*rs++ = ' ';
	if ((flags & PADZERO) && fieldwidth > len) {
		while (fieldwidth-- > len)
			*rs++ = '0';
	}
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
	pp = rs;
	if ((flags & HASHFLG) || ndigits != 0)		/* Add '.'? */
		*rs++ = dpoint;

	if (rdecpt < 0) {
		len = rdecpt;
		while (len++ < 0 && ndigits-- > 0)
			*rs++ = '0';
	}
	while (*b && ndigits-- > 0)
		*rs++ = *b++;
	if (flags & STRIPZERO) {
		register char	*rpp = pp;

		while (--rs > rpp) {
			if (*rs != '0')
				break;
		}
		if (*rs == dpoint)
			rs--;
		rs++;
	} else {
#ifdef	USE_ECVT
		while (ndigits-- > 0)
			*rs++ = '0';
#endif
	}
	*rs = '\0';
	return (rs - s);
}
#endif	/* __DO_LONG_DOUBLE__ */

EXPORT int
_ftogs(s, val, fieldwidth, ndigits, flags)
	register	char 	*s;
			MDOUBLE	val;
	register	int	fieldwidth;
	register	int	ndigits;
			int	flags;
{
	register	char	*b;
	register	int	len;
			int 	decpt;
			int	sign;
#ifdef	__DO_LONG_DOUBLE__
			int	Gfmt = FALSE;

	if (flags & UPPERFLG)
		Gfmt = TRUE;
#endif
#ifndef	__DO_LONG_DOUBLE__
	if ((len = _ferr(s, val, flags)) > 0)
		return (len);
#endif
	b = ecvt(val, ndigits, &decpt, &sign);
#ifdef	__DO_LONG_DOUBLE__
	len = *b;
	if (len == '-' || len == '+')
		len = b[1];
	if (len < '0' || len > '9') {		/* Inf/NaN */
		char	*p;

		for (p = b; *p; p++)
			*s++ = Gfmt ? toupper(*p) : *p;
		*s = '\0';
		return (strlen(b));
	}
#endif
	if ((flags & HASHFLG) == 0)
		flags |= STRIPZERO;
#ifdef	V7_FLOATSTYLE
	if ((decpt >= 0 && decpt-ndigits > 4) ||
#else
	if ((decpt >= 0 && decpt-ndigits > 0) ||
#endif
	    (decpt < -3)) {			/* e-format */
		decpt--;
		return (_ecv(s, b, fieldwidth, ndigits, decpt, sign, flags));
	} else {				/* f-format */
		ndigits -= decpt;
		return (_fcv(s, b, fieldwidth, ndigits, decpt, sign, flags));
	}
}


#ifndef	__DO_LONG_DOUBLE__

#ifdef	HAVE_LONGDOUBLE
#ifdef	HAVE__LDECVT
#define	qecvt(ld, n, dp, sp)	_ldecvt(*(long_double *)&ld, n, dp, sp)
#endif
#ifdef	HAVE__LDFCVT
#define	qfcvt(ld, n, dp, sp)	_ldfcvt(*(long_double *)&ld, n, dp, sp)
#endif

#if	(defined(HAVE_QECVT) || defined(HAVE__LDECVT)) && \
	(defined(HAVE_QFCVT) || defined(HAVE__LDFCVT))
#define	__DO_LONG_DOUBLE__
#define	ftoes	qftoes
#define	ftofs	qftofs
#define	_ftoes	_qftoes
#define	_ftofs	_qftofs
#define	_ftogs	_qftogs
#define	ecvt	qecvt
#define	fcvt	qfcvt
#include "fconv.c"
#undef	__DO_LONG_DOUBLE__
#endif
#endif	/* HAVE_LONGDOUBLE */

/*
 * Be careful to avoid a GCC bug: while (*s) *s++ = toupper(*s) is converted
 * to  while (*s++) *s = toupper(*s).
 */
LOCAL int
_ferr(s, val, flags)
	char	*s;
	double	val;
	int	flags;
{
	char	*old = s;
	char	*p;
	int	upper;

	upper = flags & UPPERFLG;
	if (isnan(val)) {
		if (val < 0)		/* Should not happen */
			*s++ = '-';
		else if (flags & PLUSFLG)
			*s++ = '+';
		for (p = _js_nan; *p; p++)
			*s++ = upper ? toupper(*p) : *p;
		*s = '\0';
		return (s - old);
	}

	/*
	 * Check first for NaN because finite() will return 1 on Nan too.
	 */
	if (isinf(val)) {
		if (val < 0)
			*s++ = '-';
		else if (flags & PLUSFLG)
			*s++ = '+';
		for (p = _js_inf; *p; p++)
			*s++ = upper ? toupper(*p) : *p;
		*s = '\0';
		return (s - old);
	}
	return (0);
}
#endif	/* __DO_LONG_DOUBLE__ */
#endif	/* NO_FLOATINGPOINT */
