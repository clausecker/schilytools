/* @(#)signal.h	1.11 18/06/07 Copyright 1997-2018 J. Schilling */
/*
 *	Signal abstraction for signals
 *
 *	Copyright (c) 1997-2018 J. Schilling
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

#ifndef	_SCHILY_SIGNAL_H
#define	_SCHILY_SIGNAL_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif

#ifdef	HAVE_SIGNAL_H
#ifndef	_INCL_SIGNAL_H
#include <signal.h>
#define	_INCL_SIGNAL_H
#endif
#endif

#ifndef	SIGCHLD		/* POSIX name is missing */
#ifdef	SIGCLD
#define	SIGCHLD	SIGCLD
#endif
#endif

/*
 * Very old Solaris versions (probably only before 1993) did not include
 * siginfo.h in sginal.h. POSIX requires the availaibility of siginfo_t
 * from signal.h, so include siginfo.h if available on this platform.
 */
#ifdef	HAVE_SIGINFO_H
#ifndef	_INCL_SIGINFO_H
#include <siginfo.h>
#define	_INCL_SIGINFO_H
#endif
#else
#ifdef	HAVE_SYS_SIGINFO_H
#ifndef	_INCL_SYS_SIGINFO_H
#include <sys/siginfo.h>
#define	_INCL_SYS_SIGINFO_H
#endif
#endif
#endif

#if	!defined(HAVE_TYPE_SIGINFO_T) && !defined(HAVE_SIGINFO_T)

typedef struct {
	int	si_code;	/* Child status code */
	pid_t	si_pid;		/* Child pid */
	int	si_status;	/* Child exit code or signal number */
	clock_t	si_utime;
	clock_t	si_stime;
} siginfo_t;

#ifndef	id_t
#define	id_t	pid_t
#endif

#endif	/* !defined(HAVE_TYPE_SIGINFO_T) && !defined(HAVE_SIGINFO_T) */

#endif	/* _SCHILY_SIGNAL_H */
