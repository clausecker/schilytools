/* @(#)prototyp.h	1.12 06/09/13 Copyright 1995 J. Schilling */
/*
 *	Definitions for dealing with ANSI / KR C-Compilers
 *
 *	Copyright (c) 1995 J. Schilling
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

/*
 * mconfig.h includes prototype.h so we must do this include before we test
 * for _SCHILY_PROTOTYP_H
 */
#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef	_SCHILY_PROTOTYP_H
#define	_SCHILY_PROTOTYP_H

#ifndef	PROTOTYPES
	/*
	 * If this has already been defined,
	 * someone else knows better than us...
	 */
#	ifdef	__STDC__
#		if	__STDC__				/* ANSI C */
#			define	PROTOTYPES
#		endif
#		if	defined(sun) && __STDC__ - 0 == 0	/* Sun C */
#			define	PROTOTYPES
#		endif
#	endif
#endif	/* PROTOTYPES */

#if	!defined(PROTOTYPES) && (defined(__cplusplus) || defined(_MSC_VER))
	/*
	 * C++ always supports prototypes.
	 * Define PROTOTYPES so we are not forced to make
	 * a separtate autoconf run for C++
	 *
	 * Microsoft C has prototypes but does not define __STDC__
	 */
#	define	PROTOTYPES
#endif

/*
 * If we have prototypes, we should have stdlib.h string.h stdarg.h
 */
#ifdef	PROTOTYPES
#if	!(defined(SABER) && defined(sun))
#	ifndef	HAVE_STDARG_H
#		define	HAVE_STDARG_H
#	endif
#endif
#ifndef	JOS
#	ifndef	HAVE_STDLIB_H
#		define	HAVE_STDLIB_H
#	endif
#	ifndef	HAVE_STRING_H
#		define	HAVE_STRING_H
#	endif
#	ifndef	HAVE_STDC_HEADERS
#		define	HAVE_STDC_HEADERS
#	endif
#	ifndef	STDC_HEADERS
#		define	STDC_HEADERS	/* GNU name */
#	endif
#endif
#endif

#ifdef	NO_PROTOTYPES		/* Force not to use prototypes */
#	undef	PROTOTYPES
#endif

#ifdef	PROTOTYPES
#	define	__PR(a)	a
#else
#	define	__PR(a)	()
#endif

#endif	/* _SCHILY_PROTOTYP_H */
