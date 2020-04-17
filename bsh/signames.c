/* @(#)signames.c	1.25 20/04/13 Copyright 1998-2020 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)signames.c	1.25 20/04/13 Copyright 1998-2020 J. Schilling";
#endif
/*
 *	Handle signal names for systems that don't have
 *	strsignal()/str2sig()/sig2str()
 *
 *	Copyright (c) 1998-2020 J. Schilling
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

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#ifdef	USE_JS_BOOL			/* #define USE_JS_BOOL if there is a */
#define	BOOL	JS_BOOL			/* different (incompatible) BOOL in  */
#endif					/* the using code, e.g. Boune Shell  */
#include <schily/standard.h>
#include <schily/signal.h>
#include <schily/schily.h>
#ifdef	USE_JS_BOOL			/* If in workaround mode, */
#undef	BOOL				/* revert to default BOOL */
#endif

#ifdef	FORCE_SIGNAMES
#undef	HAVE_STRSIGNAL
#undef	HAVE_STR2SIG
#undef	HAVE_SIG2STR
#endif

#if	!(defined(HAVE_STRSIGNAL) && defined(HAVE_STR2SIG) && \
		defined(HAVE_SIG2STR))

/*
 * Linux uses __SIGRTMIN instead of _SIGRTMIN.
 */
#ifndef	_SIGRTMIN
#ifdef	__SIGRTMIN
#define	_SIGRTMIN	__SIGRTMIN
#endif
#endif
#ifndef	_SIGRTMAX
#ifdef	__SIGRTMAX
#define	_SIGRTMAX	__SIGRTMAX
#endif
#endif

#if defined(__FreeBSD__) || defined(__NetBSD__)
#ifndef	_SIGRTMIN
#ifdef	SIGRTMIN
#define	_SIGRTMIN	SIGRTMIN
#endif
#endif
#ifndef	_SIGRTMAX
#ifdef	SIGRTMAX
#define	_SIGRTMAX	SIGRTMAX
#endif
#endif
#endif

#ifndef	HAVE_STRSIGNAL
EXPORT	char	*strsignal	__PR((int sig));
#endif
#ifndef	HAVE_STR2SIG
EXPORT	int	str2sig		__PR((const char *s, int *sigp));
#endif
#ifndef	HAVE_SIG2STR
EXPORT	int	sig2str		__PR((int sig, char *s));
#endif
#ifdef	_SIGRTMIN
LOCAL	void	_rtsiginit	__PR((void));
#endif

#ifdef	MAIN
main()
{
	int i;
	char	*t;
	char	n[32];
	int	s;

	for (i = 0; i < NSIG+6; i++) {
		if ((t = strsignal(i)) == 0)
			t = "NULL";
		if (sig2str(i, n) < 0)
			strcpy(n, "XXX");
		if (str2sig(n, &s) < 0)
			s = -2;

		printf("SIG: %d %s	(%d)	%s\n", i, n, s, t);
	}
}
#endif

/*
 * Let "EXIT" be first, to make the shell trap(1) command work as expected.
 */
LOCAL struct signames {
	int	signo;
	char	*signame;
	char	*sigtext;
} signames[] = {
	{ 0,		"EXIT",		"Unknown Signal", },
	{ 0,		"NULL",		"Null Signal", },
#ifdef	SIGNULL
	{ SIGNULL,	"NULL",		"Null Signal", },
#endif
#ifdef	SIGHUP
	{ SIGHUP,	"HUP",		"Hangup", },
#endif
#ifdef	SIGINT
	{ SIGINT,	"INT",		"Interrupt", },
#endif
#ifdef	SIGQUIT
	{ SIGQUIT,	"QUIT",		"Quit", },
#endif
#ifdef	SIGILL
	{ SIGILL,	"ILL",		"Illegal Instruction", },
#endif
#ifdef	SIGTRAP
	{ SIGTRAP,	"TRAP",		"Trace/Breakpoint Trap", },
#endif
#ifdef	SIGABRT
	{ SIGABRT,	"ABRT",		"IOT trap/Abort", },
#endif
#ifdef	SIGIOT
	{ SIGIOT,	"IOT",		"IOT trap/Abort", },
#endif
#ifdef	SIGEMT
	{ SIGEMT,	"EMT",		"Emulation Trap", },
#endif
#ifdef	SIGFPE
	{ SIGFPE,	"FPE",		"Arithmetic Exception", },
#endif
#ifdef	SIGKILL
	{ SIGKILL,	"KILL",		"Killed", },
#endif
#ifdef	SIGBUS
	{ SIGBUS,	"BUS",		"Bus Error", },
#endif
#ifdef	SIGSEGV
	{ SIGSEGV,	"SEGV",		"Segmentation Fault", },
#endif
#ifdef	SIGSYS
	{ SIGSYS,	"SYS",		"Bad System Call", },
#endif
#ifdef	SIGPIPE
	{ SIGPIPE,	"PIPE",		"Broken Pipe", },
#endif
#ifdef	SIGALRM
	{ SIGALRM,	"ALRM",		"Alarm Clock", },
#endif
#ifdef	SIGTERM
	{ SIGTERM,	"TERM",		"Terminated", },
#endif
#ifdef	SIGUSR1
	{ SIGUSR1,	"USR1",		"User defined Signal 1", },
#endif
#ifdef	SIGUSR2
	{ SIGUSR2,	"USR2",		"User defined Signal 2", },
#endif
#ifdef	SIGSTKFLT
	{ SIGSTKFLT,	"STKFLT",	"Stack Fault", },
#endif
#ifdef	SIGCHLD		/* POSIX name should appear first */
	{ SIGCHLD,	"CHLD",		"Child Status Changed", },
#endif
#ifdef	SIGCLD
	{ SIGCLD,	"CLD",		"Child Status Changed", },
#endif
#ifdef	SIGPWR
	{ SIGPWR,	"PWR",		"Power-Fail/Restart", },
#endif
#ifdef	SIGWINCH
	{ SIGWINCH,	"WINCH",	"Window Size Change", },
#endif
#ifdef	SIGURG
	{ SIGURG,	"URG",		"Urgent Socket Condition", },
#endif
#ifdef	SIGPOLL
	{ SIGPOLL,	"POLL",		"Pollable Event", },
#endif
#ifdef	SIGIO
	{ SIGIO,	"IO",		"Possible I/O", },
#endif
#ifdef	SIGLOST
	{ SIGLOST,	"LOST",		"Resource Lost", },
#endif
#ifdef	SIGSTOP
	{ SIGSTOP,	"STOP",		"Stopped (signal)", },
#endif
#ifdef	SIGTSTP
	{ SIGTSTP,	"TSTP",		"Stopped (user)", },
#endif
#ifdef	SIGCONT
	{ SIGCONT,	"CONT",		"Continued", },
#endif
#ifdef	SIGTTIN
	{ SIGTTIN,	"TTIN",		"Stopped (tty input)", },
#endif
#ifdef	SIGTTOU
	{ SIGTTOU,	"TTOU",		"Stopped (tty output)", },
#endif
#ifdef	SIGVTALRM
	{ SIGVTALRM,	"VTALRM",	"Virtual Timer Expired", },
#endif
#ifdef	SIGPROF
	{ SIGPROF,	"PROF",		"Profiling Timer Expired", },
#endif
#ifdef	SIGXCPU
	{ SIGXCPU,	"XCPU",		"Cpu Time Limit Exceeded", },
#endif
#ifdef	SIGXFSZ
	{ SIGXFSZ,	"XFSZ",		"File Size Limit Exceeded", },
#endif
#ifdef	SIGWAITING
	{ SIGWAITING,	"WAITING",	"No runnable lwp", },
#endif
#ifdef	SIGLWP
	{ SIGLWP,	"LWP",		"Inter-lwp signal", },
#endif
#ifdef	SIGFREEZE
	{ SIGFREEZE,	"FREEZE",	"Checkpoint Freeze", },
#endif
#ifdef	SIGTHAW
	{ SIGTHAW,	"THAW",		"Checkpoint Thaw", },
#endif
#ifdef	SIGCANCEL
	{ SIGCANCEL,	"CANCEL",	"Thread Cancellation", },
#endif
#ifdef	SIGXRES
	{ SIGXRES,	"XRES",		"Resource Control Exceeded", },
#endif
#ifdef	SIGJVM1
	{ SIGJVM1,	"JVM1",		"Reserved for JVM 1", },
#endif
#ifdef	SIGJVM2
	{ SIGJVM2,	"JVM2",		"Reserved for JVM 2", },
#endif
#ifdef	SIGUNUSED
	{ SIGUNUSED,	"UNUSED",	"Unused Signal", },
#endif
#ifdef	SIGDGTIMER1
	{ SIGDGTIMER1,	"DGTIMER1",	"DG Timer 1", },
#endif
#ifdef	SIGDGTIMER2
	{ SIGDGTIMER2,	"DGTIMER2",	"DG Timer 2", },
#endif
#ifdef	SIGDGTIMER3
	{ SIGDGTIMER3,	"DGTIMER3",	"DG Timer 3", },
#endif
#ifdef	SIGDGTIMER4
	{ SIGDGTIMER4,	"DGTIMER4",	"DG Timer 4", },
#endif
#ifdef	SIGDGNOTIFY
	{ SIGDGNOTIFY,	"DGNOTIFY",	"DG Notify", },
#endif
#ifdef	SIGINFO
	{ SIGINFO,	"INFO",		"Information Request", },
#endif
#ifdef	SIGDIL	/* HP-UX */
	{ SIGDIL,	"DIL",		"DIL Signal", },
#endif
#ifdef	SIGAIO
	{ SIGAIO,	"AIO",		"Asynchronous I/O", },
#endif
#ifdef	SIGALRM1
	{ SIGALRM1,	"ALRM1",	"Scheduling - reserved", },
#endif
#ifdef	SIGAPOLLO
	{ SIGAPOLLO,	"APOLLO",	"SIGAPOLLO", },
#endif
#ifdef	SIGCPUFAIL
	{ SIGCPUFAIL,	"CPUFAIL",
				"Predictive processor deconfiguration", },
#endif
#ifdef	SIGDANGER
	{ SIGDANGER,	"DANGER",	"System crash soon", },
#endif
#ifdef	SIGERR
	{ SIGERR,	"ERR",		"", },
#endif
#ifdef	SIGGRANT
	{ SIGGRANT,	"GRANT",	"Grant monitor mode", },
#endif
#ifdef	SIGLAB
	{ SIGLAB,	"LAB",		"Security label changed", },
#endif
#ifdef	SIGMIGRATE
	{ SIGMIGRATE,	"MIGRATE",	"Migrate process", },
#endif
#ifdef	SIGMSG
	{ SIGMSG,	"MSG",		"Ring buffer input data", },
#endif
#ifdef	SIGPHONE
	{ SIGPHONE,	"PHONE",	"Phone interrupt", },
#endif
#ifdef	SIGPRE
	{ SIGPRE,	"PRE",		"Programming exception", },
#endif
#ifdef	SIGRETRACT
	{ SIGRETRACT,	"RETRACT",	"Relinquish monitor mode", },
#endif
#ifdef	SIGSAK
	{ SIGSAK,	"SAK",		"Secure attention key", },
#endif
#ifdef	SIGSOUND
	{ SIGSOUND,	"SOUND",	"Sound completed", },
#endif
#ifdef	SIGTINT
	{ SIGTINT,	"TINT",		"Interrupt", },
#endif
#ifdef	SIGVIRT
	{ SIGVIRT,	"VIRT",		"Virtual timer alarm", },
#endif
	{ 0,	0,	0, },
};

#ifdef	_SIGRTMIN
LOCAL struct signames rtsignames[] = {
#ifdef	_SIGRTMIN
	{ _SIGRTMIN,	"RTMIN",	"First Realtime Signal", },
	{ _SIGRTMIN+1,	"RTMIN+1",	"Second Realtime Signal", },
	{ _SIGRTMIN+2,	"RTMIN+2",	"Third Realtime Signal", },
	{ _SIGRTMIN+3,	"RTMIN+3",	"Fourth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 8
	{ _SIGRTMIN+4,	"RTMIN+4",	"Fifth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 10
	{ _SIGRTMIN+5,	"RTMIN+5",	"Sixth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 12
	{ _SIGRTMIN+6,	"RTMIN+6",	"Seventh Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 14
	{ _SIGRTMIN+7,	"RTMIN+7",	"Eighth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 16
	{ _SIGRTMIN+8,	"RTMIN+8",	"Nint Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 18
	{ _SIGRTMIN+9,	"RTMIN+9",	"Tenth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 20
	{ _SIGRTMIN+10,	"RTMIN+10",	"Eleventh Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 22
	{ _SIGRTMIN+11,	"RTMIN+11",	"Twelfth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 24
	{ _SIGRTMIN+12,	"RTMIN+12",	"Thirteenth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 26
	{ _SIGRTMIN+13,	"RTMIN+13",	"Fourteenth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 28
	{ _SIGRTMIN+14,	"RTMIN+14",	"Fifteenth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 30
	{ _SIGRTMIN+15,	"RTMIN+15",	"Sixteenth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 32
	{ _SIGRTMIN+16,	"RTMIN+16",	"Seventeenth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 34
	{ _SIGRTMIN+17,	"RTMIN+17",	"Eighteenth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 36
	{ _SIGRTMIN+18,	"RTMIN+18",	"Nineteenth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 38
	{ _SIGRTMIN+19,	"RTMIN+19",	"Twentieth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 40
	{ _SIGRTMIN+20,	"RTMIN+20",	"Twenty-first Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 42
	{ _SIGRTMIN+21,	"RTMIN+21",	"Twenty-second Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 44
	{ _SIGRTMIN+22,	"RTMIN+22",	"Twenty-third Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 46
	{ _SIGRTMIN+23,	"RTMIN+23",	"Twenty-fourth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 48
	{ _SIGRTMIN+24,	"RTMIN+24",	"Twenty-fifth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 50
	{ _SIGRTMIN+25,	"RTMIN+25",	"Twenty-sixth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 52
	{ _SIGRTMIN+26,	"RTMIN+26",	"Twenty-seventh Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 54
	{ _SIGRTMIN+27,	"RTMIN+27",	"Twenty-eighth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 56
	{ _SIGRTMIN+28,	"RTMIN+28",	"Twenty-ninth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 58
	{ _SIGRTMIN+29,	"RTMIN+29",	"Thirtieth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 60
	{ _SIGRTMIN+30,	"RTMIN+30",	"Thirty-firsth Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 62
	{ _SIGRTMIN+31,	"RTMIN+31",	"Thirty-second Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 63
	{ _SIGRTMAX-31,	"RTMAX-31",	"Thirty-second Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 61
	{ _SIGRTMAX-30,	"RTMAX-30",	"Thirty-firsth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 59
	{ _SIGRTMAX-29,	"RTMAX-29",	"Thirtieth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 57
	{ _SIGRTMAX-28,	"RTMAX-28",	"Twenty-ninth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 55
	{ _SIGRTMAX-27,	"RTMAX-27",	"Twenty-eighth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 53
	{ _SIGRTMAX-26,	"RTMAX-26",
				    "Twenty-seventh Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 51
	{ _SIGRTMAX-25,	"RTMAX-25",	"Twenty-sixth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 49
	{ _SIGRTMAX-24,	"RTMAX-24",	"Twenty-fifth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 47
	{ _SIGRTMAX-23,	"RTMAX-23",	"Twenty-fourth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 45
	{ _SIGRTMAX-22,	"RTMAX-22",	"Twenty-third Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 43
	{ _SIGRTMAX-21,	"RTMAX-21",	"Twenty-second Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 41
	{ _SIGRTMAX-20,	"RTMAX-20",	"Twenty-first Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 39
	{ _SIGRTMAX-19,	"RTMAX-19",	"Twentieth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 37
	{ _SIGRTMAX-18,	"RTMAX-18",	"Nineteenth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 35
	{ _SIGRTMAX-17,	"RTMAX-17",	"Eighteenth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 33
	{ _SIGRTMAX-16,	"RTMAX-16",	"Seventeenth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 31
	{ _SIGRTMAX-15,	"RTMAX-15",	"Sixteenth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 29
	{ _SIGRTMAX-14,	"RTMAX-14",	"Fifteenth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 27
	{ _SIGRTMAX-13,	"RTMAX-13",	"Fourteenth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 25
	{ _SIGRTMAX-12,	"RTMAX-12",	"Thirteenth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 23
	{ _SIGRTMAX-11,	"RTMAX-11",	"Twelfth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 21
	{ _SIGRTMAX-10,	"RTMAX-10",	"Eleventh Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 19
	{ _SIGRTMAX-9,	"RTMAX-9",	"Tenth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 17
	{ _SIGRTMAX-8,	"RTMAX-8",	"Nint Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 15
	{ _SIGRTMAX-7,	"RTMAX-7",	"Eighth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 13
	{ _SIGRTMAX-6,	"RTMAX-6",	"Seventh Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 11
	{ _SIGRTMAX-5,	"RTMAX-5",	"Sixth Last Realtime Signal", },
#endif
#if	(_SIGRTMAX - _SIGRTMIN) >= 9
	{ _SIGRTMAX-4,	"RTMAX-4",	"Fifth Last Realtime Signal", },
#endif
#ifdef	_SIGRTMAX
	{ _SIGRTMAX-3,	"RTMAX-3",	"Fourth Last Realtime Signal", },
	{ _SIGRTMAX-2,	"RTMAX-2",	"Third Last Realtime Signal", },
	{ _SIGRTMAX-1,	"RTMAX-1",	"Second Last Realtime Signal", },
	{ _SIGRTMAX,	"RTMAX",	"Last Realtime Signal", },
#endif
	{ 0,	0,	0, },
};

LOCAL int	rtmin;
LOCAL int	rtmax;

/*
 * Make the numbers in rtsignames match the current numbers.
 * This is needed as SIGRTMIN and SIGRTMAX are macros that call
 * an interface that may not return the same numbers as
 * _SIGRTMIN and _SIGRTMAX.
 */
LOCAL void
_rtsiginit()
{
	register	int	i;
			int	nmed;
			int	max;

	rtmin = SIGRTMIN;
	rtmax = SIGRTMAX;
	nmed = (rtmin + rtmax) / 2;

	for (i = 0; rtsignames[i].signame; i++)
		;
	max = i-1;

	for (i = 0; rtsignames[i].signame; i++) {
		rtsignames[i].signo = 0;
		if ((rtmin + i) <= nmed) {
			rtsignames[i].signo = rtmin + i;
		} else if ((rtmax - max + i) > nmed) {
			rtsignames[i].signo = (rtmax - max) + i;
		}
	}
}
#endif	/* _SIGRTMIN */

#ifndef	HAVE_STRSIGNAL
/*
 * Convert the signal number into the signal description text.
 */
EXPORT char *
strsignal(sig)
	int	sig;
{
	register int	i;
		struct signames *sn = signames;

#ifdef	_SIGRTMIN
	if (rtmin == 0)
		_rtsiginit();

	if (sig >= rtmin && sig <= rtmax)
		sn = rtsignames;
#endif
	for (i = 0; sn[i].signame; i++) {
		if (sn[i].signo == sig)
			return (sn[i].sigtext);
	}
	return (NULL);
}

#endif

#ifndef	HAVE_STR2SIG
/*
 * Convert "HUP" or 1 into SIGHUP and similar for other signals.
 */
EXPORT int
str2sig(s, sigp)
	const char	*s;
	int		*sigp;
{
	register	int	i;
		struct signames *sn = signames;

#ifdef	_SIGRTMIN
	if (rtmin == 0)
		_rtsiginit();
#endif
	if (*s >= '0' && *s <= '9') {
		long	val;
#ifdef	HAVE_STRTOL
		char	*p;

		val = strtol(s, &p, 10);
		if (*p != '\0')
			return (-1);
#else
		if (*astolb(s, &val, 10) != '\0')
			return (-1);
#endif

#ifdef	_SIGRTMIN
		if (val >= rtmin && val <= rtmax)
			sn = rtsignames;
#endif
		for (i = 0; sn[i].signame; i++) {
			if (sn[i].signo == val) {
				*sigp = val;
				return (0);
			}
		}
		return (-1);
	}
	do {
		for (i = 0; sn[i].signame; i++) {
			if (strcmp(s, sn[i].signame) == 0) {
				*sigp = sn[i].signo;
				return (0);
			}
		}
#ifdef	_SIGRTMIN
	} while (sn == signames && (sn = rtsignames));
#else
	} while (0);
#endif

	return (-1);
}
#endif

#ifndef	HAVE_SIG2STR
/*
 * Convert signal numbers into the signal names (e.g. 1 -> "HUP").
 */
EXPORT int
sig2str(sig, s)
	int	sig;
	char	*s;
{
	register	int	i;
		struct signames *sn = signames;

#ifdef	_SIGRTMIN
	if (rtmin == 0)
		_rtsiginit();

	if (sig >= rtmin && sig <= rtmax)
		sn = rtsignames;
#endif
	for (i = 0; sn[i].signame; i++) {
		if (sn[i].signo == sig) {
			strcpy(s, sn[i].signame);
			return (0);
		}
	}
	return (-1);
}
#endif

#endif	/* ! (HAVE_STRSIGNAL && HAVE_STR2SIG && HAVE_SIG2STR) */
