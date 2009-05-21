/* @(#)wchar.h	1.6 09/04/12 Copyright 2007-2009 J. Schilling */
/*
 *	Abstraction from wchar.h
 *
 *	Copyright (c) 2007-2009 J. Schilling
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
#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif
#ifdef	HAVE_STDDEF_H
#ifndef	_INCL_STDDEF_H
#include <stddef.h>		/* Needed for e.g. size_t (POSIX)  */
#define	_INCL_STDDEF_H
#endif
#endif
#ifndef _SCHILY_STDIO_H
#include <schily/stdio.h>	/* Needed for e.g. FILE (POSIX)	   */
#endif
#ifndef	_SCHILY_VARARGS_H
#include <schily/varargs.h>	/* Needed for e.g. va_list (POSIX) */
#endif


#ifdef	HAVE_WCHAR_H

#ifndef	_INCL_WCHAR_H
#include <wchar.h>
#define	_INCL_WCHAR_H
#endif

#else	/* HAVE_WCHAR_H */

#undef	USE_WCHAR
#endif	/* !HAVE_WCHAR_H */


#ifndef	USE_WCHAR

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
#include <schily/utypes.h>	/* For TYPE_MAXVAL() */
#endif

#undef	WCHAR_MAX
#define	WCHAR_MAX	TYPE_MAXVAL(wchar_t)
#undef	WCHAR_MIN
#define	WCHAR_MIN	TYPE_MINVAL(wchar_t)

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
