/* @(#)wait3.c	1.21 16/08/10 Copyright 1995-2016 J. Schilling */
#undef	USE_LARGEFILES	/* XXX Temporärer Hack für Solaris */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)wait3.c	1.21 16/08/10 Copyright 1995-2016 J. Schilling";
#endif
/*
 * Compatibility function for BSD wait3().
 *
 * J"org Schilling (joerg@schily.isdn.cs.tu-berlin.de js@cs.tu-berlin.de)
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
 * Tries to get rusage information from /proc filesystem.
 * NOTE: since non root processes are not allowed to open suid procs
 * we cannot get complete rusage information in this case.
 *
 * Theory of Operation:
 *
 * On stock SVR4 there is no way to get resource usage information.
 * We may only get times information from siginfo struct:
 *
 * wait3()
 * {
 *	call waitid(,,,);
 *	if (child is found) {
 *		compute times from siginfo and fill in rusage
 *	}
 * }
 *
 * Solaris (at least 2.3) has PIOCUSAGE which is guaranteed
 * to work even on zombies:
 *
 * wait3()
 * {
 *	call waitid(P_ALL,,,options|WNOWAIT);
 *	if (child is found) {
 *		compute times from siginfo and fill in rusage
 *		if (can get /proc PIOCUSAGE info)
 *			fill in rest of rusage from /proc
 *		selective call waitid(P_PID, pid,,);
 * }
 *
 * /proc ioctl's that work on zombies:
 *	PIOCPSINFO, PIOCGETPR, PIOCUSAGE, PIOCLUSAGE
 *
 */
#define	wait3	__nothing__	/* Avoid collisions with prototype in wait.h */

/*
 * XXX SGI kann 64 bit resources,
 * XXX Solaris kann kein 64 bit proc file (LF32)
 */
#undef	USE_LARGEFILES	/* XXX Temporärer Hack für Solaris */

#ifdef	BSH
#	include <schily/mconfig.h>
#	include <schily/wait.h>
#else
#	include <wait.h>
#endif

#ifdef	UNIXWARE
/*
 * wait3() on SCO UnixWare is broken. It does not subtract the sum of the child
 * times of all the processes run before this one.
 */
#undef	HAVE_WAIT3
#endif

#ifdef	__sun
/*
 * wait3() exists on newer Solaris versions but doesnot fill more than ru_utime
 * and ru_stime. We fill anything but the rss related parts from procfs.
 */
#undef	HAVE_WAIT3
#endif

#ifdef	FORCE_OWN_WAIT3
#undef	HAVE_WAIT3
#endif

#ifdef	NO_OWN_WAIT3
#undef	WNOWAIT
#endif


#if	defined(HAVE_WAIT3) || !defined(HAVE_SYS_PROCFS_H) || !defined(HAVE_WAITID)
#undef	WNOWAIT
#endif

#ifdef	WNOWAIT		/* We are on SVR4 */
#define	DID_WAIT3

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/siginfo.h>
#ifdef	HAVE_SYS_PROCSET_H
#include <sys/procset.h>
#endif
#include <sys/param.h>
#include <sys/procfs.h>
#include <string.h>
#include <unistd.h>
#include <schily/resource.h>

#undef	wait3

static	int	wait_prusage(siginfo_t *, int, struct tms *, struct rusage *);
static	int	wait_status(int, int);
static	void	wait_times(siginfo_t *, struct rusage *);
	pid_t	wait3(int *status, int options, struct rusage *rusage);

pid_t
wait3(status, options, rusage)
		int	*status;
		int	options;
	struct	rusage	*rusage;
{
	siginfo_t	info;
	struct tms	tms_start;

	if (rusage)
		memset((void *)rusage, 0, sizeof (struct rusage));
	memset((void *)&info, 0, sizeof (siginfo_t));

#ifndef	HAVE_SI_UTIME
	times(&tms_start);
#endif
	/*
	 * BSD wait3() only supports WNOHANG & WUNTRACED
	 *
	 * You may want to modify the next two lines to meet your requirements:
	 * 1)	options &= (WNOHANG|WUNTRACED);
	 * 2a)	options |= (WEXITED|WSTOPPED|WTRAPPED);
	 * 2b)	options |= (WEXITED|WSTOPPED|WTRAPPED|WCONTINUED);
	 *
	 * If you want BSD compatibility use 1) and 2a)
	 * If you want maximum SYSV compatibility remove both lines.
	 */
	options &= (WNOHANG|WUNTRACED);
	options |= (WEXITED|WSTOPPED|WTRAPPED);
	if (waitid(P_ALL, 0, &info, options|WNOWAIT) < 0)
		return ((pid_t)-1);

	(void) wait_prusage(&info, options, &tms_start, rusage);
	if (status)
		*status = wait_status(info.si_code, info.si_status);
	return (info.si_pid);
}

static int
wait_prusage(info, options, tms_startp, rusage)
	siginfo_t	*info;
	int		options;
	struct rusage	*rusage;
	struct tms	*tms_startp;
{
#ifdef	PIOCUSAGE
	int		f;
	char		cproc[32];
	prusage_t	prusage;
#endif
#ifndef	HAVE_SI_UTIME
	struct  tms	tms_stop;	/* Cannot use siginfo, use times() */
#endif
	siginfo_t	info2;
	int		ret;

	if ((options & WNOHANG) && (info->si_pid == 0))
		return (0);	/* no children */

	if (rusage == 0)
		goto norusage;

	wait_times(info, rusage);

#ifdef	PIOCUSAGE
#ifdef	profs_2_COMMENT
	/*
	 * If Solaris removes PROCFS1 support we need to open and read this:
	 */
	sprintf(cproc, "/proc/%ld/usage", (long)info->si_pid);
#endif
	sprintf(cproc, "/proc/%ld", (long)info->si_pid);
	if ((f = open(cproc, 0)) < 0)
		goto norusage;
	if (ioctl(f, PIOCUSAGE, &prusage) < 0) {
		close(f);
		goto norusage;
	}
	close(f);
#ifdef	COMMENT
Missing fields:
	rusage->ru_maxrss = XXX;	/* maximum resident set size */
	rusage->ru_ixrss = XXX;		/* integral shared memory size */
	rusage->ru_idrss = XXX;		/* integral unshared data size */
	rusage->ru_isrss = XXX;		/* integral unshared stack size */
#endif
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
#endif
norusage:
	ret = waitid(P_PID, info->si_pid, &info2, options);

#ifndef	HAVE_SI_UTIME
	if (rusage) {
		times(&tms_stop);

		rusage->ru_utime.tv_sec  = (tms_stop.tms_cutime - tms_startp->tms_cutime) / HZ;
		rusage->ru_utime.tv_usec = ((tms_stop.tms_cutime - tms_startp->tms_cutime) % HZ) * (1000000/HZ);
		rusage->ru_stime.tv_sec  = (tms_stop.tms_cstime - tms_startp->tms_cstime) / HZ;
		rusage->ru_stime.tv_usec = ((tms_stop.tms_cstime - tms_startp->tms_cstime) % HZ) * (1000000/HZ);

	}
#endif
	return (ret);
}

/*
 * Convert the status code to old style wait status
 */
static int
wait_status(code, status)
	int	code;
	int	status;
{
	register int	stat = (status & 0377);

	switch (code) {

	case CLD_EXITED:
#if defined(BSH) && !defined(NO_STATUS_NULL_FIX)
		if (status != 0 && stat == 0)
			stat = 128;
#endif
		stat <<= 8;
		break;
	case CLD_KILLED:
		break;
	case CLD_DUMPED:
		stat |= WCOREFLG;
		break;
	case CLD_TRAPPED:
	case CLD_STOPPED:
		stat <<= 8;
		stat |= WSTOPFLG;
		break;
	case CLD_CONTINUED:
		stat = WCONTFLG;
		break;
	}
	return (stat);
}

/*
 * Convert the siginfo times to rusage timeval
 */
static void
wait_times(info, rusage)
	siginfo_t	*info;
	struct rusage	*rusage;
{
	int	hz = HZ;	/* HZ is mapped into sysconf(_SC_CLK_TCK) */

#ifdef	HAVE_SI_UTIME
	rusage->ru_utime.tv_sec = info->si_utime / hz;
	rusage->ru_utime.tv_usec = (info->si_utime % hz) * 1000000 / hz;

	rusage->ru_stime.tv_sec = info->si_stime / hz;
	rusage->ru_stime.tv_usec = (info->si_stime % hz) * 1000000 / hz;
#endif
}

#endif		/* WNOWAIT */

#if	!defined(HAVE_WAIT3) && !defined(DID_WAIT3)

/*
 * If we like to implement wait3() on top of other interfaces like wait4()
 * or waitpid(), the code goes here.
 */

#endif	/* !defined(HAVE_WAIT3) && !defined(DID_WAIT3) */
