/* @(#)bsh.c	1.65 11/07/10 Copyright 1984,1985,1988,1989,1991,1994-2011 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)bsh.c	1.65 11/07/10 Copyright 1982,1984,1985,1988,1989,1991,1994-2011 J. Schilling";
#endif
/*
 *	bsh command interpreter - main Program
 *
 *	Copyright (c) 1982,1984,1985,1988,1989,1991,1994-2011 J. Schilling
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
#include <schily/ctype.h>
#include <schily/signal.h>
#include <schily/setjmp.h>
#include <schily/sigblk.h>
#include <schily/pwd.h>
#include "bsh.h"
#include "node.h"
#include "abbrev.h"
#include "str.h"
#include "strsubs.h"
#include <schily/varargs.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/fcntl.h>
#include <schily/locale.h>

#ifdef	SIGUSR1
#	define	PROTSIG	SIGUSR1
#else
#	define	PROTSIG	31
#endif

char *prompts[2]	= { NULL, NULL};

/*
 * Some saved old signal values...
 */
sigtype osig2		= (sigtype) SIG_DFL;
sigtype osig3		= (sigtype) SIG_DFL;
sigtype osig15		= (sigtype) SIG_DFL;
sigtype osig18		= (sigtype) SIG_DFL;
sigtype osig21		= (sigtype) SIG_DFL;
sigtype osig22		= (sigtype) SIG_DFL;

int	batch		= 0;
int	verbose		= FALSE;
int	cflg		= FALSE;
int	eflg		= FALSE;
int	vflg		= FALSE;
int	iflg		= FALSE;
int	nflg		= FALSE;
int	sflg		= FALSE;
int	tflg		= FALSE;
int	no_closeflg	= FALSE;
int	no_histflg	= FALSE;
int	mailcheck	= 600;		/* Mail check interval		    */
int	qflg		= FALSE;
int	pfshell		= FALSE;

int	prflg		= FALSE;
int	ttyflg		= FALSE;

int	parseflg	= FALSE;
int	ctlc		= 0;
int	ex_status	= 0;
int	do_status	= 0;		/* for recognition of readaccess error */
pid_t	mypid		= 0;
pid_t	opgrp		= 0;
pid_t	mypgrp		= 0;
Tnode	*lastcmd	= (Tnode *)NULL; /* Used by ancient #e command */
BOOL	noslash		= FALSE;	/* Used for restrictions */
char	*user		= NULL;		/* Used for ~ Expansion  */
char	*hostname	= NULL;
char	*cmdfname	= NULL;		/* Used for #! commands */
LOCAL	int sig_err_cnt	= 0;		/* abort on 10.th error */

#ifdef	INTERACTIVE
int	prompt		= 0;
#else
Tnode	**cur_base	= 0;		/* for use by history */
int	history		= 0;
#endif

int	vac		= 0;
/*char	* const *vav	= (char * const *)NULL;*/
char	**vav		= (char **)NULL;
FILE	*cmdfp		= (FILE *) NULL;
FILE	*gstd[3];
char	**evarray	= (char **) NULL;
unsigned evasize	= 0;
int	evaent		= 0;
char	*initav0	= NULL;

BOOL	firstsh = FALSE;
char	*inithome = NULL;
FILE	*protfile = (FILE *)NULL;	/* output file for consprot */

LOCAL	int	prompterrs	= 0;	/* no errors on stderr jet */

LOCAL	jmp_buf	jmpblk;
LOCAL	SIGBLK	sb;

extern	abidx_t	deftab;			/* Default tab for Abbrev/Alias */

extern	int	delim;
extern	int	nerrors;

EXPORT	sigret	intr		__PR((int sig));
LOCAL	BOOL	clearferr	__PR((void));
LOCAL	int	sigjmp		__PR((const char *signalstr, long j, long arg));
LOCAL	void	printsig	__PR((const char *sig, const char *arg));
LOCAL	sigret	proton_off	__PR((int sig));
EXPORT	int	main		__PR((int ac, char **av, char **ev));
EXPORT	BOOL	dofile		__PR((char *s, abidx_t tab, int flag, FILE ** std, BOOL jump));
EXPORT	void	doopen		__PR((FILE * fd, char *s, abidx_t tab, int flag, FILE ** std, BOOL jump));
EXPORT	void	process		__PR((FILE * f, int flag, FILE ** std, BOOL jump));
EXPORT	int	berror		__PR((const char *s, ...));
EXPORT	char	*errstr		__PR((int err));
EXPORT	void	close_other_files	__PR((FILE ** std));
LOCAL	char	*getgfile	__PR((void));
EXPORT	char	*getuname	__PR((int uid));
EXPORT	char	*getpwdir	__PR((char *name));
EXPORT	char	*mypwhome	__PR((void));
EXPORT	char	*myhome		__PR((void));
LOCAL	void	gargs		__PR((int ac, char *const* av, char *opts, int *no_i2flg, int *no_gaflg, int *no_laflg));
EXPORT	void	exitbsh		__PR((int excode));
LOCAL	void	bshusage	__PR((int flag, char *name, char *s));

/* ARGSUSED */
EXPORT sigret
intr(sig)
	int	sig;
{
	extern int sigcount[];

	signal(SIGINT, intr);
	ctlc++;
	sigcount[SIGINT]++;
#ifdef	DEBUG
	fprintf(stderr,
		"ctlc: pid: %ld sigcount[%d] %d parseflg: %d jmpblk.ret: 0x%x\n",
			(long)mypid, SIGINT, sigcount[SIGINT], parseflg, jmpblk[0]);
	fflush(stderr);
#endif
/*
 * XXXY Vielleicht doch wieder lonjmp solange keine ueberall funktionierende
 * XXXY raisecond() Implementierung vorliegt.
 * XXXY raisecond geht nur wenn wir USE_SCANSTACK bei der Kompilation von
 * XXXY raisecond.c definieren.
 */
	if (parseflg && !cflg) {		/* Kein Jump, wenn bsh -c '' */
		raisecond(sn_ctlc, (long)NULL);	/* raise the condition */
	}
}

#	ifndef	_NFILE
#		define	_MAXFILES	20	/* XXX nicht ok */
#	else
#		define	_MAXFILES	_NFILE
#	endif
LOCAL BOOL
clearferr()
{
	register int	i;

	/*
	 * Wenn ein Pseudotty geschloszen wird (quit bei shelltool)
	 * gibt es fortgesetzte write-errors und der bsh beginnt zu
	 * onanieren, wenn auch hier ferror geloescht wird.
	 */
	if (ferror(stderr)) {
		if (++prompterrs >= 10) {
			exitbsh(ex_status);
			/* NOTREACHED */
		}
	}

	for (i = 0; i < 3; i++) {
		if (ferror(gstd[i])) {
			clearerr(gstd[i]);
			return (TRUE);
		}
	}
	return (FALSE);
}

/*
 * XXXY long j kann erst geaendert werden, wenn raisecond() ein void *
 * XXXY Argument hat.
 */
LOCAL int
sigjmp(signalstr, j, arg)
	const char	*signalstr;
	long	j;		/* (jmp_buf)j: stack frame information */
	long	arg;
{
	jmp_buf	*jp = (jmp_buf *)j;

	printsig(signalstr, (char *)arg);

	if (!(strindex("file", (char *)signalstr) && clearferr()) &&
	    !streql(signalstr, sn_ctlc) && (++sig_err_cnt >= 10)) {
#ifdef	INTERACTIVE
		reset_tty_modes();
		reset_line_disc();
		reset_tty_pgrp();
#endif
		abort();
	}
	longjmp(*jp, TRUE);
	return (FALSE);
}

LOCAL void
printsig(sig, arg)
	const char	*sig;
	const char	*arg;
{
	char	str[80];

	sprintf(str, "Caught %.20s Signal", sig);
	write(STDERR_FILENO, str, strlen(str));
	if (arg) {
		sprintf(str, " from '%.20s'", arg);
		write(STDERR_FILENO, str, strlen(str));
	}
	write(STDERR_FILENO, ".\r\n", 3);
}

/* ARGSUSED */
LOCAL sigret
proton_off(sig)
	int	sig;
{
	char	protfname[25];

	signal(PROTSIG, proton_off);
	if (protfile) {
		fclose(protfile);
		protfile = (FILE *) NULL;
	} else {
		sprintf(protfname, "%s.%ld", tmpname, (long)mypid);
		protfile = fileopen(protfname, for_wca);
#ifdef	F_SETFD
		fcntl(fdown(protfile), F_SETFD, FD_CLOEXEC);
#endif
	}
}

EXPORT int
main(ac, av, ev)
	int ac;
	char *av[];
	char *ev[];
{
	char	*gabbrevs;
	char	*initfname;
	int	no_i2flg	= 0;
	int	no_gaflg	= 0;
	int	no_laflg	= 0;

	save_args(ac, av);
	initav0 = av[0];
	vac = ac;
	vav = av;
	firstsh = ac > 0 && av[0][0] == '-';	/* see if its a login shell */

	/*
	 * Cygwin32 makes stdin/stdout/stderr non constant expressions
	 * so we cannot do loader initialization.
	 *
	 * XXX May this be a problem?
	 */
	gstd[0]	= stdin;
	gstd[1]	= stdout;
	gstd[2]	= stderr;

	inittime();

/*error("euid: %d ruid: %d", geteuid(), getuid());*/

#ifdef	HAVE_SETEUID
	/*
	 * setreuid() ist POSIX aber alte *BSD BS haben kein saved uid.
	 * Hier sollte noch ein Test auf _POSIX_SAVED_IDS hinein.
	 */
	seteuid(getuid());	/* Ungefährlich auf Systemen ohne saved uid */
#else
#ifdef	HAVE_SETREUID		/* BSD & POSIX */
	setreuid(-1, getuid());
#else
#	ifdef	HAVE_SETRESUID
	setresuid(-1, getuid(), -1);	/* HP-UX setresuid(ruid, euid, suid)*/
#	else
	/*
	 * Hier sollte nur dann eine Warnung/Abbruch kommen, wenn
	 * der bsh tatsächlich suid root installiert ist.
	 */
#if !defined(__EMX__) && !defined(__DJGPP__) && !defined(__BEOS__)
error  No function to set uid available
#endif

#	endif
#endif
#endif	/* HAVE_SETEUID */

#if	defined(HAVE_SIGPROCMASK)
	{
		sigset_t set;

		sigemptyset(&set);	/* csh macht login falsch */
		sigprocmask(SIG_SETMASK, &set, 0);
	}
#else
#	if defined(HAVE_SIGSETMASK)
	sigsetmask(0);			/* csh macht login falsch */
#	endif
#endif
	signal(PROTSIG, proton_off);
	mypid = getpid();
	mypgrp = opgrp = getpgid(0);
#ifdef	WRONG
#ifdef	JOBCONTROL
	setpgid(0, mypid);
	mypgrp = getpgid(0);
#endif
#endif	/* WRONG */
	ev_init(ev);
	if (setlocale(LC_ALL, "") == NULL)
		error("Bad locale in inital environment.\n");
	ev_insert(concat(ignoreeofname, eql, off, (char *)NULL));
	inituser();
	inithostname();
	initprompt();
#ifdef INTERACTIVE
	init_input();
#else
	inithistory();
#endif
	osig2 = signal(SIGINT, (sigtype) SIG_IGN);
	if (osig2 != (sigtype) SIG_IGN)
		signal(SIGINT, intr);
	osig3 = signal(SIGQUIT, (sigtype) SIG_IGN);
	osig15 = signal(SIGTERM, (sigtype) SIG_IGN);
#ifdef	SIGTSTP
	osig18 = signal(SIGTSTP, (sigtype) SIG_IGN);
#endif
#ifdef	SIGTTIN
	osig21 = signal(SIGTTIN, (sigtype) SIG_IGN);
#endif
#ifdef	SIGTTOU
	osig22 = signal(SIGTTOU, (sigtype) SIG_IGN);
#endif
	if (firstsh) {
		signal(SIGINT, intr);
		osig2 = (sigtype) SIG_DFL;
		osig3 = (sigtype) SIG_DFL;
		osig15 = (sigtype) SIG_DFL;
		osig18 = (sigtype) SIG_DFL;
		osig21 = (sigtype) SIG_DFL;
		osig22 = (sigtype) SIG_DFL;
	}

#ifdef	EXECATTR_FILENAME
	if (ac > 0) {
		const char	*fn = fbasename(av[0]);

		if (streql(fn, "pfbsh") || streql(fn, "-pfbsh")) {
			pfshell = TRUE;
			pfinit();
		}
	}
#endif

	gargs(ac, av, bshopts, &no_i2flg, &no_gaflg, &no_laflg);

	if (qflg)
		(void) signal(SIGQUIT, SIG_DFL);

	if (batch) {
		vac -= ac - batch;
		vav += ac - batch;
		if (vac <= 1)
			sflg++;
	}
#ifdef	XXX
	/* XXX solange setreuid() am Anfang steht ungefaehrlich !! */
	else if (getuid() != geteuid() || getgid() != getegid()) {
		berror("%s: %s", fbasename(initav0), errstr(EACCES));
		exit(EACCES);
	}
#endif


	if (!no_closeflg)
		close_other_files(gstd);

	setinput((FILE *) NULL);		/* Create input stream */
	gabbrevs = getgfile();			/* Get .globals file name */
	inithome = concat(getcurenv(homename), (char *)NULL);

	if (firstsh) {
						/* Run /etc/initbsh  file */
		dofile(sysinitname, GLOBAL_AB, 0, gstd, TRUE);

						/* Run /etc/initrbsh file */
		if (strchr(fbasename(initav0), 'r'))
			dofile(sysrinitname, GLOBAL_AB, 0, gstd, TRUE);
	}

	if (!no_gaflg) {			/* Read in .globals */
		ab_use(GLOBAL_AB, gabbrevs);
	}

	if (!no_laflg) {			/* Read in .locals */
		ab_use(LOCAL_AB, localname);
	}

	if (firstsh) {
						/* Run .init script */
		initfname = concat(inithome, slash, initname, (char *)NULL);
		dofile(initfname, GLOBAL_AB, 0, gstd, TRUE);
	} else {
						/* Run .init2 script */
		initfname = concat(inithome, slash, init2name, (char *)NULL);
		if (!no_i2flg)
			dofile(initfname, GLOBAL_AB, NOTMS, gstd, TRUE);
	}
	free(initfname);
	if (verbose)
		vflg++;

	vav++;
	vac--;
	if (!sflg && !batch &&
			getfiles(&vac, (char * const **)&vav, bshopts) <= 0)
		sflg++;

	if (!sflg) {
		if (cflg) {
			/*
			 * -c Option: force "one line" command.
			 */
			pushline(vav[0]);
			do {
				freetree(cmdline(0, gstd, FALSE));
			} while (delim != EOF);
		} else if (!dofile(vav[0], GLOBAL_AB, 0, gstd, TRUE)) {
			berror(ecantopen, vav[0], errstr(ex_status = do_status));
		}
	} else {
		cflg = FALSE;
#ifdef	INTERACTIVE
		if (!no_histflg)
			read_init_history();
#endif
#ifdef	JOBCONTROL
		setpgid(0, mypid);
		mypgrp = getpgid(0);
#endif
		do
			process(stdin, 0, gstd, TRUE);
		while (delim != EOF && !tflg);
	}
	exitbsh(ex_status);
	return (ex_status);	/* Keep lint happy */
}

EXPORT BOOL
dofile(s, tab, flag, std, jump)
	char	*s;
	abidx_t	tab;
	int	flag;
	FILE	*std[];
	BOOL	jump;
{
	FILE	*fd;

#ifdef DEBUG
	fprintf(stderr, "dofile(%s,%d,..%sjump) ", s, tab, jump?nullstr:"no");
#endif
	if ((fd = fileopen(s, for_read)) != (FILE *) NULL) {
#ifdef DEBUG
		fprintf(stderr, "(ok)\n");
		fflush(stderr);
#endif
#ifdef	F_SETFD
		fcntl(fdown(fd), F_SETFD, FD_CLOEXEC);
#endif
		doopen(fd, s, tab, flag, std, jump);
		fclose(fd);
		return (TRUE);
	} else {
		do_status = geterrno();
#ifdef DEBUG
		fprintf(stderr, "(error) %s\n", errstr(do_status));
		fflush(stderr);
#endif
		return (FALSE);
	}
}

EXPORT void
doopen(fd, s, tab, flag, std, jump)
	FILE	*fd;
	char	*s;
	abidx_t	tab;
	int	flag;
	FILE	*std[];
	BOOL	jump;
{
	abidx_t	savetab;	/* abbrev tab */
	int	prsave;		/* Promptflag */
	int	ttysave;	/* ttyflag */
	int	ssave;		/* sflag */

#ifdef DEBUG
	fprintf(stderr, "doopen(%d,%s,%d,..%sjump) ",
			fdown(fd), s, tab, jump?nullstr:"no");
#endif
	do_status = 0;
	cmdfname = s;
	cmdfp = fd;
	savetab = deftab;
	deftab = tab;
	prsave = prflg;
	ttysave = ttyflg;
	ssave = sflg;
	sflg = FALSE;
	process(fd, flag, std, jump);
	cmdfp = (FILE *)NULL;
	cmdfname = NULL;
	deftab = savetab;
	prflg = prsave;		/* process setzt prflg  um */
	ttyflg = ttysave;	/* process setzt ttyflg um */
	sflg = ssave;
}

EXPORT void
process(f, flag, std, jump)
	FILE	*f;
	int	flag;
	FILE	*std[];
	BOOL	jump;		/* Damit dofile und process auch von hoeherer*/
				/* Ebene aufgerufen werden kann		    */
{
	FILE	*old;
	int	save = delim;
	Tnode	*cmd;
	SIGBLK	sigfirst;
#ifndef	INTERACTIVE
	int	i, found, max;
#endif

	ttyflg = isatty(fdown(f));
	prflg = ttyflg || iflg;

	if (ttyflg) {
		setbuf(std[0], NULL);	/* XXX ist das die richtige Stelle ? */
		osig18 = (sigtype) SIG_DFL;
		osig21 = (sigtype) SIG_DFL;
		osig22 = (sigtype) SIG_DFL;
	}
	old = setinput(f);
	starthandlecond(&sigfirst);
	if (jump) {
		if (setjmp(jmpblk)) {
			eatline();
			if (!prflg || delim == EOF) {
				setinput(old);
				unhandlecond(&sigfirst);
				delim = save;
				return;
			}
		} else {
			handlecond(sn_any_other, &sb, sigjmp, (long)jmpblk);
		}
	}
	do {
		ctlc = 0;
		if (prflg)
			testmail();
#ifdef INTERACTIVE
		prompt = 0;
#else
		if (prflg) {
#ifdef	JOBCONTROL
			tty_setpgrp(fdown(f), mypgrp);
#endif
			fprintf(stderr, prompts[0]);
			prompterrs = 0;		/* diesmal kein write-error */
		}
#endif
		cmd = cmdline(flag, std, FALSE); /* Parse / execute next line */
#ifndef INTERACTIVE
		if (cmd && history > 0) {
			max = high_hist();
			found = -1;
			for (i = max; i >= 0 && found == -1; i--)
				if (treeequal(cmd, cur_base[i]))
					found = i;
			if (found == -1)
				h_append(cmd);
			else
				lr_used(found);
			/* no freetree(lastcmd) h_append() free's it */
			lastcmd = cmd;
		}
		if (cmd && history == 0) {
			freetree(lastcmd);
			lastcmd = cmd;
		}
#else
		if (cmd) {
			freetree(lastcmd);
			lastcmd = cmd;
		}
#endif
	} while (delim != EOF &&
			((nerrors == 0 && !ctlc) || prflg) &&
						!(ttyflg && tflg));
	setinput(old);
	unhandlecond(&sigfirst);
	delim = save;
}

#ifdef	PROTOTYPES
EXPORT int
berror(const char *s, ...)
#else
/* VARARGS1 */
EXPORT int
berror(s, va_alist)
	char	*s;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, s);
#else
	va_start(args);
#endif
	ret = fprintf(stderr, "%r\n", s, args);
	va_end(args);
	fflush(stderr);
	return (ret);
}

/*
 * Return system error message string for arg 'err'.
 */

#if defined(__BEOS__) || defined(__HAIKU__)
#define	silent_error(e)		((e) < 0 && (e) >= -1024)
#else
#define	silent_error(e)		((e) < 0)
#endif

EXPORT char *
errstr(err)
	int	err;
{
	static	char	errbuf[12];
	char	*estr;

	if (silent_error(err)) {
		return (nullstr);
	} else {
		estr = errmsgstr(err);
		if (estr == NULL) {
			sprintf(errbuf, "%d", err);
			estr = errbuf;
		}
		return (estr);
	}
}

EXPORT void
close_other_files(std)
	FILE	*std[3];
{
	register int s0 = fdown(std[0]);
	register int s1 = fdown(std[1]);
	register int s2 = fdown(std[2]);
	register int i;
#ifdef	_SC_OPEN_MAX
	register int max = sysconf(_SC_OPEN_MAX);
#else
#ifdef	HAVE_GETDTABLESIZE
	register int max = getdtablesize();
#else
	register int max = _MAXFILES;
#endif
#endif

	for (i = 0; i++ < max; ) {
		if (i == STDIN_FILENO || i == STDOUT_FILENO ||
						i == STDERR_FILENO) {
			continue;
		}
		if (i != s0 && i != s1 && i != s2)
			close(i);
	}
}

LOCAL char *
getgfile()
{
	char	*gname;
	char	*hdir;
	char	buf[10];
	struct passwd *pw;
/*	extern struct passwd *getpwuid();*/

	hdir = getcurenv(homename);
	if (hdir) {
		gname = concat(hdir, slash, globalname, (char *)NULL);
	} else {
		sprintf(buf, "%ld", (long)geteuid());
		/*
		 * First search for user in passwd file. If user can be found,
		 * look in his home directory for .globals file.
		 * If the user is not in the passwd file then search
		 * current directory for .globals file.
		 */
		pw = getpwuid(geteuid());
		endpwent();
		if (!pw)
			return (concat(globalname, (char *)NULL));
		gname = concat(pw->pw_dir, slash, globalname, (char *)NULL);
		ev_insert(concat(homename, eql, pw->pw_dir, (char *)NULL));
	}
	return (gname);
}

EXPORT char *
getuname(uid)
	int	uid;
{
	char	buf[12];
	register struct passwd *pw;

	pw = getpwuid(uid);
	endpwent();
	if (pw)
		return (makestr(pw->pw_name));
	sprintf(buf, "%d", uid);
	return (makestr(buf));
}

EXPORT char *
getpwdir(name)
	char *name;
{
	register struct passwd *pw;
/*	extern struct passwd *getpwnam();*/

	pw = getpwnam(name);
	endpwent();

	if (!pw)
		return (NULL);
	return (makestr(pw->pw_dir));
}

EXPORT char *
mypwhome()
{
	static char *my_pwhome = 0;

	if (!my_pwhome)
		my_pwhome = getpwdir(user);
	if (!my_pwhome) {
		my_pwhome = getcurenv(homename);
		if (my_pwhome)
			my_pwhome = makestr(my_pwhome);
	}
	if (!my_pwhome)
		return (NULL);
	return (makestr(my_pwhome));
}

EXPORT char *
myhome()
{
	char *my_home;

	my_home = getcurenv(homename);
	if (!my_home)
		return (NULL);
	return (makestr(my_home));
}

LOCAL void
gargs(ac, av, opts, no_i2flg, no_gaflg, no_laflg)
	int	ac;
	char	*const *av;
	char	*opts;
	int 	*no_i2flg, *no_gaflg, *no_laflg;
{
	BOOL	hflg = FALSE;
	BOOL	be_fast = FALSE;
	BOOL	be_xfast = FALSE;
	BOOL	prversion = FALSE;
/*	char	bshopts[]	= "v,V,i,c,e,h,2,g,l,n,s,t,f,F,o,q,help,version";*/

	av++;
	ac--;
	if (getargs(&ac, &av, opts,
			&verbose,
			&vflg,
			&iflg,
			&cflg,
			&eflg,
			&no_histflg,
			no_i2flg,
			no_gaflg,
			no_laflg,
			&nflg,
			&sflg,
			&tflg,
			&be_fast,
			&be_xfast,
			&no_closeflg,
			&qflg,		/* Undoc' d .. don't ignore SIGQUIT */
			&hflg, &prversion) < 0) {
		if (av[0][0] != '-') {	/* Be careful, cmd args may have '=' */
			batch = ac+1;
		} else {
			bshusage(TRUE, fbasename(initav0), av[0]);
		}
	}
	if (av[0] != NULL && streql(av[0], "-"))
		batch = ac;
#ifdef	ODEBUG
	error("-v%d -V%d -i%d -c%d -e%d -h%d -2%d -g%d -l%d -n%d -s%d -t%d -f%d -F%d -o%d -q%d -help%d -version%d\n",
		verbose, vflg, iflg, cflg, eflg, no_histflg,
		*no_i2flg, *no_gaflg, *no_laflg, nflg, sflg, tflg,
		be_fast, be_xfast, no_closeflg, qflg, hflg, prversion);
#endif
	if (hflg)
		bshusage(FALSE, fbasename(initav0), (char *) NULL);
	if (prversion) {
		extern	int	MVERSION;
		extern	int	mVERSION;

		printf("bsh %d.%02d (%s-%s-%s)\n\n", MVERSION, mVERSION, HOST_CPU, HOST_VENDOR, HOST_OS);
		printf("Copyright (C) 1982, 1984, 1985, 1988-1989, 1991, 1994-2011 Jörg Schilling\n");
		printf("This is free software; see the source for copying conditions.  There is NO\n");
		printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		exit(0);
	}
	if (be_fast)
		(*no_i2flg)++, no_histflg++;
	if (be_xfast)
		(*no_i2flg)++, no_histflg++, (*no_gaflg)++, (*no_laflg)++;
	if (cflg && streql(fbasename(initav0), commandname))
		(*no_i2flg)++;
}

EXPORT void
exitbsh(excode)
	int	excode;
{
	if (sflg) {
						/* see if its a top level */
						/* run final file */
#ifdef	INTERACTIVE
		if (!no_histflg && ev_eql(savehistname, on))
			save_history(FALSE);
#endif
		if (firstsh)
			dofile(concat(inithome, slash, finalname, (char *)NULL),
						GLOBAL_AB, 0, gstd, TRUE);
	}

#ifdef	INTERACTIVE
	reset_tty_modes();
	reset_line_disc();		/* Line discipline */
	reset_tty_pgrp();
#endif
	exit(excode);
}

LOCAL void
bshusage(flag, name, s)
	int	flag;
	char	*name;
	char	*s;
{
	if (flag)
		berror(ebadopt, name, s);
	berror("%s%s %s", usage, name, ubsh);
	if (flag)
		exit(1);
	else
		exit(0);
}
