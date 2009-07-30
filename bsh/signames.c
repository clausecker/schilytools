/* @(#)signames.c	1.6 09/07/11 Copyright 1998-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)signames.c	1.6 09/07/11 Copyright 1998-2009 J. Schilling";
#endif
/*
 *	Handle signal names for systems that don't have
 *	strsignal()/str2sig()/sig2str()
 *
 *	Copyright (c) 1998-2009 J. Schilling
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

#include <schily/stdio.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/signal.h>

#if	!(defined(HAVE_STRSIGNAL) && defined(HAVE_STR2SIG) && defined(HAVE_SIG2STR))

#ifndef	HAVE_STRSIGNAL
EXPORT	char	*strsignal	__PR((int sig));
#endif
#ifndef	HAVE_STR2SIG
EXPORT	int	str2sig		__PR((const char *s, int *sigp));
#endif
#ifndef	HAVE_SIG2STR
EXPORT	int	sig2str		__PR((int sig, char *s));
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

LOCAL struct signames {
	int	signo;
	char	*signame;
	char	*sigtext;
} signames[] = {
	{ 0,		"NULL",	"Null Signal", },
	{ 0,		"EXIT",	"Unknown Signal", },
#ifdef	SIGNULL
	{ SIGNULL,	"NULL",	"Null Signal", },
#endif
#ifdef	SIGHUP
	{ SIGHUP,	"HUP",	"Hangup", },
#endif
#ifdef	SIGINT
	{ SIGINT,	"INT",	"Interrupt", },
#endif
#ifdef	SIGQUIT
	{ SIGQUIT,	"QUIT",	"Quit", },
#endif
#ifdef	SIGILL
	{ SIGILL,	"ILL",	"Illegal Instruction", },
#endif
#ifdef	SIGTRAP
	{ SIGTRAP,	"TRAP",	"Trace/Breakpoint Trap", },
#endif
#ifdef	SIGABRT
	{ SIGABRT,	"ABRT",	"IOT trap/Abort", },
#endif
#ifdef	SIGIOT
	{ SIGIOT,	"IOT",	"IOT trap/Abort", },
#endif
#ifdef	SIGEMT
	{ SIGEMT,	"EMT",	"Emulation Trap", },
#endif
#ifdef	SIGFPE
	{ SIGFPE,	"FPE",	"Arithmetic Exception", },
#endif
#ifdef	SIGKILL
	{ SIGKILL,	"KILL",	"Killed", },
#endif
#ifdef	SIGBUS
	{ SIGBUS,	"BUS",	"Bus Error", },
#endif
#ifdef	SIGSEGV
	{ SIGSEGV,	"SEGV",	"Segmentation Fault", },
#endif
#ifdef	SIGSYS
	{ SIGSYS,	"SYS",	"Bad System Call", },
#endif
#ifdef	SIGPIPE
	{ SIGPIPE,	"PIPE",	"Broken Pipe", },
#endif
#ifdef	SIGALRM
	{ SIGALRM,	"ALRM",	"Alarm Clock", },
#endif
#ifdef	SIGTERM
	{ SIGTERM,	"TERM",	"Terminated", },
#endif
#ifdef	SIGUSR1
	{ SIGUSR1,	"USR1",	"User defined Signal 1", },
#endif
#ifdef	SIGUSR2
	{ SIGUSR2,	"USR2",	"User defined Signal 2", },
#endif
#ifdef	SIGSTKFLT
	{ SIGSTKFLT,	"STKFLT", "Stack Fault", },
#endif
#ifdef	SIGCLD
	{ SIGCLD,	"CLD",	"Child Status Changed", },
#endif
#ifdef	SIGCHLD
	{ SIGCHLD,	"CHLD",	"Child Status Changed", },
#endif
#ifdef	SIGPWR
	{ SIGPWR,	"PWR",	"Power-Fail/Restart", },
#endif
#ifdef	SIGWINCH
	{ SIGWINCH,	"WINCH",	"Window Size Change", },
#endif
#ifdef	SIGURG
	{ SIGURG,	"URG",	"Urgent Socket Condition", },
#endif
#ifdef	SIGPOLL
	{ SIGPOLL,	"POLL",	"Pollable Event", },
#endif
#ifdef	SIGIO
	{ SIGIO,	"IO",	"Possible I/O", },
#endif
#ifdef	SIGLOST
	{ SIGLOST,	"LOST",	"Resource Lost", },
#endif
#ifdef	SIGSTOP
	{ SIGSTOP,	"STOP",	"Stopped (signal)", },
#endif
#ifdef	SIGTSTP
	{ SIGTSTP,	"TSTP",	"Stopped (user)", },
#endif
#ifdef	SIGCONT
	{ SIGCONT,	"CONT",	"Continued", },
#endif
#ifdef	SIGTTIN
	{ SIGTTIN,	"TTIN",	"Stopped (tty input)", },
#endif
#ifdef	SIGTTOU
	{ SIGTTOU,	"TTOU",	"Stopped (tty output)", },
#endif
#ifdef	SIGVTALRM
	{ SIGVTALRM,	"VTALRM",	"Virtual Timer Expired", },
#endif
#ifdef	SIGPROF
	{ SIGPROF,	"PROF",	"Profiling Timer Expired", },
#endif
#ifdef	SIGXCPU
	{ SIGXCPU,	"XCPU",	"Cpu Time Limit Exceeded", },
#endif
#ifdef	SIGXFSZ
	{ SIGXFSZ,	"XFSZ",	"File Size Limit Exceeded", },
#endif
#ifdef	SIGWAITING
	{ SIGWAITING,	"WAITING",	"No runnable lwp", },
#endif
#ifdef	SIGLWP
	{ SIGLWP,	"LWP",	"Inter-lwp signal", },
#endif
#ifdef	SIGFREEZE
	{ SIGFREEZE,	"FREEZE",	"Checkpoint Freeze", },
#endif
#ifdef	SIGTHAW
	{ SIGTHAW,	"THAW",	"Checkpoint Thaw", },
#endif
#ifdef	_SIGRTMIN
	{ _SIGRTMIN,	"RTMIN",	"First Realtime Signal", },
	{ _SIGRTMIN+1,	"RTMIN+1",	"Second Realtime Signal", },
	{ _SIGRTMIN+2,	"RTMIN+2",	"Third Realtime Signal", },
	{ _SIGRTMIN+3,	"RTMIN+3",	"Fourth Realtime Signal", },
#endif
#ifdef	_SIGRTMAX
	{ _SIGRTMAX-3,	"RTMAX-3",	"Fourth Last Realtime Signal", },
	{ _SIGRTMAX-2,	"RTMAX-2",	"Third Last Realtime Signal", },
	{ _SIGRTMAX-1,	"RTMAX-1",	"Second Last Realtime Signal", },
	{ _SIGRTMAX,	"RTMAX",	"Last Realtime Signal", },
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
	{ 0,	0,	0, },
};

#ifndef	HAVE_STRSIGNAL
EXPORT char *
strsignal(sig)
	int	sig;
{
	register int	i;

	for (i = 0; signames[i].signame; i++) {
		if (signames[i].signo == sig)
			return (signames[i].sigtext);
	}
	return (NULL);
}

#endif

#ifndef	HAVE_STR2SIG
EXPORT int
str2sig(s, sigp)
	const char	*s;
	int		*sigp;
{
	register	int	i;

	for (i = 0; signames[i].signame; i++) {
		if (strcmp(s, signames[i].signame) == 0) {
			*sigp = signames[i].signo;
			return (0);
		}
	}
	return (-1);
}
#endif

#ifndef	HAVE_SIG2STR
EXPORT int
sig2str(sig, s)
	int	sig;
	char	*s;
{
	register	int	i;

	for (i = 0; signames[i].signame; i++) {
		if (signames[i].signo == sig) {
			strcpy(s, signames[i].signame);
			return (0);
		}
	}
	return (-1);
}
#endif

#endif	/* !(defined(HAVE_STRSIGNAL) && defined(HAVE_STR2SIG) && defined(HAVE_SIG2STR)) */
