/* @(#)libport.h	1.14 07/01/16 Copyright 1995-2007 J. Schilling */
/*
 *	Copyright (c) 1995-2007 J. Schilling
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
#ifndef _SCHILY_STANDARD_H
#include <schily/standard.h>
#endif

/*
 * Try to get HOST_NAME_MAX for gethostname()
 */
#ifndef _SCHILY_UNISTD_H
#include <schily/unistd.h>
#endif

#ifndef HOST_NAME_MAX
#if	defined(HAVE_NETDB_H) && !defined(HOST_NOT_FOUND) && \
				!defined(_INCL_NETDB_H)
#include <netdb.h>
#define	_INCL_NETDB_H
#endif
#ifdef	MAXHOSTNAMELEN
#define	HOST_NAME_MAX	MAXHOSTNAMELEN
#endif
#endif

#ifndef HOST_NAME_MAX
#ifndef	_SCHILY_PARAM_H
#include <schily/param.h>	/* Include various defs needed with some OS */
#endif				/* Linux MAXHOSTNAMELEN */
#ifdef	MAXHOSTNAMELEN
#define	HOST_NAME_MAX	MAXHOSTNAMELEN
#endif
#endif

#ifndef HOST_NAME_MAX
#define	HOST_NAME_MAX	255
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
#ifndef	HAVE_GETHOSTNAME
extern	int		gethostname	__PR((char *name, int namelen));
#endif
#ifndef	HAVE_GETDOMAINNAME
extern	int		getdomainname	__PR((char *name, int namelen));
#endif
#ifndef	HAVE_GETPAGESIZE
EXPORT	int		getpagesize	__PR((void));
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

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_LIBPORT_H */
