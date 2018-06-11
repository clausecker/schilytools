/* @(#)waitid.c	1.2 18/06/10 Copyright 2015-2018 J. Schilling */
/*
 *	Emulate the waitid() syscall in case it is missing or disfunctional
 *
 *	Copyright (c) 2015-2018 J. Schilling
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

#include	<schily/wait.h>
#include	<schily/schily.h>

#ifndef	HAVE_WAITID
EXPORT int
waitid(idtype, id, infop, opts)
	idtype_t	idtype;
	id_t		id;
	siginfo_t	*infop;		/* Must be != NULL */
	int		opts;
{
	int		exstat;
	pid_t		pid;
#ifdef	__needed__
	struct tms	tms;
#endif

	opts &= ~(WEXITED|WTRAPPED);	/* waitpid() doesn't understand them */
#if	WSTOPPED != WUNTRACED
	if (opts & WSTOPPED) {
		opts &= ~WSTOPPED;
		opts |= WUNTRACED;
	}
#endif

	if (idtype == P_PID)
		pid = id;
	else if (idtype == P_PGID)
		pid = -id;
	else if (idtype == P_ALL)
		pid = -1;
	else
		pid = 0;

#ifdef	__needed__
	/*
	 * We cannot clear infop->si_utime and infop->si_stime as they are
	 * missing on HP-UX-10.x.
	 */
	infop->si_utime = 0;
	infop->si_stime = 0;
	if (WNOWAIT == 0 || (opts & WNOWAIT) == 0) {
		times(&tms);
	}
#endif
	pid = waitpid(pid, &exstat, opts);
	infop->si_pid = pid;
	infop->si_code = 0;
	infop->si_status = 0;

	if (pid == (pid_t)-1)
		return (-1);

#ifdef	__needed__
	if (WNOWAIT == 0 || (opts & WNOWAIT) == 0) {
		infop->si_utime = tms.tms_cutime;
		infop->si_stime = tms.tms_cstime;
		times(&tms);
		infop->si_utime = tms.tms_cutime - infop->si_utime;
		infop->si_stime = tms.tms_cstime - infop->si_stime;
	}
#endif

	if (WIFEXITED(exstat)) {
		infop->si_code = CLD_EXITED;
		infop->si_status = WEXITSTATUS(exstat);
	} else if (WIFSIGNALED(exstat)) {
		if (WCOREDUMP(exstat))
			infop->si_code = CLD_DUMPED;
		else
			infop->si_code = CLD_KILLED;
		infop->si_status = WTERMSIG(exstat);
	} else if (WIFSTOPPED(exstat)) {
#ifdef	SIGTRAP
		if (WSTOPSIG(exstat) == SIGTRAP)
			infop->si_code = CLD_TRAPPED;
		else
#endif
			infop->si_code = CLD_STOPPED;
		infop->si_status = WSTOPSIG(exstat);
	} else if (WIFCONTINUED(exstat)) {
		infop->si_code = CLD_CONTINUED;
		infop->si_status = 0;
	}

	return (0);
}
#endif
