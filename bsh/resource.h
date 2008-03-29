/* @(#)resource.h	1.6 08/03/28 Copyright 1995-2008 J. Schilling */
/*
 *	Missing parts for wait3() taken from SunOS
 *
 *	Copyright (c) 1995-2008 J. Schilling
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

#ifndef	_RESOURCE_H
#define	_RESOURCE_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef	_SCHILY_TIME_H
#include <schily/time.h>
#endif

/*
 * Get definitions from system include files
 */
#ifdef	HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
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

struct	rusage {
	struct timeval ru_utime;	/* user time used */
	struct timeval ru_stime;	/* system time used */
	long	ru_maxrss;
#define	ru_first	ru_ixrss
	long	ru_ixrss;		/* XXX: 0 */
	long	ru_idrss;		/* XXX: sum of rm_asrss */
	long	ru_isrss;		/* XXX: 0 */
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
	int	rlim_cur;		/* current (soft) limit */
	int	rlim_max;		/* maximum value for rlim_cur */
};

#endif	/* RLIMIT_CPU */

#endif /* _RESOURCE_H */
