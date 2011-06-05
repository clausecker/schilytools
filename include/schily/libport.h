/* @(#)libport.h	1.26 11/06/04 Copyright 1995-2010 J. Schilling */
/*
 *	Copyright (c) 1995-2010 J. Schilling
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


#ifndef _SCHILY_LIBPORT_H
#define	_SCHILY_LIBPORT_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_TYPES_H
#include <schily/types.h>
#endif

#ifndef _SCHILY_UNISTD_H
#include <schily/unistd.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#if	defined(_INCL_SYS_TYPES_H) || defined(_INCL_TYPES_) || defined(size_t)
#	ifndef	FOUND_SIZE_T
#	define	FOUND_SIZE_T
#	endif
#endif
#if	defined(_MSC_VER) && !defined(_SIZE_T_DEFINED)
#	undef	FOUND_SIZE_T
#endif

#ifdef	OPENSERVER
/*
 * Don't use the usleep() from libc on SCO's OPENSERVER.
 * It will kill our processes with SIGALRM.
 */
/*
 * Don't #undef HAVE_USLEEP in this file, SCO has a
 * usleep() prototype in unistd.h
 */
/* #undef	HAVE_USLEEP */
#endif

#ifndef	HAVE_GETHOSTID
extern	long		gethostid	__PR((void));
#endif
#ifndef	HAVE_GETPAGESIZE
extern	int		getpagesize	__PR((void));
#endif
#ifndef	HAVE_USLEEP
extern	int		usleep		__PR((int usec));
#endif

#ifndef	HAVE_STRCAT
extern	char		*strcat		__PR((char *s1, const char *s2));
#endif
#ifndef	HAVE_STRCHR
extern	char		*strchr		__PR((const char *s1, int c));
#endif
#ifndef	HAVE_STRCMP
extern	int		strcmp		__PR((const char *s1, const char *s2));
#endif
#ifndef	HAVE_STRCPY
extern	char		*strcpy		__PR((char *s1, const char *s2));
#endif
#if	!defined(HAVE_STRDUP) || defined(__SVR4)
extern	char		*strdup		__PR((const char *s));
#endif
#ifdef	FOUND_SIZE_T
#ifndef	HAVE_STRNDUP
extern	char		*strndup	__PR((const char *s, size_t len));
#endif
#ifndef	HAVE_STRLEN
extern	size_t		strlen		__PR((const char *s));
#endif
#ifndef	HAVE_STRNLEN
extern	size_t		strnlen		__PR((const char *s, size_t len));
#endif
#ifndef	HAVE_STRLCAT
extern	size_t		strlcat		__PR((char *s1, const char *s2,
							size_t len));
#endif
#ifndef	HAVE_STRLCPY
extern	size_t		strlcpy		__PR((char *s1, const char *s2,
							size_t len));
#endif
#ifndef	HAVE_STRNCAT
extern	char		*strncat	__PR((char *s1, const char *s2,
							size_t len));
#endif
#ifndef	HAVE_STRNCMP
extern	int		strncmp		__PR((const char *s1, const char *s2,
							size_t len));
#endif
#ifndef	HAVE_STRNCPY
extern	char		*strncpy	__PR((char *s1, const char *s2,
							size_t len));
#endif
#endif	/* FOUND_SIZE_T */
#ifndef	HAVE_STRRCHR
extern	char		*strrchr	__PR((const char *s1, int c));
#endif

#ifdef	_SCHILY_WCHAR_H
#ifndef	HAVE_WCSCAT
extern	wchar_t		*wcscat		__PR((wchar_t *s1, const wchar_t *s2));
#endif
#ifndef	HAVE_WCSCHR
extern	wchar_t		*wcschr		__PR((const wchar_t *s1, wchar_t c));
#endif
#ifndef	HAVE_WCSCMP
extern	int		wcscmp		__PR((const wchar_t *s1,
							const wchar_t *s2));
#endif
#ifndef	HAVE_WCSCPY
extern	wchar_t		*wcscpy		__PR((wchar_t *s1, const wchar_t *s2));
#endif
#ifndef	HAVE_WCSDUP
extern	wchar_t		*wcsdup		__PR((const wchar_t *s));
#endif
#ifdef	FOUND_SIZE_T
#ifndef	HAVE_WCSNDUP
extern	wchar_t		*wcsndup	__PR((const wchar_t *s, size_t len));
#endif
#ifndef	HAVE_WCSLEN
extern	size_t		wcslen		__PR((const wchar_t *s));
#endif
#ifndef	HAVE_WCSNLEN
extern	size_t		wcsnlen		__PR((const wchar_t *s, size_t len));
#endif
#ifndef	HAVE_WCSLCAT
extern	size_t		wcslcat		__PR((wchar_t *s1, const wchar_t *s2,
							size_t len));
#endif
#ifndef	HAVE_WCSLCPY
extern	size_t		wcslcpy		__PR((wchar_t *s1, const wchar_t *s2,
							size_t len));
#endif
#ifndef	HAVE_WCSNCAT
extern	wchar_t		*wcsncat	__PR((wchar_t *s1, const wchar_t *s2,
							size_t len));
#endif
#ifndef	HAVE_WCSNCMP
extern	int		wcsncmp		__PR((const wchar_t *s1,
							const wchar_t *s2,
							size_t len));
#endif
#ifndef	HAVE_WCSNCPY
extern	wchar_t		*wcsncpy	__PR((wchar_t *s1, const wchar_t *s2,
							size_t len));
#endif
#endif	/* FOUND_SIZE_T */
#ifndef	HAVE_WCSRCHR
extern	wchar_t		*wcsrchr	__PR((const wchar_t *s1, wchar_t c));
#endif
#endif	/* _SCHILY_WCHAR_H */

#ifndef	HAVE_RENAME
extern	int		rename		__PR((const char *old,
							const char *new));
#endif

/*
 * XXX Note: libgen.h / -lgen on Solaris contain eaccess()
 */
#ifndef	HAVE_EACCESS
extern	int		eaccess		__PR((const char *name, int mode));
#endif

/*
 * See also libgen.h
 */
#ifndef	HAVE_BASENAME
extern	char		*basename	__PR((char *name));
#endif
#ifndef	HAVE_DIRNAME
extern	char		*dirname	__PR((char *name));
#endif

#ifndef	HAVE_TIMEGM
#if	defined(_SCHILY_TIME_H)
extern	time_t		timegm		__PR((struct tm *));
#endif
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_LIBPORT_H */
