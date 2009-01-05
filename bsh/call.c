/* @(#)call.c	1.17 08/12/20 Copyright 1985-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)call.c	1.17 08/12/20 Copyright 1985-2008 J. Schilling";
#endif
/*
 *	Call bsh defined functions and signal handlers
 *
 *	Copyright (c) 1985-2008 J. Schilling
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

#include <schily/mconfig.h>
#include <stdio.h>
#include <signal.h>
#include "bsh.h"
#include "node.h"
#include "btab.h"
#include "str.h"
#include "strsubs.h"
#include <schily/stdlib.h>
#include <schily/string.h>

#define	NSIGNALS	32
#define	SIG_DEF		(sigtype) SIG_DFL

typedef struct functions {
		char	  *f_name;
		char	  *f_val;
	struct	functions *f_next;
} _FUNCS, *FUNCS;

LOCAL FUNCS	funcs = (FUNCS) NULL;
LOCAL int	in_function = 0;
LOCAL int	do_return = 0;

char		*sigcmds[NSIGNALS + 1] = {0};
int		sigcount[NSIGNALS + 1] = {0};
sigtype		ofunc[NSIGNALS + 1] = {
    SIG_DEF,
    SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF,
    SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF,
    SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF,
    SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF, SIG_DEF
};

LOCAL	sigret	sig		__PR((int signo));
EXPORT	void	bsignal		__PR((Argvec * vp, FILE ** std, int flag));
LOCAL	void	print_sigs	__PR((FILE * f));
LOCAL	void	set_some_sigs	__PR((Argvec * vp, FILE ** std));
LOCAL	void	set_sig		__PR((FILE ** std, int signo, char *cmd));
LOCAL	void	del_sig		__PR((FILE ** std, int signo));
EXPORT	void	esigs		__PR((void));
LOCAL	void	sig_cmd		__PR((int signo, char *cmdl));
LOCAL	void	call_cmd	__PR((char *cmdl, FILE ** std));
EXPORT	void	bfunc		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	breturn		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	BOOL	func_call	__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	char	*map_func	__PR((char *name));
LOCAL	void	set_func	__PR((Argvec * vp));
LOCAL	void	del_func	__PR((Argvec * vp, FILE ** std));
LOCAL	void	unknown_func	__PR((FILE ** std, char *name));
LOCAL	void	pr_funcs	__PR((FILE * f));
LOCAL	void	pr_func		__PR((FILE * f, char *name, char *val));

LOCAL sigret
sig(signo)
	int signo;
{
	signal(signo, sig);
	sigcount[signo]++;
#ifdef	DEBUG
	printf("sig:  pid: %d sigcount[%d]: %d.\n",
				getpid(), signo, sigcount[signo]);
#endif
}

/* ARGSUSED */
EXPORT void
bsignal(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	if (vp->av_ac == 1)
		print_sigs(std[1]);
	else if (vp->av_ac == 2)
		wrong_args(vp, std);
	else
		set_some_sigs(vp, std);
}

LOCAL void
print_sigs(f)
	FILE	*f;
{
	register	int	i;

	for (i = 1; i <= NSIGNALS; i++) {
		if (sigcmds[i])
			fprintf(f, "%2d: %s\n", i, sigcmds[i]);
	}
}

LOCAL void
set_some_sigs(vp, std)
	Argvec	*vp;
	FILE	*std[];
{
			int	signo;
	register	char	**av = &(vp->av_av[2]);

	for (; *av; av++) {
		if (!toint(std, *av, &signo)) {
			return;
		} else if (signo < 1 || signo > NSIGNALS) {
			ex_status = 1;
			fprintf(std[2], "bad signo: %d.\n", signo);
		} else {
			if (strlen(vp->av_av[1]))
				set_sig(std, signo, vp->av_av[1]);
			else
				del_sig(std, signo);
		}
	}
}

LOCAL void
set_sig(std, signo, cmd)
		FILE	*std[];
	register int	signo;
		char	*cmd;
{
	if (sigcmds[signo])
		del_sig(std, signo);
	sigcmds[signo] = makestr(cmd);
	sigcount[signo] = 0;
	if (signo == SIGINT) {
		ofunc[signo] = signal(SIGINT, intr);
		if (ofunc[signo] != intr)
			signal(SIGINT, sig);
	} else {
		ofunc[signo] = signal(signo, sig);
	}
}

LOCAL void
del_sig(std, signo)
		FILE	*std[];
	register int	signo;
{
	if (sigcmds[signo]) {
		free(sigcmds[signo]);
		sigcmds[signo] = NULL;
		signal(signo, ofunc[signo]);
	} else {
		fprintf(std[2], "Signal #%d not set.\n", signo);
		ex_status = 1;
	}
}

EXPORT void
esigs()
{
	register int	i;
	register char	**rsigcmds = sigcmds;
	register int	*rsigcount = sigcount;

	for (i = 1; i <= NSIGNALS; i++) {
		if (rsigcount[i]) {
#ifdef	DEBUG
			printf("esigs: sigcount[%d]: %d, sigcmds[%d]: %s.\n",
					i, rsigcount[i], i, rsigcmds[i]);
#endif
			rsigcount[i] = 0;
			if (rsigcmds[i])
				sig_cmd(i, rsigcmds[i]);
		}
	}
}

LOCAL void
sig_cmd(signo, cmdl)
	int	signo;
	char	*cmdl;
{
	char	buf[11];
	int	ctlcsave = ctlc;

	ctlc = 0;
	sprintf(buf, "%d", signo);
	ev_insert(concat("signo", eql, buf, (char *)NULL));
	call_cmd(cmdl, gstd);
	ev_delete("signo");
	ctlc = ctlcsave;
}

LOCAL void
call_cmd(cmdl, std)
	char	*cmdl;
	FILE	*std[];
{
	extern int 	delim;
		int	s_delim = delim;

	extern int	level,			/* aus cond.c	*/
			idsp,
			idlen,
			read_delim,
			*idstack;
	extern char	*firstp,		/* ist eigentlich MSTK  */
			*stackp,
			*foundline;

		int	s_level = level,
			s_idsp = idsp,
			s_idlen = idlen,
			s_read_delim = read_delim,
			*s_idstack = idstack;
		char	*s_firstp = firstp,
			*s_stackp = stackp,
			*s_foundline = foundline;


	in_function++;
	level = 0;				/* definierter Anfangswert */
	idsp = 0;
	idlen = 0;
	read_delim = 0;
	idstack = (int *) NULL;
	firstp = NULL,
	stackp = NULL,
	foundline = NULL;

	delim = EOF;
	pushline(cmdl);
	do {
		freetree(cmdline(0, std, FALSE));
#ifdef	DEBUG
		printf("das war cmdline.\n");
#endif
		if (do_return) {
			--ctlc, --do_return;
			if (delim != EOF)
				while (nextch() != EOF);
		}
	} while (delim != EOF);
	if (idstack)
		freestack();
	delim = s_delim;
	level = s_level;
	idsp = s_idsp;
	idlen = s_idlen;
	read_delim = s_read_delim;
	idstack = s_idstack;
	firstp = s_firstp;
	stackp = s_stackp;
	foundline = s_foundline;
	in_function--;
}

/* ARGSUSED */
EXPORT void
bfunc(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	register int 	ac = vp->av_ac;
	register char	**av = vp->av_av;
		char	*cmd;

	if (ac == 1) {
		pr_funcs(std[1]);
	} else if (ac == 2) {
		if (!(cmd = map_func(av[1]))) {
			unknown_func(std, av[1]);
			return;
		}
		pr_func(std[2], av[1], cmd);
	} else if (ac > 3) {
		wrong_args(vp, std);
	} else {
		if (strlen(av[2]))
			set_func(vp);
		else
			del_func(vp, std);
	}
}

/* ARGSUSED */
EXPORT void
breturn(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	if (!in_function) {
		bnallo(vp, std, flag);
	} else if (vp->av_ac > 2) {
		wrong_args(vp, std);
	} else {
		if (vp->av_ac == 2)
			toint(std, vp->av_av[1], &ex_status);
		ctlc++;						/* Hihi */
		do_return++;
	}
}

/* ARGSUSED */
EXPORT BOOL
func_call(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	char	*cmd;

	if (!(cmd = map_func(vp->av_av[0])))
		return (FALSE);
	if (vp->av_ac != 1)
		wrong_args(vp, std);
	else
		call_cmd(cmd, std);
	return (TRUE);
}

EXPORT char *
map_func(name)
	register char	*name;
{
	register FUNCS	rf = funcs;
	register int	cmp;

	while (rf) {
		if ((cmp = strcmp(name, rf->f_name)) < 0)
			return (NULL);
		if (cmp == 0)
			break;
		rf = rf->f_next;
	}
	if (!rf)
		return (NULL);
	return (rf->f_val);
}

LOCAL void
set_func(vp)
	Argvec	*vp;
{
	register char	*name = vp->av_av[1];
	register FUNCS	rf = funcs;
	register FUNCS	rf1 = funcs;
	register FUNCS	new;
	register int	cmp;

	new = (FUNCS)malloc(sizeof (_FUNCS));
	new->f_name = makestr(name);
	new->f_val = makestr(vp->av_av[2]);

	while (rf) {
		if ((cmp = strcmp(name, rf->f_name)) < 0)
			break;
		if (cmp == 0) {
			free(rf->f_name);
			free(rf->f_val);
			rf->f_name = new->f_name;
			rf->f_val = new->f_val;
			free((char *) new);
			return;
		}
		rf1 = rf;
		rf = rf->f_next;
	}

	if (rf == rf1) {	/* erster Eintrag */
		new->f_next = rf1;
		funcs = new;
	} else {
		new->f_next = rf1->f_next;
		rf1->f_next = new;
	}
}

LOCAL void
del_func(vp, std)
	Argvec	*vp;
	FILE	*std[];
{
	register char	*name = vp->av_av[1];
	register FUNCS	rf = funcs;
	register FUNCS	rf1 = funcs;

	while (rf) {
		if (streql(name, rf->f_name))
			break;
		rf1 = rf;
		rf = rf->f_next;
	}
	if (!rf) {
		unknown_func(std, name);
		return;
	}
	if (rf == rf1) 	/* erster Eintrag */
		funcs = rf1->f_next;
	else
		rf1->f_next = rf->f_next;

	free(rf->f_name);
	free(rf->f_val);
	free((char *) rf);
}

LOCAL void
unknown_func(std, name)
	FILE	*std[];
	char	*name;
{
	fprintf(std[2], "Function '%s' unknown.\n", name);
	ex_status = 1;
}

LOCAL void
pr_funcs(f)
	register FILE	*f;
{
	register FUNCS	rf = funcs;

	while (rf) {
		pr_func(f, rf->f_name, rf->f_val);
		rf = rf->f_next;
	}
}

LOCAL void
pr_func(f, name, val)
	FILE	*f;
	char	*name;
	char	*val;
{
	fprintf(f, "function %s '%s'\n", name, quote_string(val, "\n'"));
}
