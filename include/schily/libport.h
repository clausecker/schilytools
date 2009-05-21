/* @(#)libport.h	1.18 09/05/05 Copyright 1995-2009 J. Schilling */
/*
 *	Copyright (c) 1995-2009 J. Schilling
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

#ifdef	OPENSERVER
/*
 * Don't use the usleep() from libc on SCO's OPENSERVER.
 * It will kill our processes with SIGALRM.
 */
/*
 * Don't #undef HAVE_USLEEP in this file, SCO has a
 * usleep() prototype in unistd.h
 */
/*#undef	HAVE_USLEEP*/
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

#if	!defined(HAVE_STRDUP) || defined(__SVR4)
extern	char		*strdup		__PR((const char *s));
#endif
#ifndef	HAVE_STRNCPY
extern	char		*strncpy	__PR((char *s1, const char *s2, size_t len));
#endif
#ifndef	HAVE_STRLCPY
extern	size_t		strlcpy		__PR((char *s1, const char *s2, size_t len));
#endif

#ifndef	HAVE_RENAME
extern	int		rename		__PR((const char *old, const char *new));
#endif

#ifndef	HAVE_EACCESS
extern	int		eaccess		__PR((char *name, int mode));
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_LIBPORT_H */
