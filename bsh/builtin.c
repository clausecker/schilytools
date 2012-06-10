/* @(#)builtin.c	1.80 12/06/03 Copyright 1988-2012 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)builtin.c	1.80 12/06/03 Copyright 1988-2012 J. Schilling";
#endif
/*
 *	Builtin commands
 *
 *	Copyright (c) 1985-2012 J. Schilling
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
#include <schily/signal.h>
#include <schily/utypes.h>
#include <schily/resource.h>
#include "bsh.h"
#include "str.h"
#include "abbrev.h"
#include "strsubs.h"
#include "btab.h"
#include "map.h"
#include "node.h"
#include <schily/setjmp.h>
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/fcntl.h>
#include <schily/utypes.h>
#include <schily/getargs.h>
#include <schily/time.h>
#include <schily/btorder.h>

extern int	firstsh;
extern int 	delim;
extern int 	ex_status;

extern	pid_t	opgrp;
extern	pid_t	mypgrp;
extern	pid_t	mypid;

LOCAL jmp_buf	waitjmp;

LOCAL	btab	*blook		__PR((char	*name, btab *bt, int n));
EXPORT	void	wrong_args	__PR((Argvec * vp, FILE ** std));
EXPORT	void	unimplemented	__PR((Argvec * vp, FILE ** std));
LOCAL	BOOL	not_loginsh	__PR((FILE ** std));
LOCAL	BOOL	helpwanted	__PR((Argvec * vp, FILE ** std));
EXPORT	BOOL	busage		__PR((Argvec * vp, FILE ** std));
EXPORT	BOOL	toint		__PR((FILE ** std, char *s, int *i));
EXPORT	BOOL	tolong		__PR((FILE ** std, char *s, long *l));
EXPORT	BOOL	tollong		__PR((FILE ** std, char *s, Llong *ll));
LOCAL	BOOL	isbadlpid	__PR((FILE ** std, long	lpid));
EXPORT	void	bnallo		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bdummy		__PR((Argvec * vp, FILE ** std, int flag));
LOCAL	int	do_oset		__PR((const char *arg, void *valp, int *pac, char *const **pav, const char *opt));
LOCAL	int	do_ounset	__PR((const char *arg, void *valp, int *pac, char *const **pav, const char *opt));
EXPORT	void	bsetcmd		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bunset		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bsetenv		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bunsetenv	__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bconcat		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bmap		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bunmap		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bexit		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	beval		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bdo		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	benv		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bwait		__PR((Argvec * vp, FILE ** std, int flag));
LOCAL	sigret	waitint		__PR((int sig));
LOCAL	int	gsig		__PR((char *s, int *sigp));
EXPORT	void	bkill		__PR((Argvec * vp, FILE ** std, int flag));
LOCAL	void	killp		__PR((FILE ** std, BOOL grpflg, pid_t p, int sig));
EXPORT	void	bsuspend	__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bresume		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bfg		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bpgrp		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bsync		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bumask		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bsetmask	__PR((Argvec * vp, FILE ** std, int flag));
LOCAL	char	*cvmode		__PR((int c, char *cp, char *ep, char *mode));
LOCAL	int	mval		__PR((mode_t m));
LOCAL	void	pmask		__PR((FILE ** std, BOOL do_posix));
EXPORT	void	blogout		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	becho		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bshift		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bremap		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bsavehist	__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bhistory	__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bsource		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	brepeat		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bexec		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	blogin		__PR((Argvec * vp, FILE ** std, int flag));
LOCAL	void	my_exec		__PR((char *name, int first_ac, Argvec *vp, FILE **std));
EXPORT	void	berrstr		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	btype		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	btrue		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bfalse		__PR((Argvec * vp, FILE ** std, int flag));
#ifdef	DO_FIND
LOCAL	int	quitfun		__PR((void *arg));
#endif
EXPORT	BOOL	builtin		__PR((Argvec * vp, FILE ** std, int flag));
LOCAL	int	suspend		__PR((pid_t p));
LOCAL	int	presume		__PR((pid_t p));
LOCAL	mode_t	setcmask	__PR((mode_t newmask));
LOCAL	mode_t	getcmask	__PR((void));

/*
 * Lookup the builtin command table for a specific function
 */
LOCAL btab *
blook(name, bt, n)
	register char	*name;
	register btab	*bt;
		int	n;
{
	register char	*s1;
	register char	*s2;
	register int	mid;
	register int	low;
	register int	high;

	low = 0;
	high = n - 1;
	while (low <= high) {
		mid = (low + high) / 2;
		s1 = name;
		s2 = bt[mid].b_name;
		for (; *s1 == *s2; s1++, s2++) /* viel schneller als strcmp */
			if (*s1 == '\0')
				return (&bt[mid]);
		if (*s1 < *s2) {
			high = mid - 1;
			continue;
		}
		low = mid + 1;
	}
	return ((btab *) NULL);
}

/*
 * Generic function used when a builtin is called with a wrong number of args
 */
EXPORT void
wrong_args(vp, std)
	Argvec	*vp;
	FILE	*std[];
{
	fprintf(std[2], "%s: Wrong number of args.\n", vp->av_av[0]);
	busage(vp, std);
	ex_status = 1;
}

/*
 * Generic funtcion used to replace unimplemented funtions
 */
EXPORT void
unimplemented(vp, std)
	Argvec	*vp;
	FILE	*std[];
{
	fprintf(std[2], "%s: unimplemented.\n", vp->av_av[0]);
	ex_status = 1;
}

/*
 * Error function used when a function which is resereved for a login shell
 * is used from a non-login shell
 */
LOCAL BOOL
not_loginsh(std)
	FILE	*std[];
{
	if (!firstsh) {
		fprintf(std[2], "Not login shell.\n");
		ex_status = 1;
		return (TRUE);
	}
	return (FALSE);
}

/*
 * The generic function to check whether a builtin has been called
 * with the -help option
 */
LOCAL BOOL
helpwanted(vp, std)
	Argvec	*vp;
	FILE	*std[];
{
	if (vp->av_ac > 1 &&
	    (streql(vp->av_av[1], helpname) ||
	    (vp->av_av[1][0] == '-' && vp->av_av[1][1] == '-' &&
	    streql(&vp->av_av[1][1], helpname)))) {
		return (busage(vp, std));
	}
	return (FALSE);
}

/*
 * The generic function to print the usage for a builtin function.
 */
EXPORT BOOL
busage(vp, std)
	Argvec	*vp;
	FILE	*std[];
{
	register btab	*bp;
	register char	*name = vp->av_av[0];

	if ((bp = blook(name, bitab, n_builtin)) != NULL && bp->b_help) {
		if (bp->b_help == (char *)-1)
			return (FALSE);
		fprintf(std[2], "%s%s [options] %s\n", usage, name, bp->b_help);
		return (TRUE);
	}
	return (FALSE);
}

EXPORT BOOL
toint(std, s, i)
	FILE	*std[];
	char	*s;
	int	*i;
{
	if (*s == '\0' || *astoi(s, i)) {
		fprintf(std[2], "Not a number: %s.\n", s);
		ex_status = 1;
		return (FALSE);
	}
	return (TRUE);
}

EXPORT BOOL
tolong(std, s, l)
	FILE	*std[];
	char	*s;
	long	*l;
{
	if (*s == '\0' || *astol(s, l)) {
		fprintf(std[2], "Not a number: %s.\n", s);
		ex_status = 1;
		return (FALSE);
	}
	return (TRUE);
}

EXPORT BOOL
tollong(std, s, ll)
	FILE	*std[];
	char	*s;
	Llong	*ll;
{
	if (*s == '\0' || *astoll(s, ll)) {
		fprintf(std[2], "Not a number: %s.\n", s);
		ex_status = 1;
		return (FALSE);
	}
	return (TRUE);
}

LOCAL BOOL
isbadlpid(std, lpid)
	FILE	*std[];
	long	lpid;
{
	pid_t	p = lpid;

	if (p != lpid) {
		fprintf(std[2], "Bad process id: %ld.\n", lpid);
		ex_status = 1;
		return (TRUE);
	}
	return (FALSE);
}

/* ARGSUSED */
EXPORT void
bnallo(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	fprintf(std[2], "'%s' not allowed here.\n", vp->av_av[0]);
}

/* ARGSUSED */
EXPORT void
bdummy(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	/* this procedure does nothing */
}

struct seta {
	int	pfshell;
	int	olist;
};

/* ARGSUSED */
LOCAL int
do_oset(arg, valp, pac, pav, opt)
	const char	*arg;
	void		*valp;
	int		*pac;
	char	*const	**pav;
	const char	 *opt;
{
	if (arg[0] == '-' || arg[0] == '+') {
		/*
		 * Strange Korn Shell rules:
		 * If next arg is an option, -o was called without parameter.
		 */
		if (arg == &opt[2])		/* arg concatenated with -o */
			return (BADFLAG);
		(*pav)--;
		(*pac)++;
		*(int *)valp = TRUE;
		((struct seta *)valp)->olist = TRUE;
		return (1);
	}
	if (streql(arg, "profile")) {
		((struct seta *)valp)->pfshell = TRUE;
		return (1);
	}
	((struct seta *)valp)->olist = -1;
	return (BADFLAG);
}

/* ARGSUSED */
LOCAL int
do_ounset(arg, valp, pac, pav, opt)
	const char	*arg;
	void		*valp;
	int		*pac;
	char	*const	**pav;
	const char	 *opt;
{
	if (arg[0] == '-' || arg[0] == '+') {
		/*
		 * Strange Korn Shell rules:
		 * If next arg is an option, +o was called without parameter.
		 */
		if (arg == &opt[2])		/* arg concatenated with +o */
			return (BADFLAG);
		(*pav)--;
		(*pac)++;
		((struct seta *)valp)->olist = TRUE;
		return (1);
	}
	if (streql(arg, "profile")) {
		((struct seta *)valp)->pfshell = FALSE;
		return (1);
	}
	((struct seta *)valp)->olist = -1;
	return (BADFLAG);
}

/* ARGSUSED */
EXPORT void
bsetcmd(vp, std, flag)
	register	Argvec	*vp;
			FILE	*std[];
			int	flag;
{
	int	ac;
	char	* const *av;
	BOOL	help	= FALSE;
	struct seta	seta;
	int	ret;

	if (vp->av_ac <= 1) {
		ev_list(std[1]);
		return;
	}
	seta.pfshell	= pfshell;
	seta.olist	= FALSE;

	ac = vp->av_ac - 1;	/* set values */
	av = &vp->av_av[1];
	ret = getargs(&ac, &av, "help,P%1,+P%0,o&,+o&",
			&help,
			&seta.pfshell, &seta.pfshell,
			do_oset, &seta, do_ounset, &seta);

	/*
	 * The Korn Shell implements a really strange syntax. 
	 * set -o uses an optional argument which is hard to parse with a
	 * standardized optio parser.
	 */
	if (ret == BADFLAG && ac == 1 && seta.olist >= 0 &&
	    ((av[0][0] == '-' || av[0][0] == '+') && av[0][1] == 'o' && av[0][2] == '\0')) {
		seta.olist = TRUE;
	} else if (ret < 0 &&
		    (av[0][0] == '-' || av[0][0] == '+' || seta.olist < 0)) {
		fprintf(std[2], "Bad option '%s'\n", av[0]);
		busage(vp, std);
		ex_status = 1;
		return;
	}

	if (seta.pfshell != pfshell) {
		pfshell = seta.pfshell;
		if (pfshell)
			pfinit();
		else
			pfend();
	}

	if (ret != FLAGDELIM && ac == 1)
		ev_insert(makestr(av[0]));
	else if (ac > 1)
		wrong_args(vp, std);

	if (seta.olist) {
		fprintf(std[1], "Current option settings\n");
		fprintf(std[1], "profile		%s\n", pfshell ? "on" : "off");
	}
}

/* ARGSUSED */
EXPORT void
bunset(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	ev_delete(vp->av_av[1]);
}

/* ARGSUSED */
EXPORT void
bsetenv(vp, std, flag)
	register Argvec	*vp;
		FILE	*std[];
		int	flag;
{
	if (vp->av_ac == 1)
		ev_list(std[1]);
	else if (vp->av_ac != 3)
		wrong_args(vp, std);
	else
		ev_insert(concat(vp->av_av[1], eql, vp->av_av[2], (char *)NULL));
}

/* ARGSUSED */
EXPORT void
bunsetenv(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	ev_delete(vp->av_av[1]);
}

/* ARGSUSED */
EXPORT void
bconcat(vp, std, flag)
	register	Argvec	*vp;
			FILE	*std[];
			int	flag;
{
	register	char	*args;

	if (vp->av_ac < 3) {
		wrong_args(vp, std);
	} else {
		args = concatv(&vp->av_av[2]);
		ev_insert(concat(vp->av_av[1], eql, args, (char *)NULL));
		free(args);
	}
}

/* ARGSUSED */
EXPORT void
bmap(vp, std, flag)
	register	Argvec	*vp;
			FILE	*std[];
			int	flag;
{
#ifdef	INTERACTIVE
	if (vp->av_ac == 1)
		list_map(std[1]);
	else if (vp->av_ac == 2 && streql(vp->av_av[1], "-r"))
		remap();
	else if (vp->av_ac == 3 && streql(vp->av_av[1], "-u"))
		del_map(vp->av_av[2]);
	else if (vp->av_ac != 3 && vp->av_ac != 4)
		wrong_args(vp, std);
	else if (!add_map(vp->av_av[1], vp->av_av[2], vp->av_av[3])) {
		ex_status = 1;
		fprintf(std[2], "'%s' already defined.\n", vp->av_av[1]);
	}
#else
	unimplemented(vp, std);
#endif
}

/* ARGSUSED */
EXPORT void
bunmap(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
#ifdef	INTERACTIVE
	del_map(vp->av_av[1]);
#else
	unimplemented(vp, std);
#endif
}

/* ARGSUSED */
EXPORT void
bexit(vp, std, flag)
	register	Argvec	*vp;
			FILE	*std[];
			int	flag;
{
	if (vp->av_ac > 2) {
		wrong_args(vp, std);
		return;
	}
	if (vp->av_ac == 2 && !toint(std, vp->av_av[1], &ex_status))
		return;
	exitbsh(ex_status);
}

/* ARGSUSED */
EXPORT void
beval(vp, std, flag)
	register	Argvec	*vp;
			FILE	*std[];
			int	flag;
{
	register int i;

	delim = ' ';
	pushline(nl);
	for (i = vp->av_ac; --i > 0; )
		pushline(vp->av_av[i]);
/*	freetree(cmdline(0|PGRP, std, FALSE));*/
	freetree(cmdline(flag, std, FALSE));
}

/*
 * Execute a command with new args
 */
/* ARGSUSED */
EXPORT void
bdo(vp, std, flag)
	register	Argvec	*vp;
			FILE	*std[];
			int	flag;
{
	int	sac = vac;			/* save old args */
	char	**sav = vav;
	BOOL	isdosh;

	isdosh = streql(vp->av_av[0], "dosh");

	if (isdosh) {
		/*
		 * dosh command_string [command_name [[args]]
		 */
		if (vp->av_ac <= 1)
			return;
		vac = vp->av_ac - 2;		/* set new args */
		vav = &vp->av_av[2];
	} else {
		/*
		 * do command_string [args]
		 */
		vac = vp->av_ac - 1;		/* set new args */
		vav = &vp->av_av[1];
	}
#ifdef DEBUGX
	printf("        bdo: executing '%s'\n", vp->av_av[1]); flush();
#endif
	pushline(vp->av_av[1]);
/*	freetree(cmdline(0|PGRP, std, FALSE));*/
	freetree(cmdline(flag, std, FALSE));
	vac = sac;
	vav = sav;
}

/*
 * Execute a command with new environment.
 * Also called for "name=value ...", in this case the ENV flag is set.
 */
/* ARGSUSED */
EXPORT void
benv(vp, std, flag)
	register	Argvec	*vp;
			FILE	*std[];
			int	flag;
{
extern	unsigned evasize;
extern	int	evaent;
	Argvec	*nvp;
	int	len;
	int	i;
	int	ac;
	char	* const *av;
	char	*p;
	char	*opt = "i";
	BOOL	iflag = FALSE;
	BOOL	keepenv = FALSE;
	pid_t	child;
volatile int	cstat;

	ac = len = vp->av_ac - 1;	/* set values */
	av = &vp->av_av[1];
	if (ac < 1) {
		ev_list(std[1]);
		return;
	}
	if ((flag & ENV) && strchr(av[ac-1], '=') != NULL) {
		for (i = 0; i < ac; i++) {
			if (strchr(av[i], '=') == NULL)
				break;
		}
		if (i == ac)
			keepenv = TRUE;	/* This is only a list of a=b args */

	} else if ((flag & ENV) == 0) {
		if (av[0][0] == '-' && av[0][1] == '\0') { /* non std '-' opt */
			iflag = TRUE;
			ac--;
			av++;
		}
		if (getargs(&ac, &av, opt, &iflag) < 0 && av[0][0] == '-') {
			busage(vp, std);
			ex_status = 1;
			return;
		}
	}

	/*
	 * Add new environment entries.
	 * This is only a list of a=b args - no real "env" command.
	 */
	if (keepenv) {
		for (; ac > 0 && (p = strchr(av[0], '=')) != NULL; ac--, av++) {
			if (ev_set_locked(av[0]))
				continue;
			ev_insert(makestr(av[0]));
		}
		return;
	}

	setstime();
	child = shfork(flag);
	if (child == 0) {

		if (iflag) {
			evarray = (char **)NULL;
			evasize = 0;
			evaent = 0;
			ev_inc();
		}
		/*
		 * Add new environment entries.
		 */
		for (; ac > 0 && (p = strchr(av[0], '=')) != NULL; ac--, av++) {
			if (ev_set_locked(av[0]))
				continue;
			ev_insert(makestr(av[0]));
		}
		if (ac < 1) {			/* If no args list env */
			ev_list(std[1]);
			_exit(0);
		}


		nvp = allocvec(ac);
		for (i = 0; i < ac; i++) {
			nvp->av_av[i] = vp->av_av[i + (len-ac) + 1];
		}
		cstat = execcmd(nvp, std, flag | (iflag ? IGNENV : 0));
		if (ex_status)
			_exit(ex_status);
		_exit(cstat);
		/*
		 * This is a (hidden) background command, so we wo not
		 * need to call: free(nvp);
		 */
	} else if (child != -1) {
		if (!(flag & ASYNC))
			ewait(child, WALL|NOTMS);
	}
}

/*
 * Wait for commands that have been previously executed in background.
 */
/* ARGSUSED */
EXPORT void
bwait(vp, std, flag)
	register	Argvec	*vp;
			FILE	*std[];
			int	flag;
{
	pid_t	p;
	long	lp;
	char	**t;
	volatile sigtype intsav = SIG_DFL;

	if (!setjmp(waitjmp)) {
		if (osig2 != (sigtype) SIG_IGN)
			intsav = signal(SIGINT, waitint);
		if (vp->av_ac == 1) {
			ex_status = ewait((pid_t)0, 0);
		} else for (t = &vp->av_av[1]; *t != NULL; t++) {
			if (!tolong(std, *t, &lp))
				return;
			p = lp;
			if (isbadlpid(std, lp))
				return;
			ex_status = ewait(p, WALL);
		}
	}
	if (osig2 != (sigtype) SIG_IGN)
		signal(SIGINT, intsav);
}

/* ARGSUSED */
LOCAL sigret
waitint(sig)
	int	sig;
{
	extern int sigcount[];

	signal(SIGINT, waitint);
	sigcount[SIGINT]++;
	longjmp(waitjmp, TRUE);
}

LOCAL int
gsig(s, sigp)
	char	*s;
	int	*sigp;
{
	int	sig;

	if (*astoi(s, &sig)) {
		if (str2sig(s, &sig) != 0) {
			if (streql(s, "IOT")) {
				sig = SIGABRT;
			} else {
				return (-1);
			}
		}
	}
	*sigp = sig;
	if (sig > NSIG || sig < 0)
		return (-1);
	return (1);
}

/* ARGSUSED */
EXPORT void
bkill(vp, std, flag)
	register	Argvec	*vp;
		FILE	*std[];
		int	flag;
{
	int	ac;
	char	* const *av;
	char	*opt = "l,&";
	BOOL	list = FALSE;
	BOOL	kpgrp;
	pid_t	p;
	long	lp;
	int	sig = SIGTERM;
	int	i;
	register char * const *t;

	ac = vp->av_ac - 1;		/* set values */
	av = &vp->av_av[1];
	if (getargs(&ac, &av, opt, &list, gsig, &sig, &kpgrp) < 0) {
		if (sig > NSIG || sig < 0)
			fprintf(std[2], "%s: Bad signo: %d.", vp->av_av[0], sig);
		else
			fprintf(std[2], ebadopt, vp->av_av[0], av[0]);
		fprintf(std[2], " kill -l lists signals\n");
		busage(vp, std);
		ex_status = 1;
		return;
	}
	if (list) {
		for (i = 1; i <= NSIG-1; i++) {
			char	sname[32];

			if (sig2str(i, sname) == 0)
				fprintf(std[1], "%s ", sname);
			if (i % 8 == 0)
				fprintf(std[1], "%s", nl);
		}
		fprintf(std[1], "%s", nl);
		return;
	}
	if (ac < 1) {
		wrong_args(vp, std);
		return;
	}
	kpgrp = streql(vp->av_av[0], "killpg");
	for (t = av; *t != NULL; t++) {
		if (!tolong(std, *t, &lp))
			return;
		p = lp;
		if (isbadlpid(std, lp))
			return;
		killp(std, kpgrp, p, sig);
	}
}

#	ifndef	HAVE_KILLPG
#		define	killpg(p, s)	kill(-(p), s)
#	endif

#ifdef	PROTOTYPES
LOCAL void
killp(FILE ** std, BOOL grpflg, pid_t p, int sig)
#else
LOCAL void
killp(std, grpflg, p, sig)
	FILE	*std[];
	BOOL	grpflg;
	pid_t	p;
	int	sig;
#endif
{
	if ((grpflg ? killpg(p, sig) : kill(p, sig)) < 0) {
		char	sname[32];

		ex_status = geterrno();
		fprintf(std[2], "Can't send ");
		if (sig2str(sig, sname) == 0)
			fprintf(std[2], "SIG%s ", sname);
		else
			fprintf(std[2], "signal %d ", sig);
		fprintf(std[2], "to %s %lld. %s\n",
			grpflg ? "processgroup" : "process", (Llong)p,
			errstr(ex_status));
	}
#ifdef	SIGCONT
	if (sig == SIGHUP || sig == SIGTERM)
		grpflg ? killpg(p, SIGCONT) : kill(p, SIGCONT);
#endif	/* SIGCONT */
}

/* ARGSUSED */
EXPORT void
bsuspend(vp, std, flag)
	register	Argvec	*vp;
			FILE	*std[];
			int	flag;
{
	pid_t	p = (pid_t)-1;
	long	lp;
	register char **t;
	sigtype intsav = (sigtype) SIG_DFL;
	int	stop;

#if	defined(SIGTSTP) || defined(JOS)

	stop = streql("stop", vp->av_av[0]);
	if (vp->av_ac == 1) {
		if (stop) {
			wrong_args(vp, std);
			return;
		}
		p = mypid;
		if (firstsh) {
			fprintf(std[2], "Can't suspend login shell.\n");
			ex_status = 1;
			return;
		}
#ifdef	SIGTSTP
		intsav = signal(SIGTSTP, (sigtype) SIG_DFL);
#endif
		if (suspend(p) < 0)
			ex_status = geterrno();
	} else for (t = &vp->av_av[1]; *t != NULL; t++) {
		if (!tolong(std, *t, &lp))
			return;
		p = lp;
		if (isbadlpid(std, lp))
			return;
		/* stop == killpg ???? */
		if (kill(p, stop ? SIGSTOP : SIGTSTP) < 0) {
			ex_status = geterrno();
			break;
		}
	}
#ifdef	SIGTSTP
	if (intsav != (sigtype) SIG_DFL)
		signal(SIGTSTP, intsav);
#endif
	if (ex_status != 0)
		fprintf(std[2], "Can't %s %lld. %s\n",
				vp->av_av[0], (Llong)p, errstr(ex_status));
#else	/* defined(SIGTSTP) || defined(JOS) */
	unimplemented(vp, std);
#endif	/* defined(SIGTSTP) || defined(JOS) */
}


/* ARGSUSED */
EXPORT void
bresume(vp, std, flag)
	register	Argvec	*vp;
			FILE	*std[];
			int	flag;
{
	pid_t	p;
	long	lp;
#ifdef	JOBCONTROL
	pid_t	pgrp;
#endif	/* JOBCONTROL */

#if	defined(SIGCONT) || defined(JOS)

	if (!tolong(std, vp->av_av[1], &lp))
		return;
	p = lp;
	if (isbadlpid(std, lp))
		return;
#ifdef	JOBCONTROL
	pgrp = getpgid(p);
	tty_setpgrp(0, pgrp);
#endif	/* JOBCONTROL */
	if (presume(p) < 0) {
		ex_status = geterrno();
		fprintf(std[2], "Can't resume %ld. %s\n", (long)p, errstr(ex_status));
	} else if (!(flag & ASYNC)) {
		ewait(p, WALL);
	}

#else	/* defined(SIGCONT) || defined(JOS) */

	unimplemented(vp, std);

#endif	/* defined(SIGCONT) || defined(JOS) */
}

/* ARGSUSED */
EXPORT void
bfg(vp, std, flag)
	register	Argvec	*vp;
			FILE	*std[];
			int	flag;
{
	int	fg;
	pid_t	p;
	long	lp;
	pid_t	pgrp;
#ifdef	JOBCONTROL
	extern pid_t	lastsusp;	/* XXX Temporary !!! */
#endif

#if	defined(SIGCONT)

	fg = streql("fg", vp->av_av[0]);
	if (vp->av_ac > 2) {
		wrong_args(vp, std);
		return;
	} else if (vp->av_ac == 1) {
#ifdef	JOBCONTROL
		if ((lp = lastsusp) == 0)
			return;
#endif
		if (streql("$", vp->av_av[0]))
			fg = TRUE;
	} else if (!tolong(std, vp->av_av[1], &lp))
		return;

	p = lp;
	if (isbadlpid(std, lp))
		return;
#if	!defined(HAVE_GETPGID) && !defined(HAVE_BSD_GETPGRP)
	if ((pgrp = -1) < 0) {
		seterrno(0);	/* XXX ??? */
		/*
		 * XXX We should have a local process table
		 * XXX to map pid to pgrp.
		 */
#else
	if ((pgrp = getpgid(p)) < 0) {
#endif
		ex_status = geterrno();
		fprintf(std[2], "Can't get processgroup of %ld. %s\n",
						(long)p, errstr(ex_status));
		return;
	}
#ifdef	JOBCONTROL
	if (fg)
		tty_setpgrp(0, pgrp);
#endif	/* JOBCONTROL */
	if (killpg(pgrp, SIGCONT) < 0) {
		ex_status = geterrno();
		fprintf(std[2], "Can't resume %ld. %s\n",
						(long)p, errstr(ex_status));
	}
	/*else*/ if (fg && !(flag & ASYNC))
		ewait(p, WALL);
#else	/* defined(SIGCONT) */
	unimplemented(vp, std);
#endif	/* defined(SIGCONT) */
}

/* ARGSUSED */
EXPORT void
bpgrp(vp, std, flag)
	register	Argvec	*vp;
			FILE	*std[];
			int	flag;
{
	pid_t	p;
	long	lp;
	pid_t	pgrp = 0;

	if (vp->av_ac > 2) {
		wrong_args(vp, std);
		return;
	} else if (vp->av_ac == 1) {
		lp = mypid;

		pgrp = tty_getpgrp(fdown(std[0]));
		fprintf(std[1], "ttyprocessgroup: %ld\n", (long)pgrp);

	} else if (!tolong(std, vp->av_av[1], &lp))
		return;
	p = lp;
	if (isbadlpid(std, lp))
		return;
#if	!defined(HAVE_GETPGID) && !defined(HAVE_BSD_GETPGRP)
	if (p != mypid) {
		unimplemented(vp, std);
		return;
	}
#endif
	seterrno(0);
	if ((pgrp = getpgid(p)) < 0 && geterrno() != 0) {
		ex_status = geterrno();
		fprintf(std[2], "Can't get processgroup of %ld. %s\n",
						(long)p, errstr(ex_status));
		return;
	}
	fprintf(std[1], "pid: %ld processgroup: %ld\n", (long)p, (long)pgrp);
}

/* ARGSUSED */
EXPORT void
bsync(vp, std, flag)
	Argvec *vp;
	FILE	*std[];
	int	flag;
{
#ifdef	HAVE_SYNC
	sync();
#else
	unimplemented(vp, std);
#endif
}

/* ARGSUSED */
EXPORT void
bumask(vp, std, flag)
	register	Argvec *vp;
	register	FILE	*std[];
			int	flag;
{
		int	ac;
		char	* const *av;
		BOOL	symbolic = FALSE;
		mode_t	newmask = 0;
	register char	*cp;

	ac = vp->av_ac - 1;		/* set values */
	av = &vp->av_av[1];
	if (getargs(&ac, &av, "S", &symbolic) < 0) {
		/*
		 * Let the last argument fail in getperm()
		 * if it is not OK. POSIX requires "umask -r" to fail,
		 * this test catches e.g. "umask a=rwx".
		 */
		if (ac == 1 && vp->av_ac >= 2 && av[0][0] != '-')
			goto ok;
		fprintf(std[2], ebadopt, vp->av_av[0], av[0]);
		fprintf(std[2], "%s", nl);
		busage(vp, std);
		ex_status = 1;
		return;
	}
	if (ac == 0) {
		if (symbolic)
			pmask(std, TRUE);
		else
			fprintf(std[1], "0%llo\n", (ULlong)getcmask());
		return;
	}
	if (ac != 1) {
		wrong_args(vp, std);
		return;
	}
ok:
	if (getperm(std[2], av[0], NULL, &newmask, ~getcmask(),
							GP_UMASK|GP_NOX)) {
		fprintf(std[2], "Improper mask\n");
		ex_status = 1;
		return;
	}
	cp = av[0];
	if (*cp >= '0' && *cp <= '7')
		(void) setcmask(newmask);
	else
		(void) setcmask(~newmask);
}

/* ARGSUSED */
EXPORT void
bsetmask(vp, std, flag)
	register	Argvec *vp;
	register	FILE	*std[];
			int	flag;
{
			char	mstr[64];
			char	*mp;
			mode_t	 oldmask;
			mode_t	 newmask;
	register	char	 **av = vp->av_av;

	if (vp->av_ac == 1) {
		pmask(std, FALSE);
		return;
	}
	if (vp->av_ac != 4) {
		wrong_args(vp, std);
		return;
	}
	oldmask = ~getcmask();
	mp = mstr;
	mp = cvmode('u', mp, &mstr[sizeof (mstr) - 1], av[1]);
	mp = cvmode('g', mp, &mstr[sizeof (mstr) - 1], av[2]);
	mp = cvmode('o', mp, &mstr[sizeof (mstr) - 1], av[3]);
	if (mp > mstr && mp[-1] == ',')
		mp[-1] = '\0';
	if (mstr[0] == '\0')	/* Keep old mask */
		return;
	if (getperm(std[2], mstr, NULL, &newmask, oldmask, GP_NOX)) {
		fprintf(std[2], "Improper mask\n");
		ex_status = 1;
		return;
	}
	(void) setcmask(~newmask);
}

LOCAL char *
cvmode(c, cp, ep, mode)
	int	c;
	char	*cp;
	char	*ep;
	char	*mode;
{
	char	*p = cp;
	char	*mp;

	for (mp = mode; *mp; mp++) {
		if (*mp == '=') {	/* Keep old mask */
			p = cp;
			mp++;
			if (*mp == '\0')
				break;
		}
		if (p == cp) {		/* Add perm type */
			*p++ = c;
		}
		if (*mp == '.') {	/* Clear mask */
			if ((p+5) >= ep)
				break;
			strcpy(p, "-rwx+");
			p += 5;
			continue;
		} else if (p == &cp[1] && *mp != '-' && *mp != '+') {
			*p++ = '=';
		}
		if (p >= ep)
			break;
		*p++ = *mp;
	}
	if (p > cp)
		*p++ = ',';
	*p = '\0';
	return (p);
}


static char *modtab[] =
{"...", "..x", ".w.", ".wx", "r..", "r.x", "rw.", "rwx"};
static char *umodtab[] =
{"", "x", "w", "wx", "r", "rx", "rw", "rwx"};

LOCAL int
mval(m)
	mode_t	m;
{
	int	ret = 0;

	if (m & (S_IRUSR|S_IRGRP|S_IROTH))
		ret |= 4;
	if (m & (S_IWUSR|S_IWGRP|S_IWOTH))
		ret |= 2;
	if (m & (S_IXUSR|S_IXGRP|S_IXOTH))
		ret |= 1;

	return (ret);
}

LOCAL void
pmask(std, do_posix)
	FILE	*std[];
	BOOL	do_posix;
{
	mode_t	m;

	m = getcmask();
	m = ~m;

	if (do_posix)
		fprintf(std[1], "u=%s,g=%s,o=%s\n",
			umodtab[mval(m & S_IRWXU)],
			umodtab[mval(m & S_IRWXG)],
			umodtab[mval(m & S_IRWXO)]);
	else
		fprintf(std[1], "%s %s %s\n",
			modtab[mval(m & S_IRWXU)],
			modtab[mval(m & S_IRWXG)],
			modtab[mval(m & S_IRWXO)]);
}

/* ARGSUSED */
EXPORT void
blogout(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	if (not_loginsh(std)) {
		fprintf(std[2], "use exit to exit.\n");
		ex_status = 1;
		return;
	}
	exitbsh(0);
}

/* ARGSUSED */
EXPORT void
becho(vp, std, flag)
	register Argvec	*vp;
	register FILE	*std[];
		int	flag;
{
	register int	i = 1;
	register int	first = 1;
	register int	ac = vp->av_ac;
	register char **av = vp->av_av;
	register FILE	*output;
		char	buf[4096];
		BOOL	nnl;
		BOOL	glob;

	glob = streql(av[0], "glob");
	output = streql(av[0], "err") ? std[2] : std[1];

	if (!glob && std[0] != stdin && ac == 1) {
		file_raise(std[0], FALSE);
		file_raise(output, FALSE);
		do {
			i = fileread(std[0], buf, sizeof (buf));
			if (i > 0)
				filewrite(output, buf, i);
		} while (i > 0 && !ctlc);
	} else {
		nnl = glob;
		if (!glob && ac > 1 && (streql(av[1], "-nnl")||streql(av[1], "-n")))
			i++, nnl = TRUE;
		for (; i < ac && !ctlc; i++) {
			if (first)
				first--;
			else
				glob ? fputc('\0', output) : fprintf(output, " ");
			fprintf(output, "%s", av[i]);
		}
		if (!nnl && ac > 1)
			fprintf(output, "%s", nl);
	}
}

/* ARGSUSED */
EXPORT void
bshift(vp, std, flag)
	register	Argvec	*vp;
			FILE	*std[];
			int	flag;
{
	register int 	i,
			k;
		int	count = 1;

	if (vp->av_ac > 2) {
		wrong_args(vp, std);
		return;
	}
	if (vp->av_ac == 2)
		if (!toint(std, vp->av_av[1], &count))
			return;
	for (k = 0; k < count; k++) {
		if (vac <= 1) {
			if (vac)
				fprintf(std[2], "%s: ", vav[0]);
			fprintf(std[2], "cannot shift.\n");
			ex_status = 1;
			return;
		}
		for (i = 1; i <= vac; i++)
			vav[i] = vav[i+1];
		vac--;
	}
}

/* ARGSUSED */
EXPORT void
bremap(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
#ifdef	INTERACTIVE
	remap();
#else
	unimplemented(vp, std);
#endif
}

#ifdef	INTERACTIVE
/* ARGSUSED */
EXPORT void
bsavehist(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	save_history(TRUE);
}
#endif	/* INTERACTIVE */

/* ARGSUSED */
EXPORT void
bhistory(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
#ifdef	INTERACTIVE
	put_history(std[1], TRUE);
#else
	hi_list(std[1]);
#endif
}

/* ARGSUSED */
EXPORT void
bsource(vp, std, flag)
	register	Argvec	*vp;
			FILE	*std[];
			int	flag;
{
	extern	int	do_status;
		int	ac = vp->av_ac;
		char	*name = NULL;	/* Init to make gcc happy */
		char	*pname = NULL;
	register FILE	*f;

	if (ac < 2) {
#ifndef	NO_STDIN_SOURCE
		if (isatty(fdown(std[0]))) {
			wrong_args(vp, std);
			return;
		}
		doopen(std[0], "stdin", GLOBAL_AB, flag, std, FALSE);
		return;
#else
		wrong_args(vp, std);
		return;
#endif
	}
	if (vp->av_av[0][0] != '.' &&
	    streql(vp->av_av[1], "-h")) {
		if (ac > 3) {
			wrong_args(vp, std);
			return;
		}
		if (ac == 3) {
			/*
			 * Be careful and do no PATH name search when
			 * reading a shell script into the history.
			 * We like to avoid that "source -h ls" fills
			 * the history with junk.
			 */
			name = vp->av_av[2];
			if ((f = fileopen(name, for_read)) == (FILE *) NULL)
				ex_status = geterrno();
		} else {
			name = "stdin";
			f = std[0];
		}
		if (f) {
			readhistory(f);
			if (f == stdin)
				clearerr(f);
			if (f != std[0])
				fclose(f);
		}
	} else {
		pname = name = _findinpath(vp->av_av[1], R_OK, TRUE);
		if (name) {
			dofile(name, GLOBAL_AB, flag, std, FALSE);
			ex_status = do_status;
		} else {
			ex_status = geterrno();
		}
	}
	if (ex_status != 0) {
		fprintf(std[2], ecantread, name, errstr(ex_status));
		fprintf(std[2], "%s", nl);
	}
	if (pname)
		free(pname);
}

/* repeat command - execute command several times */
/* ARGSUSED */
EXPORT void
brepeat(vp, std, flag)
	register	Argvec	*vp;
	register	FILE	*std[];
			int	flag;
{
		int	ac;
		char	* const *av;
		long	count	= 0x7FFFFFFF;
		long	dtime	= 0;
	volatile long	sttime	= 0;
		long	d;
	Tnode	* volatile cmd	= (Tnode *) NULL;
		char	*opt	= "delay#L,d#L,count#L,c#L,#L";
		char	*cmdln;
	volatile sigtype intsav = SIG_DFL;

	ac = vp->av_ac - 1;		/* set values */
	av = &vp->av_av[1];
	if (getargs(&ac, &av, opt, &dtime, &dtime,
						&count, &count, &count) < 0) {
		fprintf(std[2], ebadopt, vp->av_av[0], av[0]);
		fprintf(std[2], "%s", nl);
		busage(vp, std);
		ex_status = 1;
		return;
	}
	if (ac < 1) {
		wrong_args(vp, std);
		return;
	}
	if ((cmdln = cons_args(vp, vp->av_ac - ac)) == NULL) {
		ex_status = 1;
		return;
	}
	if (!setjmp(waitjmp)) {
		if (osig2 != (sigtype) SIG_IGN)
			intsav = signal(SIGINT, waitint);
		while (count-- > 0) {
			if (dtime) {
				if (sttime) {
					d = time(0) - sttime;
					if (d < dtime)
						sleep(dtime - d);
				}
				sttime = time(0);
			}
#ifdef DEBUGX
		printf("        brepeat: executing '%s'\n", cmdln); flush();
#endif
			if (!cmd) {
				pushline(cmdln);
/*				cmd = cmdline(0|PGRP, std, TRUE);*/
				cmd = cmdline(flag, std, TRUE);
			}
/*			execute(cmd, 0|PGRP, std);*/
			execute(cmd, flag, std);
#ifdef	JOBCONTROL
			tty_setpgrp(0, mypgrp);
#endif	/* JOBCONTROL */
			if (ctlc)
				ex_status = 1;
			if (ex_status)
				break;
		}
	}
	if (osig2 != (sigtype) SIG_IGN)
		signal(SIGINT, intsav);
	free(cmdln);
	freetree(cmd);
}

/* ARGSUSED */
EXPORT void
bexec(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	my_exec(vp->av_av[1], 1, vp, std);
}

/* ARGSUSED */
EXPORT void
blogin(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	if (!not_loginsh(std)) {
#ifdef	INTERACTIVE
		if (!no_histflg && ev_eql(savehistname, on))
			save_history(FALSE);
#endif	/* INTERACTIVE */
		my_exec(loginname, 0, vp, std);
	}
}

LOCAL void
my_exec(name, first_ac, vp, std)
			char	*name;
			int	first_ac;
	register	Argvec	*vp;
			FILE	*std[];
{
	char	**av = &vp->av_av[first_ac];
	int	ac   = vp->av_ac - first_ac;
	char	*av0 = NULL;

	if (getargs(&ac, (char * const **)&av, "av0*", &av0) < 0) {
		fprintf(std[2], ebadopt, vp->av_av[0], av[0]);
		fprintf(std[2], "%s", nl);
		busage(vp, std);
		ex_status = 1;
		return;
	}
	if (ac < 1) {
		wrong_args(vp, std);
		return;
	}
	name = makestr(name);
	if (av0) {
		free(name);
		name = makestr(av[0]);
		av0 = makestr(av0);
		free(av[0]);
		av[0] = av0;
	}
	update_cwd();			/* Er koennte sie veraendert haben */
	setpgid(0, opgrp);
#ifdef	JOBCONTROL
	tty_setpgrp(fdown(std[0]), opgrp);
#endif	/* JOBCONTROL */
	ex_status = fexec((char **)NULL, name, std[0], std[1], std[2],
								av, evarray);
	setpgid(0, mypgrp);
#ifdef	JOBCONTROL
	tty_setpgrp(fdown(std[0]), mypgrp);
#endif	/* JOBCONTROL */
	fprintf(std[2], "Can't exec '%s'. %s\n", name, errstr(ex_status));
	free(name);
}

/* ARGSUSED */
EXPORT void
berrstr(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	int	err;

	if (!toint(std, vp->av_av[1], &err))
		return;
	fprintf(std[1], "%s", errstr(err));
}

/* ARGSUSED */
EXPORT void
btype(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	register int	ac = vp->av_ac;
	register char **av = vp->av_av;
	register int	i;
	register FILE	*output = std[1];
	char		*val	= NULL;

	if (vp->av_ac < 2) {
		wrong_args(vp, std);
		return;
	}

	for (i = 1; i < ac && !ctlc; i++) {
		fprintf(output, "%s ", av[i]);
		if ((val = ab_value(LOCAL_AB, av[i], NULL, AB_BEGIN)) != NULL) {
			fprintf(output, "is a local alias to '%s'\n", val);
		} else if ((val = ab_value(GLOBAL_AB, av[i], NULL, AB_BEGIN)) != NULL) {
			fprintf(output, "is a global alias to '%s'\n", val);
		} else if (blook(av[i], bitab, n_builtin) != (btab *) NULL) {
			fprintf(output, "is a shell builtin\n");
		} else if ((val = map_func(av[i])) != NULL) {
			fprintf(output, "is a function '%s'\n", val);
		} else if ((val = _findinpath(av[i], X_OK, TRUE)) != NULL) {
			fprintf(output, "is %s\n", val);
			free(val);
			val = NULL;
		} else {
			fprintf(output, "not found\n");
		}
	}
}

/* ARGSUSED */
EXPORT void
btrue(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	ex_status = 0;
}

/* ARGSUSED */
EXPORT void
bfalse(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	ex_status = 1;
}

#ifdef	DO_FIND
#include <schily/walk.h>
#include <schily/find.h>

LOCAL int
quitfun(arg)
	void	*arg;
{
	return (*((int *)arg) != 0);
}

/* ARGSUSED */
EXPORT void
bfind(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	squit_t	quit;

	quit.quitfun = quitfun;
	quit.qfarg = &ctlc;
	ex_status = find_main(vp->av_ac, vp->av_av, evarray, std, &quit);
}
#endif

/* ARGSUSED */
EXPORT BOOL
builtin(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	register btab	*bp;
	register char	*name = vp->av_av[0];
		struct rusage	ru1;
		struct rusage	ru2;
		struct rusage	cru1;
		struct rusage	cru2;

	if (ctlc)
		return (TRUE);
	setstime();
	getpruself(&ru1);
	getpruchld(&cru1);
	if ((bp = blook(name, bitab, n_builtin)) == (btab *) NULL)
		return (FALSE);
	ex_status = 0;
	if (!helpwanted(vp, std)) {
		if (bp->b_argc && vp->av_ac != bp->b_argc)
			wrong_args(vp, std);
		else
			(*bp->b_func)(vp, std, flag);
	}
	fflush(std[1]);
	fflush(std[2]);
	if ((flag & NOTMS) == 0 && getcurenv("TIME")) {
		getpruself(&ru2);
		getpruchld(&cru2);
		rusagesub(&ru1, &ru2);
		rusagesub(&cru1, &cru2);
		rusageadd(&cru2, &ru2);
		prtimes(gstd, &ru2);
	}
	return (TRUE);
}

#ifdef	PROTOTYPES
LOCAL int
suspend(pid_t p)
#else
LOCAL int
suspend(p)
	pid_t	p;
#endif
{
#ifdef	SIGSTOP
	return (kill(p, SIGSTOP));
#else
	raisecond("suspend not implemented", 0L);
#endif
}

#ifdef	PROTOTYPES
LOCAL int
presume(pid_t p)
#else
LOCAL int
presume(p)
	pid_t	p;
#endif
{
#ifdef	SIGCONT
	if (p == getpid()) {
		seterrno(ESRCH);
		return (-1);
	}
	return (kill(p, SIGCONT));
#else
	raisecond("presume not implemented", 0L);
#endif
}

#ifdef	PROTOTYPES
LOCAL mode_t
setcmask(mode_t newmask)
#else
LOCAL mode_t
setcmask(newmask)
	mode_t	newmask;
#endif
{
	return (umask(newmask));
}

LOCAL mode_t
getcmask()
{
	mode_t	ret =  umask(0);

	umask(ret);
	return (ret);
}
