/* @(#)exec.c	1.62 12/04/26 Copyright 1985-2012 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)exec.c	1.62 12/04/26 Copyright 1985-2012 J. Schilling";
#endif
/*
 *	bsh command interpreter - Execution of parsed Tree
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
#include "bsh.h"
#include "node.h"
#include "str.h"
#include "strsubs.h"
#include <schily/string.h>
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/time.h>
#include <schily/resource.h>
#define	VMS_VFORK_OK
#include <schily/vfork.h>

/*#define	EXDEBUG*/
#ifdef	EXDEBUG
#define	DBG(txt, c)	{berror("%s(%X)", txt, flag); doverbose(c); usleep(200000); }
#else
#define	DBG(txt, c)
#endif

	pid_t	lastbackgrnd = 0;

#ifdef	JOBCONTROL
	pid_t	pgrp;
#endif

extern	int	eflg;
extern	int	nflg;
extern	int	ttyflg;
extern	pid_t	mypid;
extern	int	ex_status;

EXPORT	int	execute		__PR((Tnode * cmd, int flag, FILE ** std));
LOCAL	void	doverbose	__PR((Tnode * cmd));
LOCAL	void	doprot		__PR((Tnode * cmd));
LOCAL	int	econd		__PR((Tnode * cmd, int flag, FILE ** std));
LOCAL	int	efilter		__PR((Tnode * cmd, int flag, FILE ** std));
LOCAL	int	ecmd		__PR((Tnode * cmd, volatile int flag, FILE ** std, FILE * toclose));
EXPORT	int	execcmd		__PR((Argvec *vp, FILE *std[], int flag));
LOCAL	void	lower_my_pri	__PR((void));
EXPORT	Argvec*	scan		__PR((Tnode * cmd));
LOCAL	int	backeval	__PR((char *s, Tnode **npp));
LOCAL	int	parseback	__PR((FILE *f, Tnode **npp));
LOCAL	Tnode*	expand_string	__PR((Tnode *np));
/*LOCAL	void	trim		__PR((char *s));*/
/*LOCAL	void	trim1		__PR((char *s));*/
LOCAL	int	newio		__PR((Tnode * list, FILE ** old, FILE ** new, int flag));
LOCAL	void	closeio		__PR((FILE ** o, FILE ** n));
LOCAL	FILE	*openio		__PR((Tnode *np, char *omode, int type));
#ifdef	DEBUG
LOCAL	void	pfiles		__PR((void));
#endif

/*
 * This is the top level function that is called when a command is executed.
 *
 * Returns
 *		0	OK
 *		!= 0	failed
 */
EXPORT int
execute(cmd, flag, std)
	register Tnode	*cmd;		/* Parsed command tree */
		int	flag;
		FILE	*std[];
{
	int	newflag;

	DBG("execute", cmd);
	if (nflg)			/* Parse only */
		return (0);

	while (cmd != (Tnode *) NULL && (ntype(cmd) == '&' || ntype(cmd) == ';')) {
		newflag = flag;
		/*
		 * Wird das SUBSH flag gebraucht???
		 * dann musz evt. SUBSH in alle builtuins, die
		 * cmdline oder execute aufrufen.
		 */
/*		if (!(flag & SUBSH))*/
/*			newflag &= ~PGRP;*/
		if (ntype(cmd) == '&') {
			if (ev_eql("NOBACKGROUND", on)) {
				berror(erestricted, "&");
				return (-1);
			} else {
				newflag |=
					(BGRND|ASYNC|PRPID|NOSIG|LOPRI|NULIN);
			}
		}
		if (execute(cmd->tn_left.tn_node, newflag, std))
/*XXX ???	if (ctlc) */
			return (-1);
		if (!ttyflg && eflg && ex_status)
			return (-1);
		cmd = cmd->tn_right.tn_node;
	}
	if (vflg)
		doverbose(cmd);
	if (protfile)
		doprot(cmd);

#ifdef	JOBCONTROL__off
	if (!(flag & PGRP)) {
		pgrp = mypid;
/*		pgrp = 0;*/
		flag |= PGRP;
	}
#endif
	return (econd(cmd, flag, std));
}

LOCAL void
doverbose(cmd)
	Tnode	*cmd;
{
	if (cmd != (Tnode *)NULL) {
		fprintf(stderr, "{ ");
		printtree(stderr, cmd);
		fprintf(stderr, "}\n");
		fflush(stderr);
	}
}

LOCAL void
doprot(cmd)
	Tnode	*cmd;
{
	if (cmd != (Tnode *)NULL) {
		lseek(fdown(protfile), (off_t)0, SEEK_END);
		printtree(protfile, cmd);
		fprintf(protfile, "%s", nl);
		fflush(protfile);
	}
}

/*
 * Executes "cmd1 && cmd1" and "cmd1 || cmd2" pipelines.
 *
 * If the pipeline is asynchronous, it forks the child and
 * returns immediately as parent.
 * It handles further directives for the child (ASYNC) or parent (SYNC).
 *
 * Returns
 *		0	OK
 *		!= 0	failed
 */
LOCAL int
econd(cmd, flag, std)
	register Tnode	*cmd;
		int	flag;
		FILE	*std[];
{
	register int	i = 0;
		int	didfork = 0;
	register int	stop = 0;
		pid_t	child;

	DBG("econd", cmd);
	if (cmd != (Tnode *) NULL &&
		    (ntype(cmd) == ANDP || ntype(cmd) == ORP) && (flag & ASYNC)) {
		if ((child = shfork(flag)) != 0) {
			if (child < 0) {
				ex_status = geterrno(); /* XXX falsch in shfork() */
				return (-1);		/* Cannot fork */
			} else {
				return (0);
			}
		} else {
			flag |= SUBSH;
			flag &= ~(ASYNC | PRPID);
			didfork++;
		}
	}

	while (!stop && cmd != (Tnode *) NULL &&
		    (ntype(cmd) == ANDP || ntype(cmd) == ORP)) {
		i = efilter(cmd->tn_left.tn_node, flag, std);
		if (i) {
			stop++;				/* Cannot make pipe */
		} else {
			if ((ex_status && ntype(cmd) == ANDP) ||
			    (!ex_status && ntype(cmd) == ORP)) {
				i = 0;
				stop++;
			} else {
				cmd = cmd->tn_right.tn_node;
			}
		}
	}
	if (!stop)
		i = efilter(cmd, flag, std);
	if (didfork) {			/* Wann kommen wir hier eigentlich hin ? */
					/* Wahrscheinlich nur bei pipe failed */
/*write(STDERR_FILENO, "exit 0\n", 7);*/
		_exit(i);
	}
	return (i);
}


/*
 * Returns
 *		0	OK
 *		!= 0	failed
 */
LOCAL int
efilter(cmd, flag, std)
	register Tnode	*cmd;
		int	flag;
	register FILE	*std[];
{
		FILE	*newstd[3];
	register FILE	**nstd = newstd;
		FILE	*pipef[2];
	register FILE	*pf;
	register int	i;

	DBG("efilter", cmd);
	if (cmd == (Tnode *) NULL)
		return (0);
#ifdef	JOBCONTROL
/*	pgrp = mypid;*/
#endif
#ifdef	JOBCONTROL
	if (!(flag & PGRP)) {
		pgrp = mypid;
/*		flag |= PGRP;*/
	}
/*	else if (flag & SUBSH) {*/
/*		pgrp = getpid();*/
/*	}*/
#ifdef	EXDEBUG
	berror("pgrp: %ld pid: %ld mypid: %ld", (long)pgrp, (long)getpid(), (long)mypid);
#endif
#endif	/* JOBCONTROL */
	for (i = 0; i < 3; i++)
		nstd[i] = std[i];			/* Copy stdin/stdout/stderr */
	pf = (FILE *) NULL;				/* No input from pipe input */
	while (ntype(cmd) == '|' || ntype(cmd) == ERRPIPE) {
		if (fpipe(pipef) == 0) {
			berror(ecantcreate, pipename, errstr(geterrno()));
			if (pf != (FILE *) NULL)
				fclose(pf);
			return (1);			/* No pipe, abort */
		} else {
			if (ntype(cmd) == '|') {
				nstd[1] = pipef[1];	/* Redirect stdout */
			} else {
				nstd[2] = pipef[1];	/* Redirect stderr */
			}
/*			i = ecmd(cmd->tn_left.tn_node, flag | ASYNC, nstd, pipef[0]);*/
			i = ecmd(cmd->tn_left.tn_node, flag | ASYNC|PGRP, nstd, pipef[0]);
			if (pf != (FILE *) NULL)
				fclose(pf);
			fclose(pipef[1]);
			pf = nstd[0] = pipef[0];	/* Redirect stdin of next cmd */
			flag &= ~NULIN;
			if (i)
				return (i);		/* If exec failed, abort */
		}
		cmd = cmd->tn_right.tn_node;
	}
	nstd[1] = std[1];				/* set back old stdout*/
	nstd[2] = std[2];				/* set back old stderr*/
	i = ecmd(cmd, flag, nstd, (FILE *) NULL);
	if (pf != (FILE *) NULL)
		fclose(pf);
	return (i);
}


/*
 * Returns
 *		0	OK
 *		!= 0	failed
 */
LOCAL int
ecmd(cmd, flag, std, toclose)
	register Tnode	*cmd;
	volatile int	flag;
		FILE	*std[];
		FILE	*toclose;
{
		FILE	*newstd[3];
		pid_t	child = (pid_t)-1;
	volatile int	didfork = 0;
		Argvec	*vp;
	volatile int	cstat = 0;

	DBG("ecmd", cmd);
	if (cmd == (Tnode *) NULL)
		raisecond("!ecmd", (long)NULL);
	if (cmd->tn_left.tn_node == (Tnode *) NULL) {
		/*
		 * Null command, should never happen.....
		 */
		berror("%s", enullcmd);
		return (-1);
	}

	if (flag & ASYNC) {
		/*
		 * If we don't wait for this command fork now.
		 */
		if ((child = shfork(flag)) != 0) {
			if (child < 0) {
				ex_status = geterrno(); /* XXX falsch in shfork() */
				return (-1);		/* Fork failed */
			} else {
				/*
				 * Father returns.
				 */
				if (flag & BGRND)
					lastbackgrnd = child;
				return (0);
			}
		} else {
			/*
			 * This is the Child (child == 0), exec later.
			 */
			flag |= SUBSH;
			didfork++;
		}
		/* XXX ??? mit in else ??? */
#ifdef	DEBUG
		fprintf(stderr,
			"CLOSING: toclose: %d\n", toclose?fdown(toclose):-1);
		fflush(stderr);
#endif
		if (toclose != (FILE *) NULL) {
			fclose(toclose);
			toclose = (FILE *) NULL;
		}
		if (flag & LOPRI)
			lower_my_pri();
	}
	if (!newio(cmd->tn_right.tn_node, std, newstd, flag)) {
		/*
		 * Could not open new I/O
		 */
		ex_status = 1;
		cstat = 1;
	} else if (ntype(cmd) == '(') {
#define	OLD_LISTCODE
#ifdef	OLD_LISTCODE
		cstat = execute(cmd->tn_left.tn_node,
			(flag|SUBSH) & ~(ASYNC|PRPID|LOPRI|NULIN), newstd);
#else	/* OLD_LISTCODE */
		child = shfork(flag);
		if (child < 0)
			return (-1);		/* Fork failed */
		if (child == 0) {		/* The Child */
			flag |= SUBSH;
/*			berror("closefiles1(%d)", toclose);*/
			if (!didfork && (flag & LOPRI))
				lower_my_pri();
			cstat = execute(cmd->tn_left.tn_node,
				(flag|SUBSH) & ~(ASYNC|PRPID|LOPRI|NULIN), newstd);
			_exit(cstat);
		} else {			/* Vather */
			/*
			 * Wait non interruptable.
			 */
			cstat = ewait(child, WALL);
		}
#endif	/* OLD_LISTCODE */
	} else {					/* simple command */
		vp = scan(cmd->tn_left.tn_node);		/* convert to Argvec */
		if (cmd->tn_left.tn_node->tn_type & NENV)
			flag |= ENV;
		if (didfork)
			flag |= DIDFORK;
		cstat = execcmd(vp, newstd, flag);
		freevec(vp);
	}
	closeio(std, newstd);			/* close the files from newio */
	if (didfork) {				/* Hier kommen builtins aus dem Background */
						/* und redirect Fehler */
/*write(STDERR_FILENO, "exit 1\n", 7);*/
		_exit(0);			/* XXX Was ist mit dem exit code ? */
	}
	return (cstat);
}

EXPORT int
execcmd(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
		pid_t	child = (pid_t)-1;
	volatile int	didfork = 0;
	volatile int	cstat = 0;

	if (flag & DIDFORK) {
		flag &= ~DIDFORK;
		didfork++;
		child = 0;
	}
	/*
	 * POSIX order:
	 * 1) special builtins (break, ':', continue, '.', eval, exec, exit,
	 *			export, readonly, return, set, shift, times,
	 *			trap, unset)
	 *	ksh does not allo these special names to be defines as function
	 *
	 * 2) functions
	 * 3) other builtins
	 */
	if (!builtin(vp, std, flag) && !func_call(vp, std, flag)) {

		setstime();			/* start of proctime	 */

		/*
		 * XXX Bei längeren Pipelines (z.B. foo | bar) sind beim bsh
		 * XXX alle Prozesse Kinder vom bsh.
		 * XXX Bei Solaris und multi CPU System wird "foo" durch den
		 * XXX bsh per fork() erzeugt, der Vater läuft zuerst los und
		 * XXX das Kind, welches der "Processgroupleader" werden soll,
		 * XXX erzeugt die neue Prozessgruppe erstmal nicht. Wenn dann
		 * XXX "bar" per vfork erzeugt werden soll, dann würde dieses
		 * XXX Kind zuerst loslaufen und damit sowohl den Vater als auch
		 * XXX das Kind "foo" blockiert werden bis "bar" über exec
		 * XXX gestartet wird. Daher kann "bar" seine Prozessgruppe
		 * XXX nicht aufsetzen.
		 * XXX Die richtige Lösung wäre alle Kinder über vfork() zu
		 * XXX erzeugen...
		 * XXX Eine Notlösung: "bar" nicht per vfork erzeugen.
		 */
#ifdef	NO_VFORK_ON_PIPES
		if (mypid == pgrp)
			flag |= NOVFORK;
#endif
		if (didfork) {
			/*EMPTY*/
			;

			/*
			 * Did not fork yet, so do it now.
			 */
		} else if (flag & NOVFORK) {
			child = shfork(flag);
		} else {
#ifdef	VFORK
			block_sigs();
			child = vfork();
			if (child < 0) {
				int	err = geterrno();

				unblock_sigs();
				berror(ecantvfork, errstr(err));
				return (-1);	/* Vfork failed */
			}
			if (child > 0) {
				if (Vlist) {
					free(Vlist);
					Vlist = 0;
				}
				if (Vtmp) {
					free(Vtmp);
					Vtmp = 0;
				}
				if (Vav) {
					free(Vav);
					Vav = 0;
				}
			}
			if (child >= 0)
				pset(child, flag);	/* vfork only ?? */
			unblock_sigs();
#else
			child = shfork(flag);
#endif
		}
		if (child < 0)
			return (-1);		/* Fork failed */
		if (child == 0) {		/* The Child */
			flag |= SUBSH;
/*			berror("closefiles2(%d)", toclose);*/
			if (!didfork && (flag & LOPRI))
				lower_my_pri();
			start(vp, std, flag);
		} else {			/* The Vather */
			/*
			 * Wait non interruptable.
			 */
			cstat = ewait(child, WALL);
		}
	} else {
		cstat = 0;
	}
	return (cstat);
}


#ifdef	HAVE_SETPRIORITY	/* BSD-4.x */
LOCAL void
lower_my_pri()
{
		pid_t	c_pid;
		int	pri;

	c_pid = getpid();
	seterrno(0);
	pri = getpriority(PRIO_PROCESS, c_pid);
	if (pri == -1 && geterrno())
		return;
	pri += 5;
	setpriority(PRIO_PROCESS, c_pid, pri);
}
#else	/* !HAVE_SETPRIORITY */

#ifdef	HAVE_NICE
LOCAL void
lower_my_pri()
{
	nice(5);
}
#else
LOCAL void
lower_my_pri()
{
	printf("Cannot set priority on this OS.\n");
}
#endif

#endif	/* HAVE_SETPRIORITY */


EXPORT Argvec *
scan(cmd)
	register Tnode	*cmd;
{
	register Argvec	*vp;
	register Tnode	*ep;
	register Tnode	*epl;
	register int	i;
	register unsigned len;

	ep = epl = allocnode(STRING, (Tnode *) NULL, (Tnode *) NULL);
	while (cmd != (Tnode *) NULL) {
		if (quotetype(cmd) == BQUOTE) {		/* Insert `cmd` outp. */
			Tnode	*np = cmd;

			backeval(cmd->tn_left.tn_str, &np); /* return code? */
			cmd = np;
		}
		epl->tn_right.tn_node = expand_string(cmd);
		while (epl->tn_right.tn_node != (Tnode *) NULL)
			epl = epl->tn_right.tn_node;
		cmd = cmd->tn_right.tn_node;
	}
	epl = ep->tn_right.tn_node;
	free((char *) ep);				/* koennte *ep zerstoeren */
	ep = epl;
	len = listlen(ep);
	vp = allocvec(len);
	for (i = 0; i < len; i++) {
		vp->av_av[i] = ep->tn_left.tn_str;
		epl = ep->tn_right.tn_node;
		free((char *) ep);			/* koennte *ep zerstoeren */
		ep = epl;
	}
	return (vp);
}

LOCAL int
backeval(s, npp)
	char	*s;
	Tnode	**npp;
{
	FILE    *std[3];
	FILE	*pipef[2];
	int	i;
	int	flag = NOPGRP;
	int	child;
	int	cstat;

	for (i = 0; i < 3; i++)
		std[i] = gstd[i];		/* Copy stdin/stdout/stderr */

	if (fpipe(pipef) == 0) {
		berror(ecantcreate, pipename, errstr(geterrno()));
		return (1);			/* No pipe, abort */
	} else {
		std[1] = pipef[1];		/* Redirect stdout */
	}

	if ((child = shfork(flag)) != 0) {
		if (child < 0) {
			return (-1);		/* Cannot fork */
		}
		fclose(pipef[1]);
	} else {
		fclose(pipef[0]);

		flag |= SUBSH;
		flag &= ~(ASYNC | PRPID);
		pushline(s);
		freetree(cmdline(flag, std, FALSE));
		_exit(ex_status);
	}
	parseback(pipef[0], npp);		/* Parse cmd output */
	fclose(pipef[0]);
	cstat = ewait(child, NOTMS);

	return (cstat);
}

LOCAL int
parseback(f, npp)
	FILE	*f;
	Tnode	**npp;
{
		char	buf[1024];
		char	line[1024];
	register char	*p1 = NULL;	/* Keep GCC happy ;-) */
	register char	*p2;
	register int	c;
	register int	len = -1;
	register int	cnt;
		Tnode	*np = (Tnode *)NULL;
		Tnode	*this;
		Tnode	**ep = &np;

	p2 = line;
	cnt = 0;
	do {
		if (len <= 0) {
			len = fileread(f, buf, sizeof (buf));
			if (len <= 0)
				break;
			p1 = buf;
		}
		c = *p1++;
		len--;
		if (c == ' ' || c == '\t' || c == '\n') {
			if (cnt > 0) {
				*p2 = '\0';
				this = allocnode(STRING, (Tnode *)makestr(line), 0);
				*ep = this;
				ep = &this->tn_right.tn_node;
				p2 = line;
				cnt = 0;
			}
			continue;
		}
		if (cnt++ >= sizeof (line)) {
			berror("Word too long");
			freetree(np);
			np = (Tnode *)NULL;
			return (-1);
		}
		*p2++ = c;
	} while (len >= 0);

	if (npp && np) {
		*ep = (*npp)->tn_right.tn_node;
		(*npp)->tn_right.tn_node = *ep;
		*npp = np;
	}
	return (0);
}

LOCAL Tnode *
expand_string(np)
	register Tnode	*np;
{
	register Tnode	*expa;
	register char	*s = np->tn_left.tn_str;

	if (quotetype(np) != NOQUOTE || (expa = expand(s)) == (Tnode *) NULL)
		expa = allocnode(STRING, (Tnode *) makestr(s), (Tnode *) NULL);
/*	trim(expa->tn_left.tn_str);*/
	return (expa);
}

#ifdef	__nneeded__
LOCAL void
trim(s)
	register char *s;
{
	register char	c;
	register char	quotec = '\0';

	while ((c = *s) != '\0') {
		if (quotec == '\0') {
			if (c == '\'' || c == '"') {
				quotec = c;
				strcpy(s, &s[1]);	/* overlap won't work */
				continue;
			}
			if (c == '\\')
				strcpy(s, &s[1]);
		} else if (c == quotec) {
			quotec = '\0';
			strcpy(s, &s[1]);		/* overlap won't work */
			continue;
		}
		s++;
	}
}

LOCAL void
trim1(s)
	register char *s;
{
	register char	c;
	register char	quotec = '\0';

	while ((c = *s) != '\0') {
		if (c == '\'' || c == '"') {
			if (quotec == '\0') {
				quotec = c;
				strcpy(s, &s[1]);	/* overlap won't work */
			} else if (quotec && quotec == c) {
				quotec = '\0';
				strcpy(s, &s[1]);	/* overlap won't work */
			} else {
				s++;
			}
			continue;
		}
		if (quotec == '\0' && c == '\\')
			strcpy(s, &s[1]);		/* overlap won't work */
		s++;
	}
}
#endif	/* __nneeded__ */

LOCAL int
newio(list, old, new, flag)
	register Tnode	*list;
	register FILE	*old[];
	register FILE	*new[];
		int	flag;
{
	register int	i;
	register FILE	*fp;
		int	ret = 1;
		Tnode	*ep;
	static struct omod {
		int	type;
		int	fd;
		char	*omode;
	} openm[] = {
	    {	'<',	0,	for_read},
	    {	DOCIN,	0,	for_rwct},
	    {	'>',	1,	for_wct	},
	    {	OUTAPP,	1,	for_wca	},
	    {	'%',	2,	for_wct	},
	    {	ERRAPP,	2,	for_wca	},
	    {	0,	0,	NULL 	}
	};
	register struct omod *mp;

	for (i = 0; i < 3; i++)
		new[i] = old[i];
	if (flag & NULIN) {
		fp = fileopen(nulldev, for_read);
		if (fp == (FILE *) NULL) {
			ret = 0;
			berror(ecantopen, nulldev, errstr(geterrno()));
		} else {
			new[0] = fp;
		}
	}
	for (; list != (Tnode *) NULL; list = list->tn_right.tn_node) {
#ifdef	IODEBUG
		printio(stderr, list);
		error("\n");
#endif
		if (ntype(list) == ODUP) {
			int	fd = (list->tn_type >> 24) & 0xFF;
			int	fd2 = (list->tn_type >> 16) & 0xFF;

			fp = fdup(new[fd2]);
			if (fp != NULL) {
				if (new[fd] != old[fd])
					fclose(new[fd]);
				new[fd] = fp;
			} else {
				ret = 0;
				printio(stderr, list);
				berror("%s: %s", errstr(geterrno()), "cannot dup");
			}
			continue;
		}
		for (mp = openm; mp->type != 0; mp++)
			if (mp->type == ntype(list))
				break;
		if (mp->type == 0) {
			ret = 0;
			berror("%s", eiounimpl);
			continue;
		}
		ep = expand_string(list);
#ifdef	DEBUG
		fprintf(stderr, "expanded: ", ep->tn_left.tn_str);
		printtree(stderr, ep);
		fprintf(stderr, "%s", nl);
		fflush(stderr);
#endif
		if (ep->tn_right.tn_node != (Tnode *) NULL) {
			ret = 0;
			printio(stderr, list);
			berror(": %s", eambiguous);
		} else if ((fp = openio(ep, mp->omode, mp->type)) == (FILE *) NULL) {
			ret = 0;
			berror(mp->fd == 0 && mp->type != DOCIN ?
				ecantopen : ecantcreate,
				mp->type == DOCIN?
				tmpname:
				ep->tn_left.tn_str, errstr(geterrno()));
		} else {
			new[mp->fd] = fp;
		}
		freetree(ep);
	}
	return (ret);
}


LOCAL void
closeio(o, n)
	register FILE	*o[];
	register FILE	*n[];
{
	register int	i;

	for (i = 0; i < 3; i++) {
		if (n[i] && o[i] != n[i])
			fclose(n[i]);
	}
}

LOCAL FILE *
openio(np, omode, type)
	Tnode	*np;
	char	*omode;
	int	type;
{
	extern	int	delim;
	register FILE	*f;
	register char	*str = np->tn_left.tn_str;
		char	buf[32];
	register char	*line;
		int	save = delim;

	if (type != DOCIN) {
		f = fileopen(str, omode);
		return (f);
	}
	sprintf(buf, "%s-%ld", tmpname, (long)getpid());
	if ((f = fileopen(buf, omode)) != NULL) {
		file_raise(f, FALSE);
		unlink(buf);
		for (;;) {
			line = nextline();
			if (streql(line, str)) {
				free(line);
				break;
			}
			if (delim == EOF) {
				berror(emissterm, str);
				sclearerr();
				delim = save;
				free(line);
				break;
			}
			filewrite(f, line, strlen(line));
			fputc('\n', f);
			free(line);
		}
		fflush(f);
		fileseek(f, (off_t)0);
	}
	return (f);
}

#ifdef	DEBUG
#include <schily/fcntl.h>
LOCAL void
pfiles()
{
	int	i;

#ifdef	F_GETFD
	for (i = 0; i < 31; i++) {
		if (fcntl(i, F_GETFD, 0) >= 0)
			fprintf(stderr, "file; %d\n", i);
	}
	fflush(stderr);
#endif
}
#endif
