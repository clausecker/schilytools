/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#if defined(sun)
#pragma ident	"@(#)fault.c	1.29	06/06/16 SMI"
#endif

#include "defs.h"

/*
 * Copyright 2008-2015 J. Schilling
 *
 * @(#)fault.c	1.30 15/09/07 2008-2015 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)fault.c	1.30 15/09/07 2008-2015 J. Schilling";
#endif

/*
 * UNIX shell
 */
#ifdef	SCHILY_INCLUDES
#ifdef	HAVE_SYS_PROCSET_H
#include	<sys/procset.h>
#endif
#ifdef	HAVE_SIGINFO_H
#include	<siginfo.h>
#endif
#ifndef	HAVE_SIGINFO_T
#define	siginfo_t char
#endif
#ifndef	HAVE_STACK_T
#define	stack_t char
#endif
#ifndef	SIGSTKSZ
#define	SIGSTKSZ	8192
#endif
#ifndef	SA_SIGINFO
#define	SA_SIGINFO	0
#endif
#ifndef	SA_ONSTACK
#define	SA_ONSTACK	0
#endif
#ifdef	HAVE_UCONTEXT_H
#include	<ucontext.h>
#else
#define	ucontext_t char
#endif
#include	<schily/errno.h>
#include	<schily/string.h>
#else
#include	<sys/procset.h>
#include	<siginfo.h>
#include	<ucontext.h>
#include	<errno.h>
#include	<string.h>
#endif

/*
 * intrptr is a pointer to an integer that is incremented whenever
 * a SIGINT is received and handled. This is used to inform additional
 * "builtin" commands about an interrupt, e.g. the builtins from the
 * Comand History Editor.
 */
	int	*intrptr;
	int	intrcnt;

/*
 * Whether to use _exit() because we did call vfork()
 */
	int	exflag;

/*
 * previous signal handler for signal 0
 */
static	void (*psig0_func) __PR((int)) = SIG_ERR;

#if defined(HAVE_STACK_T) && defined(HAVE_SIGALTSTACK)
static	char sigsegv_stack[SIGSTKSZ];
#endif

static int	ignoring	__PR((int i));
static void	clrsig		__PR((int i, int dofree));
	void	done		__PR((int sig));
static void	fault		__PR((int sig));
	int	handle		__PR((int sig, sigtype func));
	void	stdsigs		__PR((void));
	void	oldsigs		__PR((int dofree));
#ifdef	HAVE_VFORK
	void	restoresigs	__PR((void));
#endif
	void	chktrap		__PR((void));
	void	systrap		__PR((int argc, char **argv));
	void	sh_sleep	__PR((unsigned int ticks));
static void	sigsegv		__PR((int sig, siginfo_t *sip,
						ucontext_t *uap));
static void	set_sigval	__PR((int sig, void (*fun)(int)));
	void	init_sigval	__PR((void));

static BOOL sleeping = 0;
static unsigned char *trapcom[MAXTRAP]; /* array of actions, one per signal */
static BOOL trapflg[MAXTRAP] =
{
	0,
	0,	/* hangup */
	0,	/* interrupt */
	0,	/* quit */
#ifdef	__never__ /* For portability, we cannot asume > 3 signals */
	0,	/* illegal instr */
	0,	/* trace trap */
	0,	/* IOT */
	0,	/* EMT */
	0,	/* float pt. exp */
	0,	/* kill */
	0,	/* bus error */
	0,	/* memory faults */
	0,	/* bad sys call */
	0,	/* bad pipe call */
	0,	/* alarm */
	0,	/* software termination */
	0,	/* unassigned */
	0,	/* unassigned */
	0,	/* death of child */
	0,	/* power fail */
	0,	/* window size change */
	0,	/* urgent IO condition */
	0,	/* pollable event occured */
	0,	/* stopped by signal */
	0,	/* stopped by user */
	0,	/* continued */
	0,	/* stopped by tty input */
	0,	/* stopped by tty output */
	0,	/* virtual timer expired */
	0,	/* profiling timer expired */
	0,	/* exceeded cpu limit */
	0,	/* exceeded file size limit */
	0,	/* process's lwps are blocked */
	0,	/* special signal used by thread library */
	0,	/* check point freeze */
	0,	/* check point thaw */
#endif
};

/*
 * NOTE: Signal numbers other then 1, 2, 3, 6, 9, 14, and 15 are not portable.
 *	 We thus cannot use a pre-initialized table for signal numbers > 3.
 */
static void (*(
sigval[MAXTRAP])) __PR((int)) =
{
	0,
	done,	/* hangup */
	fault,	/* interrupt */
	fault,	/* quit */
#ifdef	__never__ /* For portability, we cannot asume > 3 signals */
	done,	/* illegal instr */
	done,	/* trace trap */
	done,	/* IOT / ABRT */
	done,	/* EMT */
	done,	/* floating pt. exp */
	0,	/* kill */
	done,	/* bus error */
	sigsegv,	/* memory faults */
	done,	/* bad sys call */
	done,	/* bad pipe call */
	done,	/* alarm */
	fault,	/* software termination */
	done,	/* unassigned */
	done,	/* unassigned */
	0,	/* death of child */
	done,	/* power fail */
	0,	/* window size change */
	done,	/* urgent IO condition */
	done,	/* pollable event occured */
	0,	/* uncatchable stop */
	0,	/* foreground stop */
	0,	/* stopped process continued */
	0,	/* background tty read */
	0,	/* background tty write */
	done,	/* virtual timer expired */
	done,	/* profiling timer expired */
	done,	/* exceeded cpu limit */
	done,	/* exceeded file size limit */
	0,	/* process's lwps are blocked */
	0,	/* special signal used by thread library */
	0,	/* check point freeze */
	0,	/* check point thaw */
#endif
};

static int
ignoring(i)
	int	i;
{
	struct sigaction act;
	if (trapflg[i] & SIGIGN)
		return (1);
	sigaction(i, 0, &act);
	if (act.sa_handler == SIG_IGN) {
		trapflg[i] |= SIGIGN;
		return (1);
	}
	return (0);
}

static void
clrsig(i, dofree)
	int	i;
	int	dofree;
{
	if (dofree && trapcom[i] != 0) {
		free(trapcom[i]);
		trapcom[i] = 0;
	}


	if (trapflg[i] & SIGMOD) {
		/*
		 * If the signal has been set to SIGIGN and we are now
		 * clearing the disposition of the signal (restoring it
		 * back to its default value) then we need to clear this
		 * bit as well
		 *
		 */
		if (trapflg[i] & SIGIGN)
			trapflg[i] &= ~SIGIGN;

		trapflg[i] &= ~SIGMOD;
		handle(i, sigval[i]);
	}
}

void
done(sig)
	int	sig;
{
	unsigned char	*t;
	int	savxit;
	struct excode savex;
	struct excode savrex;

	if ((t = trapcom[0]) != NULL)
	{
		trapcom[0] = 0;
		/* Save exit value so trap handler will not change its val */
		savxit = exitval;
		savex = ex;
		savrex = retex;
		retex.ex_signo = 0;
		execexp(t, (Intptr_t)0);
		exitval = savxit;		/* Restore exit value */
		ex = savex;
		retex = savrex;
		free(t);
	}
	else
		chktrap();

	rmtemp(0);
	rmfunctmp();

#ifdef ACCT
	doacct();
#endif
	if (flags & subsh) {
		/* in a subshell, need to wait on foreground job */
		collect_fg_job();
	}

	(void) endjobs(0);
	if (sig) {
		sigset_t set;

		/*
		 * If the signal is SIGHUP, then it should be delivered
		 * to the process group leader of the foreground job.
		 */
		if (sig == SIGHUP)
			hupforegnd();

		sigemptyset(&set);
		sigaddset(&set, sig);
		sigprocmask(SIG_UNBLOCK, &set, 0);
		handle(sig, SIG_DFL);
		kill(mypid, sig);
	}
	if (exflag)
		_exit(exitval);
	exit(exitval);
}

static void
fault(sig)
	int	sig;
{
	int flag = 0;

	switch (sig) {
#ifdef	SIGINT
		case SIGINT:
			if (intrptr)
				(*intrptr)++;
			intrcnt++;
			break;
#endif
#ifdef	SIGALRM
		case SIGALRM:
			if (sleeping)
				return;
			break;
#endif
	}

	if (trapcom[sig])
		flag = TRAPSET;
	else if (flags & subsh)
		done(sig);
	else
		flag = SIGSET;

	trapnote |= flag;
	trapflg[sig] |= flag;
	trapsig = sig;
}

int
handle(sig, func)
	int sig;
	sigtype func;
{
	int	ret;
	struct sigaction act, oact;

	if (func == SIG_IGN && (trapflg[sig] & SIGIGN))
		return (0);

	/*
	 * Ensure that sigaction is only called with valid signal numbers,
	 * we can get random values back for oact.sa_handler if the signal
	 * number is invalid
	 *
	 */
	if (sig > MINTRAP && sig < MAXTRAP) {
		sigemptyset(&act.sa_mask);
		act.sa_flags = (sig == SIGSEGV) ? (SA_ONSTACK | SA_SIGINFO) : 0;
		act.sa_handler = func;
		sigaction(sig, &act, &oact);
	}

	if (func == SIG_IGN)
		trapflg[sig] |= SIGIGN;

	/*
	 * Special case for signal zero, we can not obtain the previos
	 * action by calling sigaction, instead we save it in the variable
	 * psig0_func, so we can test it next time through this code
	 *
	 */
	if (sig == 0) {
		ret = (psig0_func != func);
		psig0_func = func;
	} else {
		ret = (func != oact.sa_handler);
	}

	return (ret);
}

void
stdsigs()
{
	int	i;
#if defined(HAVE_STACK_T) && defined(HAVE_SIGALTSTACK)
	stack_t	ss;
#endif
#ifdef	SIGRTMIN
	int rtmin = (int)SIGRTMIN;
	int rtmax = (int)SIGRTMAX;
#else
	int rtmin = -1;
	int rtmax = -2;
#endif

#if defined(HAVE_STACK_T) && defined(HAVE_SIGALTSTACK)
	ss.ss_size = SIGSTKSZ;
	ss.ss_sp = sigsegv_stack;
	ss.ss_flags = 0;
	if (sigaltstack(&ss, NULL) == -1) {
		error("sigaltstack(2) failed");
	}
#endif

	for (i = 1; i < MAXTRAP; i++) {
		if (i == rtmin) {
			i = rtmax;
			continue;
		}
		if (sigval[i] == 0)
			continue;
		if (i != SIGSEGV && ignoring(i))
			continue;
		handle(i, sigval[i]);
	}

	/*
	 * handle all the realtime signals
	 *
	 */
	for (i = rtmin; i <= rtmax; i++) {
		handle(i, done);
	}
}

/*
 * This function is called just before execve() is called.
 * It resets the signal handlers to their defaults if the
 * signals that are either not set (trapcom[i] == NULL) or
 * set to catch the signal ((trapcom[i][0] != '\0'). Note
 * that clrsig() only affects signals that have the SIGMOD
 * flag set in trapflg[i].
 */
void
oldsigs(dofree)
	int	dofree;
{
	int	i;
	int	f;
	unsigned char	*t;

	i = MAXTRAP;
	while (i--) {
		t = trapcom[i];
		f = trapflg[i];
		if (t == 0 || *t)
			clrsig(i, dofree);
		if (dofree)
			trapflg[i] = 0;
		else
			trapflg[i] = f;		/* Remember for restoresigs */
	}
	if (dofree)
		trapnote = 0;
}

#ifdef	HAVE_VFORK
void
restoresigs()
{
	int	i;
	int	f;
	unsigned char	*t;

	i = MAXTRAP;
	while (i--) {
		t = trapcom[i];
		f = trapflg[i];
		if ((f & SIGMOD) == 0)		/* If not modified before */
			continue;		/* skip this signal	  */
		if (t) {
			if (*t)
				handle(i, fault);
			else
				handle(i, SIG_IGN);
		} else {
			handle(i, sigval[i]);
		}
		trapflg[i] = f;
	}
}
#endif

/*
 * check for traps
 */

void
chktrap()
{
	int	i = MAXTRAP;
	unsigned char	*t;

	trapnote &= ~TRAPSET;
	while (--i) {
		if (trapflg[i] & TRAPSET) {
			trapflg[i] &= ~TRAPSET;
			if ((t = trapcom[i]) != NULL) {
				int	savxit = exitval;
				struct excode savex;
				struct excode savrex;

				savex = ex;
				savrex = retex;
				retex.ex_signo = i;
				execexp(t, (Intptr_t)0);
				exitval = savxit;
				ex = savex;
				retex = savrex;
				exitset();
			}
		}
	}
}

void
systrap(argc, argv)
	int	argc;
	char	**argv;
{
	int sig;

	if (argc == 1) {
		/*
		 * print out the current action associated with each signal
		 * handled by the shell
		 *
		 */
		for (sig = 0; sig < MAXTRAP; sig++) {
			if (trapcom[sig]) {
#ifdef	DO_POSIX_TRAP
				char	buf[12];

				prs_buff(UC "trap -- '");
				prs_buff(trapcom[sig]);
				prs_buff(UC "' ");
				if (sig2str(sig, buf) < 0)
					prn_buff(sig);
				else
					prs_buff(UC buf);
#else
				prn_buff(sig);
				prs_buff((unsigned char *)colon);
				prs_buff(trapcom[sig]);
#endif
				prc_buff(NL);
			}
		}
	} else {
		/*
		 * set the action for the list of signals
		 *
		 * a1 is guaranteed to be != NULL here
		 */
		char *cmdp = *argv, *a1 = *(argv+1);
		BOOL noa1 = FALSE;

#ifdef	DO_POSIX_TRAP
		if (a1[0] == '-') {
			if (a1[1] == '\0') {
				noa1++;
			} else if (a1[1] == '-' && a1[2] == '\0') {
				a1 = *(++argv + 1);
			} else {
				gfailure(UC usage, trapuse);
				return;
			}
			++argv;
		} else
#endif
		{
			noa1 = (str2sig(a1, &sig) == 0);
			if (noa1 == 0)
				++argv;
		}
		while (*++argv) {
			if (str2sig(*argv, &sig) < 0 ||
			    sig >= MAXTRAP || sig < MINTRAP ||
			    sig == SIGSEGV) {
				failure((unsigned char *)cmdp, badtrap);
			} else if (noa1) {
				/*
				 * no action specifed so reset the signal
				 * to its default disposition
				 *
				 */
				clrsig(sig, TRUE);
			} else if (*a1) {
				/*
				 * set the action associated with the signal
				 * to a1
				 *
				 */
				if (trapflg[sig] & SIGMOD || sig == 0 ||
				    !ignoring(sig)) {
					handle(sig, fault);
					trapflg[sig] |= SIGMOD;
					replace(&trapcom[sig],
						(unsigned char *)a1);
				}
			} else if (handle(sig, SIG_IGN)) {
				/*
				 * set the action associated with the signal
				 * to SIG_IGN
				 *
				 */
				trapflg[sig] |= SIGMOD;
				replace(&trapcom[sig], (unsigned char *)a1);
			}
		}
	}
}

void
sh_sleep(ticks)
	unsigned int	ticks;
{
#ifdef	SIGALRM
	sigset_t set, oset;
	struct sigaction act, oact;


	/*
	 * add SIGALRM to mask
	 */

	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_BLOCK, &set, &oset);

	/*
	 * catch SIGALRM
	 */

	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	act.sa_handler = fault;
	sigaction(SIGALRM, &act, &oact);

	/*
	 * start alarm and wait for signal
	 */

	alarm(ticks);
	sleeping = 1;
	sigsuspend(&oset);
	sleeping = 0;

	/*
	 * reset alarm, catcher and mask
	 */

	alarm(0);
	sigaction(SIGALRM, &oact, NULL);
	sigprocmask(SIG_SETMASK, &oset, 0);
#else
	sleep(ticks);
#endif
}

/* ARGSUSED */
static void
sigsegv(sig, sip, uap)
	int		sig;
	siginfo_t	*sip;
	ucontext_t	*uap;
{
	if (sip == (siginfo_t *)NULL) {
		/*
		 * This should never happen, but if it does this is all we
		 * can do. It can only happen if sigaction(2) for SIGSEGV
		 * has been called without SA_SIGINFO being set.
		 *
		 */

		exit(ERROR);
	} else {
#ifdef	HAVE_SIGINFO_T
		if (sip->si_code <= 0) {
			/*
			 * If we are here then SIGSEGV must have been sent to
			 * us from a user process NOT as a result of an
			 * internal error within the shell eg
			 * kill -SEGV $$
			 * will bring us here. So do the normal thing.
			 *
			 */
			fault(sig);
		} else {
			/*
			 * If we are here then there must have been an internal
			 * error within the shell to generate SIGSEGV eg
			 * the stack is full and we cannot call any more
			 * functions (Remeber this signal handler is running
			 * on an alternate stack). So we just exit cleanly
			 * with an error status (no core file).
			 */
			exit(ERROR);
		}
#else
		exit(ERROR);
#endif
	}
}

static void
set_sigval(sig, fun)
	int	sig;
	void	(*fun) __PR((int));
{
	if (sig < MAXTRAP)
		sigval[sig] = fun;
}

void
init_sigval()
{
#ifdef	SIGHUP
	set_sigval(SIGHUP, done);
#endif
#ifdef	SIGINT
	set_sigval(SIGINT, fault);
#endif
#ifdef	SIGQUIT
	set_sigval(SIGQUIT, fault);
#endif
#ifdef	SIGILL
	set_sigval(SIGILL, done);
#endif
#ifdef	SIGTRAP
	set_sigval(SIGTRAP, done);
#endif
#ifdef	SIGABRT
	set_sigval(SIGABRT, done);
#endif
#ifdef	SIGIOT
	set_sigval(SIGIOT, done);	/* SIGIOT == SIGABRT */
#endif
#ifdef	SIGEMT
	set_sigval(SIGEMT, done);
#endif
#ifdef	SIGFPE
	set_sigval(SIGFPE, done);
#endif
#ifdef	SIGKILL
#ifdef	__set_nullsig__
	set_sigval(SIGKILL, 0);
#endif
#endif
#ifdef	SIGBUS
	set_sigval(SIGBUS, done);
#endif
#ifndef	NO_SIGSEGV			/* No sigsegv() for debugging */
#ifdef	SIGSEGV
	set_sigval(SIGSEGV, (void (*) __PR((int)))sigsegv);
#endif
#endif
#ifdef	SIGSYS
	set_sigval(SIGSYS, done);
#endif
#ifdef	SIGPIPE
	set_sigval(SIGPIPE, done);
#endif
#ifdef	SIGALRM
	set_sigval(SIGALRM, done);
#endif
#ifdef	SIGTERM
	set_sigval(SIGTERM, fault);
#endif
#ifdef	SIGUSR1
	set_sigval(SIGUSR1, done);
#endif
#ifdef	SIGUSR2
	set_sigval(SIGUSR2, done);
#endif
#ifdef	SIGCHLD
	set_sigval(SIGCHLD, 0);
#endif
#ifdef	SIGPWR
	set_sigval(SIGPWR, done);
#endif
#ifdef	SIGWINCH
	set_sigval(SIGWINCH, 0);
#endif
#ifdef	SIGURG
	set_sigval(SIGURG, done);
#endif
#ifdef	SIGPOLL
	set_sigval(SIGPOLL, done);
#endif
#ifdef	SIGSTOP
	set_sigval(SIGSTOP, 0);
#endif
#ifdef	SIGTSTP
	set_sigval(SIGTSTP, 0);
#endif
#ifdef	SIGCONT
	set_sigval(SIGCONT, 0);
#endif
#ifdef	SIGTTIN
	set_sigval(SIGTTIN, 0);
#endif
#ifdef	SIGTTOU
	set_sigval(SIGTTOU, 0);
#endif
#ifdef	SIGVTALRM
	set_sigval(SIGVTALRM, done);
#endif
#ifdef	SIGPROF
	set_sigval(SIGPROF, done);
#endif
#ifdef	SIGXCPU
	set_sigval(SIGXCPU, done);
#endif
#ifdef	SIGXFSZ
	set_sigval(SIGXFSZ, done);
#endif
#ifdef	SIGWAITING
	set_sigval(SIGWAITING, 0);
#endif
#ifdef	SIGLWP
	set_sigval(SIGLWP, 0);
#endif
#ifdef	SIGFREEZE
	set_sigval(SIGFREEZE, 0);
#endif
#ifdef	SIGTHAW
	set_sigval(SIGTHAW, 0);
#endif

#ifdef	SIGSTKFLT
	set_sigval(SIGSTKFLT, done);
#endif
}
