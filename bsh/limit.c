/* @(#)limit.c	1.45 21/07/13 Copyright 1987-2021 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)limit.c	1.45 21/07/13 Copyright 1987-2021 J. Schilling";
#endif
/*
 *	Resource usage routines
 *
 *	Copyright (c) 1987-2021 J. Schilling
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
 * XXX SGI kann 64 bit resources,
 * XXX Solaris kann kein 64 bit proc file (LF32)
 * XXX daher ist getrusage() jetzt in getrusage.c
 */

#include <schily/stdio.h>
#include <schily/utypes.h>
#include <schily/unistd.h>
#include <schily/fcntl.h>
#include <schily/time.h>
#include <schily/resource.h>
#include "bsh.h"
#include "str.h"
#include "strsubs.h"


#	ifndef	HAVE_GETRLIMIT
#		define	getrlimit	_getrlimit
#		endif
#	ifndef	HAVE_SETRLIMIT
#		define	setrlimit	_setrlimit
#	endif
#include "limit.h"


#ifndef	RLIM_INFINITY
#define	RLIM_INFINITY	0x7FFFFFFF
#endif

typedef struct {
	char	*l_name;
	int	l_which;
	int	l_factor;
	char	*l_scale;
} LIMIT;

LIMIT	limits[] = {
#ifdef	RLIMIT_CPU
	{	"cputime",	RLIMIT_CPU,	1,	"seconds"	},
#endif
#ifdef	RLIMIT_FSIZE
	{	"filesize",	RLIMIT_FSIZE,	1024,	"kBytes"	},
#endif
#ifdef	RLIMIT_DATA
	{	"datasize",	RLIMIT_DATA,	1024,	"kBytes"	},
#endif
#ifdef	RLIMIT_STACK
	{	"stacksize",	RLIMIT_STACK,	1024,	"kBytes"	},
#endif
#ifdef	RLIMIT_CORE
	{	"coredumpsize",	RLIMIT_CORE,	1024,	"kBytes"	},
#endif
#ifdef	RLIMIT_RSS
	{	"memoryuse",	RLIMIT_RSS,	1024,	"kBytes"	},
#endif
#if	defined(RLIMIT_UMEM) && !!defined(RLIMIT_RSS)	/* SUPER UX */
	{	"memoryuse",	RLIMIT_UMEM,	1024,	"kBytes"	},
#endif
#ifdef	RLIMIT_NOFILE
	{	"descriptors",	RLIMIT_NOFILE,	1,	""		},
#endif
#if	defined(RLIMIT_OFILE) && !defined(RLIMIT_NOFILE)
	{	"descriptors",	RLIMIT_OFILE,	1,	""		},
#endif
#ifdef	RLIMIT_VMEM
	{	"vmemsize",	RLIMIT_VMEM,	1024,	"kBytes"	},
#endif
#if	defined(RLIMIT_AS) && !defined(RLIMIT_VMEM)
	{	"vmemsize",	RLIMIT_AS,	1024,	"kBytes"	},
#endif
#ifdef	RLIMIT_HEAP	/* BS2000/OSD */
	{	"heapsize",	RLIMIT_HEAP,	1024,	"kBytes"	},
#endif
#ifdef	RLIMIT_CONCUR	/* CONVEX max. # of processors per process */
	{	"concurrency",	RLIMIT_CONCUR,	1,	"thread(s)"	},
#endif
#ifdef	RLIMIT_NPROC
	{	"nproc",	RLIMIT_NPROC,	1,	""		},
#endif
#ifdef	RLIMIT_MEMLOCK
	{	"memorylocked",	RLIMIT_MEMLOCK,	1024,	"kBytes"	},
#endif
#ifdef	RLIMIT_LOCKS
	{	"filelocks",	RLIMIT_LOCKS,	1,	""		},
#endif
#ifdef	RLIMIT_SIGPENDING
	{	"sigpending",	RLIMIT_SIGPENDING, 1,	""		},
#endif
#ifdef	RLIMIT_MSGQUEUE
	{	"msgqueues",	RLIMIT_MSGQUEUE, 1024,	"kBytes"	},
#endif
	/* Nice levels 19 .. -20 correspond to 0 .. 39 */
#ifdef	RLIMIT_NICE
	{	"maxnice",	RLIMIT_NICE,	1,	""		},
#endif
#ifdef	RLIMIT_RTPRIO
	{	"maxrtprio",	RLIMIT_RTPRIO,	1,	""		},
#endif
#ifdef	RLIMIT_RTTIME
	{	"maxrttime",	RLIMIT_RTTIME,	1,	"usec(s)"	},
#endif
#ifdef	RLIMIT_SBSIZE	/* FreeBSD maximum size of all socket buffers */
	{	"sbsize",	RLIMIT_SBSIZE,	1,	""		},
#endif
#ifdef	RLIMIT_NPTS	/* FreeBSD maximum # of pty's */
	{	"npts",		RLIMIT_NPTS,	1,	""		},
#endif
#ifdef	RLIMIT_SWAP	/* FreeBSD swap used */
	{	"swap",		RLIMIT_SWAP,	1024,	"kBytes"	},
#endif
#ifdef	RLIMIT_KQUEUES	/* FreeBSD kqueues allocated */
	{	"kqueues",	RLIMIT_KQUEUES,	1,	""	},
#endif
#ifdef	RLIMIT_UMTXP	/* FreeBSD process-shared umtx */
	{	"umtx shared locks", RLIMIT_UMTXP, 1,	""	},
#endif
};

int	nlimits = sizeof (limits)/sizeof (limits[0]);

char	unlimited[] = "unlimited";

struct timeval	starttime;
struct timeval	pt;
long		pagesize;

EXPORT	void	blimit		__PR((Argvec * vp, FILE ** std, int flag));
LOCAL	LIMIT	*limwhich	__PR((FILE ** std, char *s));
LOCAL	BOOL	getllval	__PR((FILE ** std, LIMIT * limit, char *s, Ullong *llp));
LOCAL	BOOL	getlimit	__PR((FILE ** std, LIMIT * limit, struct rlimit *limp));
LOCAL	BOOL	setlimit	__PR((FILE ** std, LIMIT * limit, struct rlimit *limp));
LOCAL	void	printlimit	__PR((FILE ** std, LIMIT * limit, struct rlimit *limp));
EXPORT	void	inittime	__PR((void));
EXPORT	void	setstime	__PR((void));
EXPORT	void	prtime		__PR((FILE ** std, long sec, long usec));
EXPORT	void	getpruself	__PR((struct rusage *prusage));
EXPORT	void	getpruchld	__PR((struct rusage *prusage));
EXPORT	void	btime		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	prtimes		__PR((FILE ** std, struct rusage *prusage));
LOCAL	void	prtm		__PR((FILE ** std, struct rusage *prusage, struct timeval *stt));
EXPORT	void	rusagesub	__PR((struct rusage *pru1, struct rusage *pru2));
EXPORT	void	rusageadd	__PR((struct rusage *pru1, struct rusage *pru2));

/* ARGSUSED */
EXPORT void
blimit(vp, std, flag)
	register	Argvec	*vp;
			FILE	*std[];
			int	flag;
{
	register int	i;
	register LIMIT	*limit;
	struct rlimit	lim;
	Ullong		l_cur;
	Ullong		l_max;

	if (vp->av_ac > 4) {
		wrong_args(vp, std);
		return;
	}
	if (vp->av_ac == 1) {
		for (i = 0, limit = limits; i < nlimits; i++, limit++) {
			if (getlimit(std, limit, &lim))
				printlimit(std, limit, &lim);
		}
	} else if (vp->av_ac >= 2) {
		if ((limit = limwhich(std, vp->av_av[1])) == (LIMIT *)NULL)
			return;
		if (!getlimit(std, limit, &lim))
			return;
		l_cur = lim.rlim_cur;
		l_max = lim.rlim_max;
		if (vp->av_ac == 2)
			printlimit(std, limit, &lim);
		else if (!getllval(std, limit, vp->av_av[2], &l_cur))
			return;
		else if (vp->av_ac == 4 &&
			!getllval(std, limit, vp->av_av[3], &l_max))
			return;
		else {
			lim.rlim_cur = l_cur;
			lim.rlim_max = l_max;
			if (lim.rlim_cur == RLIM_INFINITY && geteuid() != 0)
				lim.rlim_cur = lim.rlim_max;
			setlimit(std, limit, &lim);
		}
	}
}

LOCAL LIMIT *
limwhich(std, s)
	FILE	*std[];
	char	*s;
{
	register int	i;
	register LIMIT	*limit;
	register LIMIT	*flimit	= (LIMIT *)NULL;
	register BOOL	found	= FALSE;

	for (i = 0, limit = limits; i < nlimits; i++, limit++) {
		if (strbeg(s, limit->l_name)) {
			if (found) {
				fprintf(std[2], "%s\n", eambiguous);
				return ((LIMIT *)NULL);
			}
			flimit = limit;
			found = TRUE;
		}
	}
	if (found)
		return (flimit);
	fprintf(std[2], "No such limit.\n");
	ex_status = 1;
	return ((LIMIT *) NULL);
}

LOCAL BOOL
getllval(std, limit, s, llp)
	FILE	*std[];
	LIMIT	*limit;
	char	*s;
	Ullong	*llp;
{
	Llong	ll;

	if (strbeg(s, unlimited)) {
		*llp = RLIM_INFINITY;
		return (TRUE);
	}
	if (!tollong(std, s, &ll))
		return (FALSE);
	*llp = ll * limit->l_factor;
	return (TRUE);
}

LOCAL BOOL
getlimit(std, limit, limp)
		FILE	*std[];
		LIMIT	*limit;
	struct rlimit	*limp;
{
	if (getrlimit(limit->l_which, limp) < 0) {
		ex_status = geterrno();
		fprintf(std[2], "Can't get %s limit. %s\n",
				limit->l_name, errstr(ex_status));
		return (FALSE);
	}
	return (TRUE);
}

LOCAL BOOL
setlimit(std, limit, limp)
		FILE	*std[];
		LIMIT	*limit;
	struct rlimit	*limp;
{
	if (setrlimit(limit->l_which, limp) < 0) {
		ex_status = geterrno();
		fprintf(std[2], "Can't set %s limit. %s\n",
				limit->l_name, errstr(ex_status));
		return (FALSE);
	}
	return (TRUE);
}

LOCAL void
printlimit(std, limit, limp)
		FILE	*std[];
	register LIMIT	*limit;
	register struct	rlimit	*limp;
{

	fprintf(std[1], "%-14s", limit->l_name);

	if (limp->rlim_cur == RLIM_INFINITY)
		fprintf(std[1], "%14s\t", unlimited);
#ifdef	RLIMIT_CPU
	else if (limit->l_which == RLIMIT_CPU) {
		prtime(std, (long)limp->rlim_cur, 0L);
		fprintf(std[1], "\t");
	}
#endif
	else {
		fprintf(std[1], "%-7ld %6s\t",
			(long)(limp->rlim_cur/limit->l_factor), limit->l_scale);
	}
	if (limp->rlim_max == RLIM_INFINITY)
		fprintf(std[1], "%14s\n", unlimited);
#ifdef	RLIMIT_CPU
	else if (limit->l_which == RLIMIT_CPU) {
		prtime(std, (long)limp->rlim_max, 0L);
		fprintf(std[1], "\n");
	}
#endif
	else {
		fprintf(std[1], "%-7ld %6s\n",
			(long)(limp->rlim_max/limit->l_factor), limit->l_scale);
	}
}

#ifndef	HAVE_GETRLIMIT
getrlimit(which, limp)
		int	which;
	struct rlimit	*limp;
{
	int	val;

	switch (which) {

	case RLIMIT_FSIZE:
#ifdef	HAVE_ULIMIT
			if ((val = ulimit(1, 0)) < 0)
				return (-1);
			limp->rlim_cur = limp->rlim_max = val * 512;
			break;
#else
			seterrno(EINVAL);
			return (-1);
#endif
	default:	limp->rlim_cur = limp->rlim_max = RLIM_INFINITY;
	}
	return (0);
}
#endif

#ifndef	HAVE_SETRLIMIT
setrlimit(which, limp)
		int	which;
	struct rlimit	*limp;
{
	switch (which) {

	case RLIMIT_FSIZE:
#ifdef	HAVE_ULIMIT
			return (ulimit(2, limp->rlim_cur/512));
#else
			break;
#endif
	}
	seterrno(EINVAL);
	return (-1);
}
#endif


EXPORT void
inittime()
{
	gettimeofday(&starttime, (struct timezone *)0);

#ifdef	_SC_PAGESIZE
	pagesize = sysconf(_SC_PAGESIZE)/1024;
#else
	pagesize = getpagesize()/1024;
#endif
	if (pagesize == 0)
		pagesize = 1;
}

EXPORT void
setstime()
{
	gettimeofday(&pt, (struct timezone *)0);
}

EXPORT void
prtime(std, sec, usec)
	FILE	*std[];
	long	sec;
	long	usec;
{
	long	hour;
	long	min;

	hour = sec/3600;
	min = sec/60%60;
	sec %= 60;
	if (hour)
		fprintf(std[1], "%ld:%02ld:%02ld", hour, min, sec);
	else if (min)
		fprintf(std[1], "%ld:%02ld", min, sec);
	else
		fprintf(std[1], "%ld", sec);
	if (usec)
		fprintf(std[1], ".%03ld", usec/1000);
}

#if defined(HAVE_GETRUSAGE) || defined(PIOCUSAGE)

EXPORT void
getpruself(prusage)
	struct rusage *prusage;
{
	getrusage(RUSAGE_SELF, prusage);
}

EXPORT void
getpruchld(prusage)
	struct rusage *prusage;
{
	getrusage(RUSAGE_CHILDREN, prusage);
}

/* ARGSUSED */
EXPORT void
btime(vp, std, flag)
	register	Argvec	*vp;
			FILE	*std[];
			int	flag;
{
/*	struct	timeval	stoptime;*/
	struct	rusage	rusage;

	if (vp->av_ac == 1) {
		getrusage(RUSAGE_SELF, &rusage);
/*		gettimeofday(&stoptime, (struct timezone *)0);*/
		prtm(std, &rusage, &starttime);

		getrusage(RUSAGE_CHILDREN, &rusage);
		prtm(std, &rusage, &starttime);
	}
}

#else

EXPORT void
getpruself(prusage)
	struct rusage *prusage;
{
	fillbytes(&prusage, sizeof (*prusage), '\0');
}

EXPORT void
getpruchld(prusage)
	struct rusage *prusage;
{
	fillbytes(&prusage, sizeof (*prusage), '\0');
}

/* ARGSUSED */
EXPORT void
btime(vp, std, flag)
	register	Argvec	*vp;
			FILE	*std[];
			int	flag;
{
	unimplemented(vp, std);
}
#endif	/* ! defined(HAVE_GETRUSAGE) || defined(PIOCUSAGE) */

EXPORT void
prtimes(std, prusage)
		FILE	*std[];
	struct rusage *prusage;
{
	prtm(std, prusage, &pt);
}

LOCAL void
prtm(std, prusage, stt)
	FILE	*std[];
	register struct rusage *prusage;
	register struct timeval *stt;
{
	long	cpu;
	long	sec;
	long	usec;
	long	tmsec;
	long	tics;
	struct	timeval	stoptime;

	gettimeofday(&stoptime, (struct timezone *)0);
	sec = stoptime.tv_sec - stt->tv_sec;
	usec = stoptime.tv_usec - stt->tv_usec;
	tmsec = sec * 1000 + usec/1000;
	if (tmsec < 100)		/* avoid floating exception */
		tmsec = 100;
	if (usec < 0) {
		sec--;
		usec += 1000000;
	}
	cpu = prusage->ru_utime.tv_sec*1000 + prusage->ru_utime.tv_usec/1000;
	cpu += prusage->ru_stime.tv_sec*1000 + prusage->ru_stime.tv_usec/1000;

	tics = cpu/(1000/50);
	tics /= pagesize;		/* for ixrss & idrss		*/
	if (tics == 0)			/* avoid floating exception	*/
		tics = 1;

	prtime(std, sec, usec|1);	/* print usec always */

#if defined(__BEOS__) || defined(__HAIKU__) || \
    defined(OS390) || defined(__MVS__)
	/* XXX dirty hack */
	fprintf(std[1],
		"r %ld.%03ldu %ld.%03lds %ld%%\n",
		(long)prusage->ru_utime.tv_sec,
		(long)prusage->ru_utime.tv_usec/1000,
		(long)prusage->ru_stime.tv_sec,
		(long)prusage->ru_stime.tv_usec/1000,
		cpu/(tmsec/100));
#else
	fprintf(std[1],
		"r %ld.%03ldu %ld.%03lds %ld%% %ldM %ld+%ldk %ldst %ld+%ldio %ldpf+%ldw\n",
		(long)prusage->ru_utime.tv_sec,
		(long)prusage->ru_utime.tv_usec/1000,
		(long)prusage->ru_stime.tv_sec,
		(long)prusage->ru_stime.tv_usec/1000,
		cpu/(tmsec/100),
		prusage->ru_maxrss*pagesize,
		prusage->ru_ixrss/tics, /* tics contains pagesize	*/
		prusage->ru_idrss/tics, /* tics contains pagesize	*/
		prusage->ru_isrss/tics, /* tics contains pagesize	*/
		prusage->ru_inblock,
		prusage->ru_oublock,
		prusage->ru_majflt,
		prusage->ru_nswap);
#endif
}

EXPORT void
rusagesub(pru1, pru2)
	register struct rusage *pru1;
	register struct rusage *pru2;
{
	pru2->ru_utime.tv_sec	-= pru1->ru_utime.tv_sec;
	pru2->ru_utime.tv_usec	-= pru1->ru_utime.tv_usec;
	if (pru2->ru_utime.tv_usec < 0) {
		pru2->ru_utime.tv_sec -= 1;
		pru2->ru_utime.tv_usec += 1000000;
	}
	pru2->ru_stime.tv_sec	-= pru1->ru_stime.tv_sec;
	pru2->ru_stime.tv_usec	-= pru1->ru_stime.tv_usec;
	if (pru2->ru_stime.tv_usec < 0) {
		pru2->ru_stime.tv_sec -= 1;
		pru2->ru_stime.tv_usec += 1000000;
	}
#if defined(__BEOS__) || defined(__HAIKU__) || \
    defined(OS390) || defined(__MVS__)
	/* XXX dirty hack */
#else
	pru2->ru_maxrss		-= pru1->ru_maxrss;
	pru2->ru_ixrss		-= pru1->ru_ixrss;
	pru2->ru_idrss		-= pru1->ru_idrss;
	pru2->ru_isrss		-= pru1->ru_isrss;
	pru2->ru_inblock	-= pru1->ru_inblock;
	pru2->ru_oublock	-= pru1->ru_oublock;
	pru2->ru_majflt		-= pru1->ru_majflt;
	pru2->ru_nswap		-= pru1->ru_nswap;
#endif
}

EXPORT void
rusageadd(pru1, pru2)
	register struct rusage *pru1;
	register struct rusage *pru2;
{
	pru2->ru_utime.tv_sec	+= pru1->ru_utime.tv_sec;
	pru2->ru_utime.tv_usec	+= pru1->ru_utime.tv_usec;
	if (pru2->ru_utime.tv_usec >= 1000000) {
		pru2->ru_utime.tv_sec += 1;
		pru2->ru_utime.tv_usec -= 1000000;
	}
	pru2->ru_stime.tv_sec	+= pru1->ru_stime.tv_sec;
	pru2->ru_stime.tv_usec	+= pru1->ru_stime.tv_usec;
	if (pru2->ru_stime.tv_usec >= 1000000) {
		pru2->ru_stime.tv_sec += 1;
		pru2->ru_stime.tv_usec -= 1000000;
	}
#if defined(__BEOS__) || defined(__HAIKU__) || \
    defined(OS390) || defined(__MVS__)
	/* XXX dirty hack */
#else
	pru2->ru_maxrss		+= pru1->ru_maxrss;
	pru2->ru_ixrss		+= pru1->ru_ixrss;
	pru2->ru_idrss		+= pru1->ru_idrss;
	pru2->ru_isrss		+= pru1->ru_isrss;
	pru2->ru_inblock	+= pru1->ru_inblock;
	pru2->ru_oublock	+= pru1->ru_oublock;
	pru2->ru_majflt		+= pru1->ru_majflt;
	pru2->ru_nswap		+= pru1->ru_nswap;
#endif
}
