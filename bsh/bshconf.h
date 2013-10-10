/* @(#)bshconf.h	1.20 13/09/25 Copyright 1991-2013 J. Schilling */
/*
 *	Copyright (c) 1991-2013 J. Schilling
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

#ifdef	RETSIGTYPE	/* From schily/mconfig.h */

typedef	RETSIGTYPE	(*sigtype) __PR((int));
typedef	RETSIGTYPE	sigret;

#else

#define	VOID_SIGS

#ifdef VOID_SIGS
typedef	void	(*sigtype) __PR((int));
typedef	void	sigret;
#else
typedef	int	(*sigtype) __PR((int));
typedef	int	sigret;
#endif

#endif


#if	defined(HAVE_GETPGID) || defined(HAVE_SETPGID)
#define	POSIXJOBS
#endif

#if	!defined(HAVE_GETPGID) && defined(HAVE_BSD_GETPGRP)
#define	getpgid	getpgrp
#endif

#if	!defined(HAVE_GETSID) && defined(HAVE_BSD_GETPGRP)
#define	getsid	getpgrp
#else
#if	!defined(HAVE_GETSID) && !defined(HAVE_BSD_GETPGRP)
#define	getsid	getpgid
#endif
#endif

#if	!defined(HAVE_SETPGID) && defined(HAVE_BSD_SETPGRP)
#define	setpgid	setpgrp
#endif

#if	!defined(HAVE_SETSID) && defined(HAVE_BSD_GETPGRP)
#define	setsid	setpgrp(getpid())
#endif

#if	!defined(HAVE_GETPGID) && !defined(HAVE_BSD_GETPGRP)
/*
 * FreeBSD (anything that is BSD-4.4 derived)
 * does not conform to POSIX and is not old BSD conforming either.
 * Note that this seems to have been changed between 1995 and 2000
 *
 * getpgrp()/setpgrp() are not POSIX on BSD-4.4
 *
 * setpgrp() is old BSD compliant,
 * getpgrp() is not BSD compliant but it is POSIX compliant
 *
 * setpgid() is OK (POSIX compliant)
 * getpgid() is missing!
 *
 * The builtin command 'pgrp' will not work correctly for this reason.
 *
 *	BSD:
 *
 *	getpgrp(pid)		-> pgid for pid
 *	setpgrp(pid, pgid)	-> set pgid of pid
 *
 *	POSIX:
 *
 *	getpgid(pid)		-> pgid for pid
 *	setpgid(pid, pgid)	-> set pgid of pid
 *	getpgrp(void)		-> pgid for $$
 *	setpgrp(void)		-> setpgid(0,0)
 *
 *	4.4-BSD:
 *
 *	getpgid(pid)		-> is missing!
 *	setpgid(pid, pgid)	-> set pgid of pid
 *	getpgrp(void)		-> ????
 *	setpgrp(pid, pgid)	-> set pgid of pid
 */
#define	getpgid(a)	getpgrp()
#endif

#if	!defined(HAVE_GETPGID) && !defined(HAVE_BSD_GETPGRP) && \
	!defined(HAVE_GETPGRP)
#undef	getpgid
#define	getpgid(a)		getpid()
#endif

#if	!defined(HAVE_SETPGID) && !defined(HAVE_BSD_SETPGRP) && \
	!defined(HAVE_SETPGRP)
#undef	setpgid
#define	setpgid(pid, pgid)	(0)
#endif

#ifdef	HAVE_SIGSET
/*
 * Try to by default use the function that sets up signal handlers in a way
 * that does not reset the handler after it has been called.
 */
#define	signal	sigset
#endif

#ifdef	__linux__
#define	__USE_BSD_SIGNAL	/* needed for Linux */
#endif

#if	defined(__CYGWIN32__) || defined(__CYGWIN__)
#undef	DO_SUID
#endif

#ifndef	HAVE_CRYPT
#undef	DO_SUID
#endif

#ifdef	DO_PFEXEC
#ifdef	HAVE_EXEC_ATTR_H
#include <exec_attr.h>
#endif
#endif

#ifndef	DO_PFEXEC
#undef	EXECATTR_FILENAME
#endif

#ifdef	LIB_SHEDIT
#define	freetree	shell_freetree
#endif
