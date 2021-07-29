/* @(#)resource.h	1.15 21/07/23 Copyright 1995-2021 J. Schilling */
/*
 *	Abstraction from resource limits
 *
 *	Missing parts for wait3() taken from SunOS
 *
 *	Copyright (c) 1995-2021 J. Schilling
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

#ifndef	_SCHILY_RESOURCE_H
#define	_SCHILY_RESOURCE_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef	_SCHILY_STDINT_H
#include <schily/stdint.h>	/* May be needed for rlim_t */
#endif

#ifndef	_SCHILY_TIME_H
#include <schily/time.h>
#endif

/*
 * Get definitions from system include files
 */
#ifdef	HAVE_SYS_RESOURCE_H
#ifndef	_INCL_SYS_RESOURCE_H
#include <sys/resource.h>
#define	_INCL_SYS_RESOURCE_H
#endif
#endif

#if	defined(OS390) || defined(__MVS__)

#ifdef	__never__			/* Compilation fail		*/
#ifdef	HAVE_SYS_DBX_PLUGIN_H		/* with suseconds_t redefined	*/
#ifndef	_INCL_SYS_DBX_PLUGIN_H
#include <sys/dbx_plugin.h>		/* Needed for RLIM_NLIMITS on z/OS */
#define	_INCL_SYS_DBX_PLUGIN_H
#endif
#endif
#endif	/* __never__ */

#if	defined(RLIMIT_MEMLIMIT) && !defined(RLIM_NLIMITS)
#define	RLIM_NLIMITS	(RLIMIT_MEMLIMIT+1)
#endif
#if	!defined(RLIM_NLIMITS)
#define	RLIM_NLIMITS	8
#endif

#endif	/* defined(OS390) || defined(__MVS__) */

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Now check which definitions are missing and define them localy.
 */

#ifndef	RUSAGE_SELF
/*
 * Resource utilization information.
 */

#define	RUSAGE_SELF	0
#define	RUSAGE_CHILDREN	-1

#endif	/* RUSAGE_SELF */

#ifndef	HAVE_STRUCT_RUSAGE

/*
 * On a vanilla BSD system, all fields are set.
 * On other systems, parts may not be supported.
 *
 * S: 0 means that the value is 0 on Solaris
 * S: * means that the value is the sum of rm_asrss on Solaris
 *
 * POSIX and BeOS/Haiku define only ru_utime and ru_stime.
 */
struct	rusage {
	struct timeval ru_utime;	/* user time used */
	struct timeval ru_stime;	/* system time used */
	long	ru_maxrss;		/* maximum resident set size S: 0 */
#define	ru_first	ru_ixrss
	long	ru_ixrss;		/* integral shared memory size S: 0 */
	long	ru_idrss;		/* integral unshared data size S: * */
	long	ru_isrss;		/* integral unshared stack size S: 0 */
	long	ru_minflt;		/* any page faults not requiring I/O */
	long	ru_majflt;		/* any page faults requiring I/O */
	long	ru_nswap;		/* swaps */
	long	ru_inblock;		/* block input operations */
	long	ru_oublock;		/* block output operations */
	long	ru_msgsnd;		/* messages sent */
	long	ru_msgrcv;		/* messages received */
	long	ru_nsignals;		/* signals received */
	long	ru_nvcsw;		/* voluntary context switches */
	long	ru_nivcsw;		/* involuntary " */
#define	ru_last		ru_nivcsw
};
#endif	/* HAVE_STRUCT_RUSAGE */

#ifndef	HAVE_TYPE_RLIM_T
#define	rlim_t	Intmax_t
#endif

#ifndef	RLIMIT_CPU
/*
 * Resource limits
 */
#define	RLIMIT_CPU	0		/* cpu time in milliseconds */
#define	RLIMIT_FSIZE	1		/* maximum file size */
#define	RLIMIT_DATA	2		/* data size */
#define	RLIMIT_STACK	3		/* stack size */
#define	RLIMIT_CORE	4		/* core file size */
#define	RLIMIT_RSS	5		/* resident set size */

#define	RLIM_NLIMITS	6		/* number of resource limits */

#ifndef	RLIM_INFINITY
#define	RLIM_INFINITY	0x7fffffff
#endif

struct rlimit {
	rlim_t	rlim_cur;		/* current (soft) limit */
	rlim_t	rlim_max;		/* maximum value for rlim_cur */
};

#endif	/* RLIMIT_CPU */

#ifdef	__cplusplus
}
#endif

#endif /* _SCHILY_RESOURCE_H */
