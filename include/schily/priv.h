/* @(#)priv.h	1.4 13/04/29 Copyright 2009-2013 J. Schilling */
/*
 *	Abstraction code for fine grained process privileges
 *
 *	Copyright (c) 2009-2013 J. Schilling
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

#ifndef	_SCHILY_PRIV_H
#define	_SCHILY_PRIV_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

/*
 * The Solaris process privileges interface.
 */
#if	defined(HAVE_PRIV_H) && \
	defined(HAVE_GETPPRIV) && defined(HAVE_SETPPRIV) && \
	defined(HAVE_PRIV_SET)

#define	HAVE_SOLARIS_PPRIV
#endif

#ifdef	NO_SOLARIS_PPRIV
#undef	HAVE_SOLARIS_PPRIV
#endif

#ifdef	HAVE_SOLARIS_PPRIV
#ifndef	_INCL_PRIV_H
#define	_INCL_PRIV_H
#include <priv.h>
#endif
#endif

/*
 * AIX implements an incompatible process privileges interface.
 * On AIX, we have sys/priv.h, getppriv(), setppriv() but no priv_set().
 */
#if	defined(HAVE_SYS_PRIV_H) && \
	defined(HAVE_GETPPRIV) && defined(HAVE_SETPPRIV) && \
	defined(HAVE_PRIVBIT_SET)

#define	HAVE_AIX_PPRIV
#endif

#ifdef	NO_AIX_PPRIV
#undef	HAVE_AIX_PPRIV
#endif

#ifdef	HAVE_AIX_PPRIV
#ifndef	_INCL_SYS_PRIV_H
#define	_INCL_SYS_PRIV_H
#include <sys/priv.h>
#endif
#endif

/*
 * The POSIX.1e draft has been withdrawn in 1997.
 * Linux started to implement this outdated concept in 1997.
 * On Linux, we have sys/capability.h, cap_get_proc(), cap_set_proc(),
 * cap_set_flag() cap_clear_flag()
 */
#if	defined(HAVE_SYS_CAPABILITY_H) && \
	defined(HAVE_CAP_GET_PROC) && defined(HAVE_CAP_SET_PROC) && \
	defined(HAVE_CAP_SET_FLAG) && defined(HAVE_CAP_CLEAR_FLAG)

#define	HAVE_LINUX_CAPS
#endif

#ifdef	NO_LINUX_CAPS
#undef	HAVE_LINUX_CAPS
#endif

#ifdef	HAVE_LINUX_CAPS
#ifndef	_INCL_SYS_CAPABILITY_H
#define	_INCL_SYS_CAPABILITY_H
#include <sys/capability.h>
#endif
#endif

#endif	/* _SCHILY_PRIV_H */
