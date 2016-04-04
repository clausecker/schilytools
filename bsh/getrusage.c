/* @(#)getrusage.c	1.30 16/04/04 Copyright 1987-2016 J. Schilling */
#undef	USE_LARGEFILES	/* XXX Temporärer Hack für Solaris */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)getrusage.c	1.30 16/04/04 Copyright 1987-2016 J. Schilling";
#endif
/*
 *	getrusage() emulation for SVr4
 *
 *	Copyright (c) 1987-2016 J. Schilling
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

/*
 * XXX Solaris kann kein 64 bit proc file (LF32)
 */
#undef	USE_LARGEFILES	/* XXX Temporärer Hack für Solaris */

#include <schily/stdio.h>
#include "bsh.h"
#include <schily/unistd.h>
#include <schily/fcntl.h>
#include <schily/time.h>
#include <schily/resource.h>
#include <schily/procfs.h>
#include <schily/times.h>
#include "limit.h"


/*#undef	HAVE_GETRUSAGE*/

#ifndef	HAVE_GETRUSAGE
EXPORT	int	getrusage	__PR((int who, struct rusage *rusage));
#endif

#ifndef	HAVE_GETRUSAGE
/*
 * XXX who wird nicht untersteutzt!
 */
EXPORT int
getrusage(who,  rusage)
	int		who;
	struct rusage	*rusage;
{
#if	defined(PIOCUSAGE) || _STRUCTURED_PROC == 1
	int		f;
	char		cproc[32];
	prusage_t	prusage;
#endif
#if _STRUCTURED_PROC == 1
	pstatus_t	pstatus;
#endif

	if (rusage)
		fillbytes((void *)rusage, sizeof (struct rusage), 0);

#if	defined(PIOCUSAGE) || _STRUCTURED_PROC == 1
#define	DID_RUSAGE

#if _STRUCTURED_PROC == 1
	if (who == RUSAGE_CHILDREN)
		sprintf(cproc, "/proc/%ld/status", (long)getpid());
	else
		sprintf(cproc, "/proc/%ld/usage", (long)getpid());
	if ((f = open(cproc, 0)) < 0)
		return (-1);
	if (who == RUSAGE_CHILDREN) {
		if (read(f, &pstatus, sizeof (pstatus)) < 0) {
			close(f);
			return (-1);
		}
	} else {
		if (read(f, &prusage, sizeof (prusage)) < 0) {
			close(f);
			return (-1);
		}
	}
#else
	if (who == RUSAGE_CHILDREN)
		return (-1);
	sprintf(cproc, "/proc/%ld", (long)getpid());
	if ((f = open(cproc, 0)) < 0)
		return (-1);
	if (ioctl(f, PIOCUSAGE, &prusage) < 0) {
		close(f);
		return (-1);
	}
#endif
	close(f);
	if (rusage) {
#if _STRUCTURED_PROC == 1
		if (who == RUSAGE_CHILDREN) {
			rusage->ru_utime.tv_sec =  pstatus.pr_cutime.tv_sec;
			rusage->ru_utime.tv_usec = pstatus.pr_cutime.tv_nsec/1000;
			rusage->ru_stime.tv_sec =  pstatus.pr_cstime.tv_sec;
			rusage->ru_stime.tv_usec = pstatus.pr_cstime.tv_nsec/1000;
			return (0);
		}
#endif
		rusage->ru_utime.tv_sec =  prusage.pr_utime.tv_sec;
		rusage->ru_utime.tv_usec = prusage.pr_utime.tv_nsec/1000;
		rusage->ru_stime.tv_sec =  prusage.pr_stime.tv_sec;
		rusage->ru_stime.tv_usec = prusage.pr_stime.tv_nsec/1000;

/* Missing fields:			*/
/*		rusage->ru_maxrss = XXX;*/
/*		rusage->ru_ixrss = XXX;*/
/*		rusage->ru_idrss = XXX;*/
/*		rusage->ru_isrss = XXX;*/

		rusage->ru_minflt = prusage.pr_minf;
		rusage->ru_majflt = prusage.pr_majf;
		rusage->ru_nswap  = prusage.pr_nswap;
		rusage->ru_inblock = prusage.pr_inblk;
		rusage->ru_oublock = prusage.pr_oublk;
		rusage->ru_msgsnd = prusage.pr_msnd;
		rusage->ru_msgrcv = prusage.pr_mrcv;
		rusage->ru_nsignals = prusage.pr_sigs;
		rusage->ru_nvcsw = prusage.pr_vctx;
		rusage->ru_nivcsw = prusage.pr_ictx;
	}
#endif	/* PIOCUSAGE */
#if	!defined(DID_RUSAGE) && defined(HAVE_TIMES)
#define	DID_RUSAGE
#define	NEED_CLOCK2TV
	{
		struct tms	tms;
	LOCAL	void clock2tv	__PR((clock_t t, struct timeval *tp));

		times(&tms);

		if (who == RUSAGE_SELF) {
			clock2tv(tms.tms_utime, &rusage->ru_utime);
			clock2tv(tms.tms_stime, &rusage->ru_stime);
		} else if (who == RUSAGE_CHILDREN) {
			clock2tv(tms.tms_cutime, &rusage->ru_utime);
			clock2tv(tms.tms_cstime, &rusage->ru_stime);
		}
	}
#endif
	return (0);
}

#ifdef	NEED_CLOCK2TV
#ifndef	HZ
#define	HZ	sysconf(_SC_CLK_TCK)
#endif

LOCAL void
clock2tv(t, tp)
	clock_t		t;
	struct timeval	*tp;
{
	int _hz = HZ;	/* HZ may be a macro to a sysconf() call */

	tp->tv_sec = t / _hz;
	tp->tv_usec = t % _hz;
	if (_hz <= 1000000)
		tp->tv_usec *= 1000000 / _hz;
	else
		tp->tv_usec /= _hz / 1000000;
}
#endif

#endif	/* HAVE_GETRUSAGE */
