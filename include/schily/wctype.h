/* @(#)wctype.h	1.2 09/04/12 Copyright 2009 J. Schilling */
/*
 *	Abstraction from wctype.h
 *
 *	Copyright (c) 2009 J. Schilling
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

#ifndef _SCHILY_WCTYPE_H
#define	_SCHILY_WCTYPE_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef	_SCHILY_WCHAR_H
#include <schily/wchar.h>
#endif

#ifdef	HAVE_WCTYPE_H

#ifdef	USE_WCHAR
#ifndef	_INCL_WCTYPE_H
#include <wctype.h>
#define	_INCL_WCTYPE_H
#endif

#define	USE_WCTYPE
#endif	/* USE_WCHAR */
#else	/* HAVE_WCTYPE_H */

#undef	USE_WCTYPE
#endif	/* !HAVE_WCTYPE_H */

#ifndef	USE_WCTYPE
#undef	USE_WCHAR

/*
 * We either don't have wide chars or we don't use them...
 */
#undef	wchar_t
#define	wchar_t	char
#undef	wint_t
#define	wint_t	int

#undef	WEOF
#define	WEOF	((wint_t)-1)

#ifndef	_SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif

#undef	WCHAR_MAX
#define	WCHAR_MAX	TYPE_MAXVAL(wchar_t)
#undef	WCHAR_MIN
#define	WCHAR_MIN	TYPE_MINVAL(wchar_t)

#include <ctype.h>

#undef	iswalnum
#define	iswalnum(c)	isalnum(c)
#undef	iswalpha
#define	iswalpha(c)	isalpha(c)
#undef	iswcntrl
#define	iswcntrl(c)	iscntrl(c)
#undef	iswcntrl
#define	iswcntrl(c)	iscntrl(c)
#undef	iswdigit
#define	iswdigit(c)	isdigit(c)
#undef	iswgraph
#define	iswgraph(c)	isgraph(c)
#undef	iswlower
#define	iswlower(c)	islower(c)
#undef	iswprint
#define	iswprint(c)	isprint(c)
#undef	iswpunct
#define	iswpunct(c)	ispunct(c)
#undef	iswspace
#define	iswspace(c)	isspace(c)
#undef	iswupper
#define	iswupper(c)	isupper(c)
#undef	iswxdigit
#define	iswxdigit(c)	isxdigit(c)

#undef	towlower
#define	towlower(c)	tolower(c)
#undef	towupper
#define	towupper(c)	toupper(c)

#undef	MB_CUR_MAX
#define	MB_CUR_MAX	1
#undef	MB_LEN_MAX
#define	MB_LEN_MAX	1

#undef	mbtowc
#define	mbtowc(wp, cp, len)	(*(wp) = *(cp), 1)
#undef	wctomb
#define	wctomb(cp, wc)		(*(cp) = wc, 1)

#endif	/* !USE_WCTYPE */

#endif	/* _SCHILY_WCTYPE_H */
