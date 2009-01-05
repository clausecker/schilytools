/* @(#)vedstats.c	1.6 08/12/22 Copyright 2000-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)vedstats.c	1.6 08/12/22 Copyright 2000-2008 J. Schilling";
#endif
/*
 *	Statistics module for VED (Visual EDitor)
 *
 *	Copyright (c) 2000-2008 J. Schilling
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

#include "ved.h"
#include <signal.h>
#include <setjmp.h>
#include <schily/jmpdefs.h>

EXPORT	long	charstyped;

EXPORT	void	vedstartstats	__PR((void));
EXPORT	void	vedstopstats	__PR((void));
EXPORT	void	vedstatistics	__PR((void));

#ifdef	VED_STATS

#ifdef	HAVE_LIMITS_H
#include <limits.h>
#endif
#include <schily/time.h>
/*
 * Make sure to include schily/time.h before, because of a Next Step bug.
 */
#ifdef	HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif

#ifndef	CLK_TCK
#define	CLK_TCK	60
#endif

#ifdef	HAVE_TIMES
LOCAL	struct tms	stms;
LOCAL	struct tms	etms;
#endif

EXPORT void
vedstartstats()
{
#ifdef	HAVE_TIMES
	times(&stms);
#endif
}

EXPORT void
vedstopstats()
{
#ifdef	HAVE_TIMES
	times(&etms);
#endif
}

EXPORT void
vedstatistics()
{
#ifdef	HAVE_TIMES
	struct tms	tms;
	long		usecs;
#endif

	if (getenv("VED_STATISTICS") == NULL)
		return;

#ifdef	HAVE_TIMES
	times(&tms);

	error("input chars %ld\n", charstyped);
	usecs = 1000000 / CLK_TCK;
	usecs *= tms.tms_utime;
	error("user time %8ld 탎 %5ld 탎/char\n",
		usecs, usecs/charstyped);
	usecs = 1000000 / CLK_TCK;
	usecs *= tms.tms_stime;
	error("sys  time %8ld 탎 %5ld 탎/char\n",
		usecs, usecs/charstyped);
	usecs = 1000000 / CLK_TCK;
	usecs *= (tms.tms_utime + tms.tms_stime);
	error("sum  time %8ld 탎 %5ld 탎/char\n",
		usecs, usecs/charstyped);

	error("witout load time:\n");
	tms.tms_utime -= stms.tms_utime;
	tms.tms_stime -= stms.tms_stime;

	usecs = 1000000 / CLK_TCK;
	usecs *= tms.tms_utime;
	error("user time %8ld 탎 %5ld 탎/char\n",
		usecs, usecs/charstyped);
	usecs = 1000000 / CLK_TCK;
	usecs *= tms.tms_stime;
	error("sys  time %8ld 탎 %5ld 탎/char\n",
		usecs, usecs/charstyped);
	usecs = 1000000 / CLK_TCK;
	usecs *= (tms.tms_utime + tms.tms_stime);
	error("sum  time %8ld 탎 %5ld 탎/char\n",
		usecs, usecs/charstyped);

	error("witout save time:\n");
	tms.tms_utime = etms.tms_utime - stms.tms_utime;
	tms.tms_stime = etms.tms_stime - stms.tms_stime;

	usecs = 1000000 / CLK_TCK;
	usecs *= tms.tms_utime;
	error("user time %8ld 탎 %5ld 탎/char\n",
		usecs, usecs/charstyped);
	usecs = 1000000 / CLK_TCK;
	usecs *= tms.tms_stime;
	error("sys  time %8ld 탎 %5ld 탎/char\n",
		usecs, usecs/charstyped);
	usecs = 1000000 / CLK_TCK;
	usecs *= (tms.tms_utime + tms.tms_stime);
	error("sum  time %8ld 탎 %5ld 탎/char\n",
		usecs, usecs/charstyped);
#endif
}

#else

EXPORT void
vedstartstats()
{
}

EXPORT void
vedstopstats()
{
}

EXPORT void
vedstatistics()
{
}

#endif	/* VED_STATS */
