/* @(#)wchar.h	1.4 07/04/25 Copyright 2007 J. Schilling */
/*
 *	Abstraction from wchar.h
 *
 *	Copyright (c) 2007 J. Schilling
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

#ifndef _SCHILY_WCHAR_H
#define	_SCHILY_WCHAR_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef	_SCHILY_STDLIB_H
#include <schily/stdlib.h>	/* for MB_CUR_MAX */
#endif

#ifdef	HAVE_WCHAR_H

#include <wchar.h>

#else	/* HAVE_WCHAR_H */

#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif
#ifdef	HAVE_STDDEF_H
#include <stddef.h>
#endif

#ifndef	_INCL_STDIO_H
#include <stdio.h>
#define	_INCL_STDIO_H
#endif

#ifndef	_SCHILY_VARARGS_H
#include <schily/varargs.h>
#endif


#undef	USE_WCHAR
#endif	/* !HAVE_WCHAR_H */

#ifndef	USE_WCHAR

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

#endif	/* !USE_WCHAR */

#endif	/* _SCHILY_WCHAR_H */
