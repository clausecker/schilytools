/* @(#)sys.c	1.70 11/06/15 Copyright 1986-2011 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)sys.c	1.70 11/06/15 Copyright 1986-2011 J. Schilling";
#endif
/*
 *	Copyright (c) 1986-2011 J. Schilling
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
#include <schily/string.h>			/* Die system strings für strsignal()*/
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/wait.h>
#include <schily/fcntl.h>
#include <schily/time.h>
#define	VMS_VFORK_OK
#include <schily/vfork.h>

/*
 * Check whether fexec may be implemented...
 */
#if	defined(HAVE_DUP) && (defined(HAVE_DUP2) || defined(F_DUPFD))
/* Everything is OK */
#else
Error fexec canno be implemented
#endif

/*#undef	HAVE_WAIT3*/
/*#undef	HAVE_SYS_PROCFS_H*/
#if	defined(HAVE_WAIT3) || defined(HAVE_SYS_PROCFS_H) /*see wait3.c*/
#	ifndef	HAVE_WAITID
	/*
	 * XXX Hack: Interix has sys/procfs.h but no waitid()
	 */
#	undef	HAVE_SYS_PROCFS_H
#	endif
#endif
#if	defined(HAVE_SYS_TIMES_H) && defined(HAVE_TIMES)
#include <schily/times.h>
#include <schily/param.h>
#endif	/* defined(HAVE_SYS_TIMES_H) && defined(HAVE_TIMES) */

#include <schily/resource.h>

/*#define	DEBUG*/

#include "bsh.h"
#include "str.h"
#include "abbrev.h"
#include "strsubs.h"			/* Die lokalen strings */
#include "ctype.h"
#include "limit.h"

#ifdef	VFORK
	char	*Vlist;
	char	*Vtmp;
	char	**Vav;
#endif

#ifdef	JOBCONTROL
extern	pid_t	pgrp;
extern	int	ttyflg;
	pid_t	lastsusp;	/* XXX Temporary !!! */
#endif

extern	pid_t	mypid;
extern	int	ex_status;
extern	int	do_status;
extern	int	firstsh;

EXPORT	void	start		__PR((Argvec * vp, FILE ** std));
EXPORT	pid_t	shfork		__PR((int flag));
EXPORT	void	pset		__PR((pid_t child, int flag));
EXPORT	void	block_sigs	__PR((void));
EXPORT	void	unblock_sigs	__PR((void));
LOCAL	void	psigsetup	__PR((int flag));
LOCAL	void	csigsetup	__PR((int flag));
LOCAL	void	ppgrpsetup	__PR((pid_t child, int flag));
LOCAL	void	cpgrpsetup	__PR((pid_t child, int flag));
EXPORT	int	ewait		__PR((pid_t child, int flag));
EXPORT	int	fexec		__PR((char **path, char *name, FILE *in, FILE *out, FILE *err, char **av, char **env));
LOCAL	void	fdmove		__PR((int fd1, int fd2));
LOCAL	BOOL	is_binary	__PR((char *name));
EXPORT	char	*_findinpath	__PR((char *name, int mode, BOOL plain_file));

EXPORT void
start(vp, std)
	Argvec	*vp;
	FILE	*std[];
{
	register int	i = 0;
		char	*path = NULL;

	/*
	 * Allow to stop the command before execution.
	 * This may happen if the user hists ^C while the command
	 * is parsed or the arguments are expanded.
	 */
	if (ctlc)
		_exit(-1);
#ifdef DEBUG
	fprintf(stderr, "STARTING child=%ld %s with %d %d %d:",
		(long)getpid(),
		vp->av_av[0], fdown(std[0]), fdown(std[1]), fdown(std[2]));
	for (i = 1; i < vp->av_ac; i++)
		fprintf(stderr, " %s", vp->av_av[i]);
	i = 0;		/* Siehe pfexec */
	fputc('\n', stderr);
	fflush(stderr);
#endif
	update_cwd();	/* Er koennte sie veraendert haben */
#ifdef	EXECATTR_FILENAME
	if (pfshell) {
		i = pfexec(&path, vp->av_av[0], std[0], std[1], std[2], vp->av_av, evarray);
		if (i != 0)
			seterrno(i);
	}
	if (i == 0)	/* No pfexec() attrs for this command, retry exec() */
#endif
	i = fexec(&path, vp->av_av[0], std[0], std[1], std[2], vp->av_av, evarray);
	/*
	 * If we come here, this may be caused by the following reasons:
	 *
	 * ENOENT	File not found
	 * E2BIG	Too many args
	 * ENOMEM	Not enough memory for execution
	 * EACCES	No permission
	 * ENOEXEC	Invalid magic number (wrong format or shell script)
	 * Other problems are not of direct interest for us.
	 */
	if (i == ENOEXEC) {
		if (is_binary(path)) {
			berror("Cannot execute binary file.");
		} else {
			vac = vp->av_ac; vav = vp->av_av;	/* set args	*/
			i = firstsh;			/* save firstsh	*/
			firstsh = FALSE;		/* for vfork	*/
			dofile(path, GLOBAL_AB, 0, std, TRUE);
			free(path);
			firstsh = i;			/* restore firstsh*/
			if ((i = do_status) == 0)
				_exit(0);
		}
	}
	berror(ecantexecute, vp->av_av[0], errstr(i));
	if (path)
		free(path);
	_exit(i);
}


EXPORT pid_t
shfork(flag)
	int	flag;
{
	int	i = 0;
	pid_t	child;

	for (;;) {
		child = fork();
		if (child >= 0) {
			break;
		} else {
			i++;
			berror(ecantfork, errstr(geterrno())); /* XXX berror sezt evt. errno */
			if (i > 10 || ctlc)
				return (child);
			else
				sleep(1);
		}
	}
	pset(child, flag);
	return (child);
}

/*
 * Set up Parent/Child process signals / process groups
 */
#ifdef	PROTOTYPES
EXPORT void
pset(pid_t child, int flag)
#else
EXPORT void
pset(child, flag)
	pid_t	child;
	int	flag;
#endif
{
	if (child != 0) {			/* Papa */

		psigsetup(flag);		/* setup sigs */
		ppgrpsetup(child, flag);	/* setup process groups */

		if (flag & PRPID)
			berror("%ld", (long)child);

	} else {				/* Sohn */

		cpgrpsetup(child, flag);	/* setup process groups */
		csigsetup(flag);		/* setup sigs */
/*		firstsh = FALSE;*/		/* vfork doesn't like this */
	}
}

EXPORT void
block_sigs()
{
#ifdef	SVR4
	sighold(SIGINT);
#endif
}

EXPORT void
unblock_sigs()
{
#ifdef	SVR4
	sigrelse(SIGINT);
#endif
}

/*
 * Parent signal setup
 */
LOCAL void
psigsetup(flag)
	int	flag;
{
	extern	int	qflg;

	/*
	 * Bei Sun werden alle Signale im Kind ueberschrieben
	 */

#ifdef	VFORK
	signal(SIGINT, (sigtype) SIG_IGN);
	if (osig2 != (sigtype) SIG_IGN || firstsh)
		signal(SIGINT, intr);

	signal(SIGQUIT, (sigtype) SIG_IGN);
	if (qflg)
		signal(SIGQUIT, (sigtype) SIG_DFL);

	signal(SIGTERM, (sigtype) SIG_IGN);
#ifdef	SIGTSTP
	signal(SIGTSTP, (sigtype) SIG_IGN);
	signal(SIGTTIN, (sigtype) SIG_IGN);
	signal(SIGTTOU, (sigtype) SIG_IGN);
#endif
#endif	/* VFORK */
}

/*
 * Child signal setup
 */
LOCAL void
csigsetup(flag)
	int	flag;
{
	sigtype	sig2;
	sigtype	sig3;

	if (flag & NOSIG) {
		sig2 = (sigtype) SIG_IGN;
		sig3 = (sigtype) SIG_IGN;
	} else {
		sig2 = osig2;
		sig3 = osig3;
	}
	signal(SIGINT, sig2);
	signal(SIGQUIT, sig3);
	signal(SIGTERM, osig15);
#ifdef	SIGTSTP
	signal(SIGTSTP, osig18);
	signal(SIGTTIN, osig21);
	signal(SIGTTOU, osig22);
#endif
}

/*
 * Parent process group setup
 */
#ifdef	PROTOTYPES
LOCAL void
ppgrpsetup(pid_t child, int flag)
#else
LOCAL void
ppgrpsetup(child, flag)
	pid_t	child;
	int	flag;
#endif
{
#ifdef	JOBCONTROL
	if (flag & NOPGRP)
		return;

/*#define	TTEST*/
#ifdef TTEST
if (!(flag & BGRND))
	if (tty_setpgrp(0, pgrp) < 0)
		berror("TIOCSPGRP(%ld): %s\n", (long)child, errstr(geterrno()));
#endif	/* TEST */

/*	if (pgrp == mypid) berror("mypid: %ld getpid(): %ld child: %ld", (long)mypid, (long)getpid(), (long)child);*/

	if (pgrp == mypid)
/*	if (pgrp == 0)*/
		pgrp = child;
#endif	/* JOBCONTROL */
}

/*
 * Child process group setup
 */
#ifdef	PROTOTYPES
LOCAL void
cpgrpsetup(pid_t child, int flag)
#else
LOCAL void
cpgrpsetup(child, flag)
	pid_t	child;
	int	flag;
#endif
{
	int	i;
#ifdef	HAVE_USLEEP
#define	MAX_SPGID_RETRY	30
#else
#define	MAX_SPGID_RETRY	1000
#endif
#ifdef	JOBCONTROL
	if (flag & NOPGRP)
		return;
	if (ttyflg) {
/*	if (pgrp == mypid) berror("mypid: %ld getpid(): %ld child: %ld", (long)mypid, (long)getpid(), (long)child);*/

		if (pgrp == mypid) {
/*		if (pgrp == 0) {*/
			pgrp = getpid();
		}
#ifdef	POSIXJOBS
		/*
		 * Wenn man mit POSIXJOBS die Terminalprozessgruppe
		 * auf einen Wert, der nicht der Prozessgruppe des
		 * aktuellen Prozesses entspricht stellen will
		 * bekommt mann EPERM, daher setpgid *vorher*
		 */

		/*
		 * Der bsh ist der Vater aller Prozesse einer Pipe und daher
		 * kann es passieren, daß der Prozessgroupleader noch nicht
		 * losgelaufen ist und selbst ein setpgid(0, pgrp) gemacht hat
		 * während schon der 2. Pipeprozess das gleiche versucht und
		 * scheitert weil die betreffende Prozesgruppe noch nicht
		 * existiert. Wir warten hier (geben damit die CPU ab und dem
		 * Prozessgroupleader die Chance setpgid(0, pgrp) zu rufen) und
		 * versuchen es wieder. Bei ausschließlicher Verwendung von
		 * vfork() würde dieses Problem nicht auftreten weil immer
		 * das Kind zuerst losläuft.
		 */
		for (i = 0; i < MAX_SPGID_RETRY && setpgid(0, pgrp) < 0; i++) {
#ifdef	HAVE_USLEEP
			usleep(1);
#endif
		}
#endif	/* POSIXJOBS */

		if (!(flag & BGRND))
			tty_setpgrp(0, pgrp);

#ifndef	POSIXJOBS
		/*
		 * Ich kann leider nicht nachprüfen, ob es auch
		 * unter BSD 4.2 ff. korrekt geht, wenn setpgid
		 * voerher gestellt wird, daher...
		 */
		for (i = 0; i < MAX_SPGID_RETRY && setpgid(0, pgrp) < 0; i++) {
#ifdef	HAVE_USLEEP
			usleep(1);
#endif
		}
#endif	/* !POSIXJOBS */
	}
#endif	/* JOBCONTROL */
}

#ifdef	PROTOTYPES
EXPORT int
ewait(pid_t child, int flag)
#else
EXPORT int
ewait(child, flag)
	pid_t	child;
	int	flag;
#endif
{
	pid_t died;
#if defined(i386) || defined(__x86) || defined(vax)
	struct {
		char 	type;
		char 	exit;
		unsigned filler:16;	/* Joerg */
	} status;
#else
	struct {
		unsigned filler:16;	/* Joerg */
		char 	exit;
		char 	type;
	} status;
#endif
	struct rusage prusage;
#if	!defined(HAVE_WAIT3) && !defined(HAVE_SYS_PROCFS_H) /*see wait3.c*/
#if	defined(HAVE_SYS_TIMES_H) && defined(HAVE_TIMES)
	struct tms	tms1;
	struct tms	tms2;
#endif
#endif
	int	stype = 0;

	const char *mesg;

	fillbytes(&prusage, sizeof (struct rusage), 0);

#ifdef DEBUG
	printf("ewait: child=%ld, flag=%x\n", (long)child, flag);
	seterrno(0);
#endif
	do {
#	if	defined(HAVE_WAIT3) || defined(HAVE_SYS_PROCFS_H) /*see wait3.c*/

		/* Brain damaged AIX requires loop */
		do {
#ifdef	__hpux
/*			died = wait3((WAIT_T *)&status, WUNTRACED, 0);*/
			died = wait3((WAIT_T *)&status, WUNTRACED, &prusage);
#else
			died = wait3((WAIT_T *)&status, WUNTRACED, &prusage);
#endif
		} while (died < 0 && geterrno() == EINTR);

#ifndef	WSTOPFLG
	/*
	 * SVR4 hat WSTOPPED als Alias für WUNTRACED
	 * WSTOPFLG entspricht dem Wert im wait Status.
	 */
#define	WSTOPFLG	WSTOPPED
#endif
		if (status.type == WSTOPFLG) {
/* BSD Korrekt!	if (status.type == WSTOPPED) {*/
#ifdef DEBUG
printf("ewait: back from child (WSTOPPED).\n");
#endif
#ifdef	JOBCONTROL
			lastsusp = died;	/* XXX Temporary !!! */
#endif
			stype = status.type;
			status.type = status.exit;
			status.exit = stype;
		}
#	else	/* defined(HAVE_WAIT3) || defined(HAVE_SYS_PROCFS_H) */
		times(&tms1);
		do {
			died = wait(&status);
		} while (died < 0 && geterrno() == EINTR);
#if	defined(HAVE_SYS_TIMES_H) && defined(HAVE_TIMES)
		if (times(&tms2) != -1) {
/*#define	TIMES_DEBUG*/
#ifdef	TIMES_DEBUG
			printf("CLK_TCK: %d\n", CLK_TCK);
			printf("tms_utime %ld\n", (long) (tms2.tms_cutime - tms1.tms_cutime));
			printf("tms_stime %ld\n", (long) (tms2.tms_cstime - tms1.tms_cstime));
#endif
			/*
			 * Hack for Haiku, where the times may sometime be negative
			 */
			if (tms2.tms_cutime < tms1.tms_cutime)
				tms2.tms_cutime = tms1.tms_cutime;
			if (tms2.tms_cstime < tms1.tms_cstime)
				tms2.tms_cstime = tms1.tms_cstime;
			prusage.ru_utime.tv_sec  = (tms2.tms_cutime - tms1.tms_cutime) / CLK_TCK;
			prusage.ru_utime.tv_usec = ((tms2.tms_cutime - tms1.tms_cutime) % CLK_TCK) * (1000000/CLK_TCK);
			prusage.ru_stime.tv_sec  = (tms2.tms_cstime - tms1.tms_cstime) / CLK_TCK;
			prusage.ru_stime.tv_usec = ((tms2.tms_cstime - tms1.tms_cstime) % CLK_TCK) * (1000000/CLK_TCK);
		}
#endif
#	endif	/* ! defined(HAVE_WAIT3) || defined(HAVE_SYS_PROCFS_H) */
#if defined(__BEOS__) || defined(__HAIKU__)	/* Dirty Hack for BeOS, we should better use the W* macros */
		{	int i = status.exit;
			status.exit = status.type;
			status.type = i;
		}
#endif
#ifdef DEBUG
printf("ewait: back from child.\n");
printf("       died       = %lx (%ld) errno= %d\n", (long)died, (long)died, geterrno());
printf("       wait       = %4.4x\n", *(int *)&status);
printf("       exit_state = %2.2x\n", status.exit);
printf("       return     = %2.2x\n", status.type);
#endif
		ex_status = status.exit;
		if (!ex_status && status.type)		/* Joerg */
			ex_status = status.type & 0177;

		if (died <= 0) {
			status.type = geterrno();
			berror("%s", enochildren);
			break;
		} else {
#ifdef	JOBCONTROL
			if (status.type == SIGINT)
				ctlc++;
#endif
			mesg = strsignal(status.type & 0177);
			if (mesg != NULL && (status.type & 0177) != 0) {
				fprintf(stderr, "\r\n");
				if (child != died || stype == WSTOPFLG)
/* BSD Korrekt!			if (child != died || stype == WSTOPPED)*/
					fprintf(stderr, "%ld: ", (long)died);

				fprintf(stderr, "%s%s\n",
					mesg,
					status.type&0200?ecore:nullstr);
			}
		}
		if ((flag & NOTMS) == 0 && getcurenv("TIME"))
			prtimes(gstd, &prusage);
	} while (child != died && child != 0 && (flag & WALL) != 0);
#ifdef DEBUG
printf("ewait: returning %x (%d)\n", status.type, status.type);
#endif
	esigs();
	return (status.type);
}

#define	sys_exec(n, in, out, err, av, ev) (execve(n, av, ev), geterrno())
#define	enofile(t)			((t) == ENOENT || \
					(t)  == ENOTDIR || \
					(t)  == EISDIR || \
					(t)  == EIO)

#ifdef	F_GETFD
#define	fd_getfd(fd)		fcntl((fd), F_GETFD, 0)
#else
#define	fd_getfd(fd)
#endif
#ifdef	F_SETFD
#define	fd_setfd(fd, val)	fcntl((fd), F_SETFD, (val));
#else
#define	fd_setfd(fd, val)
#endif

EXPORT int
fexec(path, name, in, out, err, av, env)
		char	**path;
	register char	*name;
		FILE	*in;
		FILE	*out;
		FILE	*err;
		char	*av[];
		char	*env[];
{
	register char	*pathlist;
	register char	*tmp;
	register char	*p1;
	register char	*p2;
	register int	t = 0;
	register int	exerr;
	char	*av0 = av[0];
	int	din;
	int	dout;
	int	derr;
#ifndef	set_child_standard_fds
	int	o[3];		/* Old fd's for stdinin/stdout/stderr */
	int	f[3];		/* Old close on exec flags for above  */

	o[0] = o[1] = o[2] = -1;
	f[0] = f[1] = f[2] = 0;
#endif

	exerr = 0;
	fflush(out); fflush(err);
	din  = fdown(in);
	dout = fdown(out);
	derr = fdown(err);

#ifdef	set_child_standard_fds
	set_child_standard_fds(din, dout, derr);
#else
	if (din != STDIN_FILENO) {
		f[0] = fd_getfd(STDIN_FILENO);
		o[0] = dup(STDIN_FILENO);
		fd_setfd(o[0], 1);
		fdmove(din, STDIN_FILENO);
	}
	if (dout != STDOUT_FILENO) {
		f[1] = fd_getfd(STDOUT_FILENO);
		o[1] = dup(STDOUT_FILENO);
		fd_setfd(o[1], 1);
		fdmove(dout, STDOUT_FILENO);
	}
	if (derr != STDERR_FILENO) {
		f[2] = fd_getfd(STDERR_FILENO);
		o[2] = dup(STDERR_FILENO);
		fd_setfd(o[2], 1);
		fdmove(derr, STDERR_FILENO);
	}
#endif

	/* if has slash try exec and set error code */

	if (strchr(name, '/')) {
		tmp = makestr(name);
#ifdef	VFORK
		Vtmp = tmp;
#endif
		exerr = sys_exec(tmp, din, dout, derr, av, env);
		av[0] = av0;	/* BeOS destroys things ... */
	} else {
					/* "PATH" */
		if ((pathlist = getcurenv(pathname)) == NULL)
			pathlist = defpath;
		p2 = pathlist = makestr(pathlist);
#ifdef	VFORK
		Vlist = pathlist;
#endif
		for (;;) {
			p1 = p2;
			if ((p2 = strchr(p2, ':')) != NULL)
				*p2++ = '\0';
			if (*p1 == '\0')
				tmp = makestr(name);
			else
				tmp = concat(p1, slash, name, (char *)NULL);
#ifdef	VFORK
			Vtmp = tmp;
#endif
			t = sys_exec(tmp, din, dout, derr, av, env);
			av[0] = av0;	/* BeOS destroys things ... */

			if (exerr == 0 && !enofile(t))
				exerr = t;
			if ((!enofile(t) && !(t == EACCES)) || p2 == NULL)
				break;
			free(tmp);
		}
		free(pathlist);
#ifdef	VFORK
		Vlist = 0;
#endif
	}

#ifndef	set_child_standard_fds
	if (derr != STDERR_FILENO) {
		fdmove(STDERR_FILENO, derr);
		fdmove(o[2], STDERR_FILENO);
		if (f[2] == 0)
			fd_setfd(STDERR_FILENO, 0);
	}
	if (dout != STDOUT_FILENO) {
		fdmove(STDOUT_FILENO, dout);
		fdmove(o[1], STDOUT_FILENO);
		if (f[1] == 0)
			fd_setfd(STDOUT_FILENO, 0);
	}
	if (din != STDIN_FILENO) {
		fdmove(STDIN_FILENO, din);
		fdmove(o[0], STDIN_FILENO);
		if (f[0] == 0)
			fd_setfd(STDIN_FILENO, 0);
	}
	if (exerr == 0)
		exerr = t;
	seterrno(exerr);
#endif

#ifdef	VFORK
	Vtmp = 0;
	Vlist = 0;
#endif
	if (path)
		*path = tmp;
	else
		free(tmp);
	return (exerr);
}

#ifndef	set_child_standard_fds

LOCAL void
fdmove(fd1, fd2)
	int	fd1;
	int	fd2;
{
	close(fd2);
#ifdef	F_DUPFD
	fcntl(fd1, F_DUPFD, fd2);
#else
#ifdef	HAVE_DUP2
	dup2(fd1, fd2);
#endif
#endif
	close(fd1);
}

#endif

#if	defined(HAVE_ELF)
#	include <elf.h>
#else
# if	defined(HAVE_AOUT)
#	include	<a.out.h>
# endif
#endif

#ifndef	NMAGIC	/*XXX Elf & Coff ???*/

LOCAL BOOL
is_binary(name)
	char	*name;
{
	int		f = open(name, 0);
	unsigned char	c = ' ';

	if (f < 0 || read(f, &c, 1) != 1) {
		close(f);
		return (TRUE);
	}
	return (!isprint(c) && !isspace(c));
}

#else

LOCAL BOOL
is_binary(name)
	char	*name;
{
	int	f = open(name, 0);
	struct	exec x;
	unsigned char	c;

	fillbytes(&x, sizeof (x), '\0');

	if (f < 0 || read(f, &x, sizeof (x)) <= 1) {
		close(f);
		return (TRUE);
	}
	c = *((char *)&x);

	return (!N_BADMAG(x) || (!isprint(c) && !isspace(c)));
}
#endif

#include <schily/stat.h>

/*
 * Uour local version of findinpath() that is using getcurenv()
 * instead of getenv().
 */
EXPORT char *
_findinpath(name, mode, plain_file)
	char	*name;
	int	mode;
	BOOL	plain_file;
{
	char	*pathlist;
	char	*p1;
	char	*p2;
	char	*tmp;
	int	err = 0;
	int	exerr = 0;
	struct stat sb;

	if (name == NULL)
		return (NULL);
	if (strchr(name, '/'))
		return (makestr(name));

	if ((pathlist = getcurenv(pathname)) == NULL)
		pathlist = defpath;
	p2 = pathlist = makestr(pathlist);

	for (;;) {
		p1 = p2;
		if ((p2 = strchr(p2, ':')) != NULL)
			*p2++ = '\0';
		if (*p1 == '\0')
			tmp = makestr(name);
		else
			tmp = concat(p1, slash, name, (char *)NULL);

		seterrno(0);
		if (stat(tmp, &sb) >= 0) {
			if ((plain_file || S_ISREG(sb.st_mode)) &&
				(eaccess(tmp, mode) >= 0)) {
				free(pathlist);
				return (tmp);
			}
			if ((err = geterrno()) == 0)
				err = ENOEXEC;
		} else {
			err = geterrno();
		}
		free(tmp);
		if (exerr == 0 && !enofile(err))
			exerr = err;
		if ((!enofile(err) && !(err == EACCES)) || p2 == NULL)
			break;
	}
	free(pathlist);
	seterrno(exerr);
	return (NULL);
}
