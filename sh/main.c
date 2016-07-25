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
#pragma ident	"@(#)main.c	1.37	06/06/16 SMI"
#endif

#include "defs.h"

/*
 * Copyright 2008-2016 J. Schilling
 *
 * @(#)main.c	1.53 16/07/11 2008-2016 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)main.c	1.53 16/07/11 2008-2016 J. Schilling";
#endif

/*
 * UNIX shell
 */
#ifdef	SCHILY_INCLUDES
#include	"sym.h"
#include	"hash.h"
#include	"timeout.h"
#include	<schily/types.h>
#include	<schily/stat.h>
#include	<schily/fcntl.h>
#include	<schily/wait.h>
#ifdef	INTERACTIVE
#include	<schily/shedit.h>
#endif
#include	"dup.h"
#include	"sh_policy.h"
#ifdef	DO_SYSALIAS
#include	"abbrev.h"
#endif
#undef	feof
#else
#include	"sym.h"
#include	"hash.h"
#include	"timeout.h"
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<sys/wait.h>
#include	"dup.h"
#include	"sh_policy.h"
#ifdef	DO_SYSALIAS
#include	"abbrev.h"
#endif
#endif

#ifdef RES
#include	<sgtty.h>
#endif

#define	no_pipe	(int *)0

pid_t mypid, mypgid, mysid;

static BOOL	beenhere = FALSE;
unsigned char	tmpout[TMPOUTSZ];
struct fileblk	stdfile;
struct fileblk *standin = &stdfile;
int mailchk = 0;

static unsigned char	*mailp;
static long	*mod_time = 0;
static BOOL login_shell = FALSE;

#if vax
char **execargs = (char **)(0x7ffffffc);
#endif

#if pdp11
char **execargs = (char **)(-2);
#endif

	int	main		__PR((int c, char *v[], char *e[]));
static void	exfile		__PR((int prof));
	void	chkpr		__PR((void));
	void	settmp		__PR((void));
static void	Ldup		__PR((int, int));
	void	chkmail		__PR((void));
	void	setmail		__PR((unsigned char *));
	void	setmode		__PR((int prof));
	void	secpolicy_print	__PR((int level, const char *msg));

int
main(c, v, e)
	int	c;
	char	*v[];
	char	*e[];
{
	int		rflag = ttyflg;
	int		rsflag = 1;	/* local restricted flag */
	struct namnod	*n;

	init_sigval();
	mypid = getpid();
	mypgid = getpgid(mypid);
	mysid = getsid(mypid);

	/*
	 * Do locale processing only if /usr is mounted.
	 */
	localedir_exists = (access(localedir, F_OK) == 0);

	/*
	 * initialize storage allocation
	 */

	if (stakbot == 0) {
		addblok((unsigned)0);
	}

	/*
	 * If the first character of the last path element of v[0] is "-"
	 * (ex. -sh, or /bin/-sh), this is a login shell
	 */
	if (*simple((unsigned char *)v[0]) == '-') {
#ifdef	SIGXCPU
		signal(SIGXCPU, SIG_DFL);
#endif
#ifdef	SIGXFSZ
		signal(SIGXFSZ, SIG_DFL);
#endif
		/*
		 * As the previous comment states, this is a login shell.
		 * Therefore, we set the login_shell flag to explicitly
		 * indicate this condition.
		 */
		login_shell = TRUE;
	}

	stdsigs();

	/*
	 * set names from userenv
	 */

	setup_env();

	/*
	 * LC_MESSAGES is set here so that early error messages will
	 * come out in the right style.
	 * Note that LC_CTYPE is done later on and is *not*
	 * taken from the previous environ
	 */

	/*
	 * Do locale processing only if /usr is mounted.
	 */
	if (localedir_exists)
		(void) setlocale(LC_ALL, "");
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "SYS_TEST"	/* Use this only if it weren't */
#endif
	(void) textdomain(TEXT_DOMAIN);

	/*
	 * This is a profile shell if the simple name of argv[0] is
	 * pfsh or -pfsh
	 */
#ifdef	EXECATTR_FILENAME
	if (c > 0 && (eq("pfsh", simple((unsigned char *)*v)) ||
	    eq("-pfsh", simple((unsigned char *)*v)) ||
	    eq("pfbosh", simple((unsigned char *)*v)) ||
	    eq("-pfbosh", simple((unsigned char *)*v)))) {
		flags |= pfshflg;
		secpolicy_init();
	}
#endif

	/*
	 * 'rsflag' is zero if SHELL variable is
	 *  set in environment and
	 *  the simple file part of the value.
	 *  is rsh
	 */
	if ((n = findnam((unsigned char *)"SHELL")) != NULL) {
		if (eq("rsh", simple(n->namval)) ||
		    eq("rbosh", simple(n->namval)))
			rsflag = 0;
	}

	/*
	 * a shell is also restricted if the simple name of argv(0) is
	 * rsh or -rsh in its simple name
	 */

#ifndef RES

	if (c > 0 && (eq("rsh", simple((unsigned char *)*v)) ||
	    eq("-rsh", simple((unsigned char *)*v)) ||
	    eq("rbosh", simple((unsigned char *)*v)) ||
	    eq("-rbosh", simple((unsigned char *)*v))))
		rflag = 0;

#endif

	if (eq("jsh", simple((unsigned char *)*v)) ||
	    eq("-jsh", simple((unsigned char *)*v)) ||
	    eq("jbosh", simple((unsigned char *)*v)) ||
	    eq("-jbosh", simple((unsigned char *)*v)))
		flags |= monitorflg;

#ifdef	DO_POSIX_PATH
	/*
	 * If the last path name component is not "sh", it may behave different.
	 */
	if (eq("sh", simple((unsigned char *)*v))) {
#ifdef	HAVE_GETEXECNAME
		const char	*exname = getexecname();
#else
			char	*exname = getexecpath();
#endif
		if (exname) {
			if (strstr(exname, "/xpg4"))	/* X-Open interface? */
				flags2 |= posixflg;

#ifdef	POSIX_BOSH_PATH
			if (eq(POSIX_BOSH_PATH, exname))
				flags2 |= posixflg;
#endif
#ifndef	HAVE_GETEXECNAME
			libc_free(exname);
#endif
		}
	}
#endif

	hcreate();
	set_dotpath();


	/*
	 * look for options
	 * dolc is $#
	 */
	dolc = options(c, (unsigned char **)v);

	if (dolc < 2) {
		flags |= stdflg;
		setopts();				/* set flagadr */
	}
	if ((flags & stdflg) == 0)
		dolc--;

	if ((flags & privflg) == 0) {
		uid_t euid;
		gid_t egid;
		uid_t ruid;
		gid_t rgid;

		/*
		 * Determine all of the user's id #'s for this process and
		 * then decide if this shell is being entered as a result
		 * of a fork/exec.
		 * If the effective uid/gid do NOT match and the euid/egid
		 * is < 100 and the egid is NOT 1, reset the uid and gid to
		 * the user originally calling this process.
		 */
		euid = geteuid();
		ruid = getuid();
		egid = getegid();
		rgid = getgid();
		if ((euid != ruid) && (euid < 100))
			setuid(ruid);   /* reset the uid to the orig user */
		if ((egid != rgid) && ((egid < 100) && (egid != 1)))
			setgid(rgid);   /* reset the gid to the orig user */
	}

	dolv = (unsigned char **)v + c - dolc;
	dolc--;

	/*
	 * return here for shell file execution
	 * but not for parenthesis subshells
	 */
	if (setjmp(subshell)) {
		freejobs();
#ifdef	DO_SYSALIAS
		/*
		 * Shell scripts start with empty alias definitions.
		 * Turn off all aliases and disable persistent aliases.
		 */
		ab_use(GLOBAL_AB, NULL);
		ab_use(LOCAL_AB, NULL);
#endif
		flags |= subsh;
	}

	/*
	 * number of positional parameters
	 */
	replace(&cmdadr, dolv[0]);	/* cmdadr is $0 */

	/*
	 * set pidname '$$'
	 */
	assnum(&pidadr, (long)mypid);

	/*
	 * set up temp file names
	 */
	settmp();

	/*
	 * default internal field separators
	 * Do not allow importing of IFS from parent shell.
	 * setup_env() may have set anything from parent shell to IFS.
	 * Always set the default ifs to IFS.
	 */
	assign(&ifsnod, (unsigned char *)sptbnl);

	dfault(&mchknod, (unsigned char *)MAILCHECK);
	mailchk = stoi(mchknod.namval);
#ifdef	DO_PS34
	dfault(&ps3nod, (unsigned char *)selectmsg);
	dfault(&ps4nod, (unsigned char *)execpmsg);
#endif
#ifdef	DO_PPID
	itos(getppid());
	dfault(&ppidnod, numbuf);
#ifdef	__readonly_ppid__
	attrib((&ppidnod), N_RDONLY);
#endif
#endif

	/* initialize OPTIND for getopt */

	n = lookup((unsigned char *)"OPTIND");
	assign(n, (unsigned char *)"1");
	/*
	 * make sure that option parsing starts
	 * at first character
	 */
	_sp = 1;

	if ((beenhere++) == FALSE) {	/* ? profile */
		if ((login_shell == TRUE) && (flags & privflg) == 0) {

			/* system profile */

#ifndef RES

			if ((input = pathopen((unsigned char *)nullstr,
					(unsigned char *)sysprofile)) >= 0)
				exfile(rflag);		/* file exists */

#endif
			/* user profile */

			if ((input = pathopen(homenod.namval?
					homenod.namval:UC "",
					(unsigned char *)profile)) >= 0) {
				exfile(rflag);
				flags &= ~ttyflg;
			}
		}
		if (rsflag == 0 || rflag == 0) {
			if ((flags & rshflg) == 0) {
				flags |= rshflg;
				setopts();		/* set flagadr */
			}
		}
#if	defined(INT_DOLMINUS) || defined(INTERACTIVE)
		if ((flags & stdflg) && (flags & oneflg) == 0 && comdiv == 0) {
			/*
			 * This is an interactive shell, mark it as interactive.
			 */
			if ((flags & intflg) == 0) {
				flags |= intflg;
			}
#ifdef	DO_BGNICE
			flags2 |= bgniceflg;
#endif
#ifdef	INTERACTIVE
			flags2 |= vedflg;
#endif
			setopts();			/* set flagadr */
		}
#endif
		if ((flags & intflg) && (flags & privflg) == 0) {
#ifdef	DO_SHRCFILES
			unsigned char	*env = envnod.namval;
			BOOL		dosysrc = TRUE;

			if (env == NULL)
				envnod.namval = env = UC rcfile;
			env = make(macro(env));

			if (env[0] == '/' && env[1] == '.' && env[2] == '/')
				dosysrc = FALSE;
			else if (env[0] == '.' && env[1] == '/')
				dosysrc = FALSE;

			flags &= ~intflg;	/* rcfiles: non-interactive */
			/* system rcfile */
			if (dosysrc &&
			    (input = pathopen((unsigned char *)nullstr,
					(unsigned char *)sysrcfile)) >= 0)
				exfile(rflag);		/* file exists */

			/* user rcfile */
			if ((input = pathopen((unsigned char *)nullstr,
					env)) >= 0) {
				exfile(rflag);
				flags &= ~ttyflg;
			}
			flags |= intflg;	/* restore interactive	*/
			setopts();		/* and flagadr		*/
			free(env);
#endif
#ifdef	DO_SYSALIAS
#ifdef	__never__
			/*
			 * The only way to have the global and local alias flag
			 * set is via the set(1) command and the set command
			 * code already reads the global and local alias files
			 * when the related flags are set.
			 */
			if ((flags2 & globalaliasflg) && homenod.namval) {
				catpath(homenod.namval, UC globalname);
				ab_use(GLOBAL_AB, (char *)make(curstak()));
			}
			if (flags2 & localaliasflg) {
				ab_use(LOCAL_AB, (char *)localname);
			}
#endif
#endif
		}

		/*
		 * open input file if specified
		 */
		if (comdiv) {		/* comdiv is -c arg */
			estabf(comdiv);
			input = -1;
		} else {
			if (flags & stdflg) {
				input = 0;
			} else {
			/*
			 * If the command file specified by 'cmdadr'
			 * doesn't exist, chkopen() will fail calling
			 * exitsh(). If this is a login shell and
			 * the $HOME/.profile file does not exist, the
			 * above statement "flags &= ~ttyflg" does not
			 * get executed and this makes exitsh() call
			 * longjmp() instead of exiting. longjmp() will
			 * return to the location specified by the last
			 * active jmpbuffer, which is the one set up in
			 * the function exfile() called after the system
			 * profile file is executed (see lines above).
			 * This would cause an infinite loop, because
			 * chkopen() will continue to fail and exitsh()
			 * to call longjmp(). To make exitsh() exit instead
			 * of calling longjmp(), we then set the flag forcexit
			 * at this stage.
			 */

				flags |= forcexit;
				input = chkopen(cmdadr, O_RDONLY);
				flags &= ~forcexit;
			}

#ifdef ACCT
			if (input != 0)
				preacct(cmdadr);
#endif
			comdiv--;
		}
	}
#ifdef pdp11
	else
		*execargs = (char *)dolv;	/* for `ps' cmd */
#endif


	exfile(0);
	done(0);
	return (exitval);	/* Keep lint happy */
}

static void
exfile(prof)
	int	prof;
{
	time_t	mailtime = 0;	/* Must not be a register variable */
	time_t	curtime = 0;

	/*
	 * move input
	 */
	if (input > 0) {
		Ldup(input, INIO);
		input = INIO;
	}


	setmode(prof);

	if (setjmp(errshell)) {
#ifdef	DO_SYSLOCAL
		if (localp) {
			localp = NULL;
			poplvars();
		}
#endif
		if (prof) {
			close(input);
			(void) endjobs(0);
			return;
		}
	}
	/*
	 * error return here
	 */

	loopcnt = peekc = peekn = 0;
	fndef = 0;
	nohash = 0;
	iopend = 0;

	if (input >= 0)
		initf(input);
	/*
	 * command loop
	 */
	for (;;) {
		intrcnt = 0;	/* Reset interrupt counter */
		tdystak(0, 0);
		stakchk();	/* may reduce sbrk */
		exitset();

		if ((flags & prompt) && standin->fstak == 0 && !eof) {

			if (mailp) {
				time(&curtime);

				if ((curtime - mailtime) >= mailchk) {
					chkmail();
					mailtime = curtime;
				}
			}

			/* necessary to print jobs in a timely manner */
			if (trapnote & TRAPSET)
				chktrap();

#ifdef	INTERACTIVE
			/*
			 * Make sure not to use variables from a shared
			 * library. This will not work on Mac OS X and
			 * on Solaris, it will pull in libshedit even
			 * when linked with -zlazyload.
			 * This is why we set up the prompt in libshedit
			 * indirectly here.
			 */
#define	EDIT_RPOMPTS	2
			if (flags2 & vedflg) {
				char *prompts[EDIT_RPOMPTS];
				if ((prompts[0] = C ps1nod.namval) == NULL)
					prompts[0] = C nullstr;
				if ((prompts[1] = C ps2nod.namval) == NULL)
					prompts[1] = C nullstr;

				shedit_setprompts(0, EDIT_RPOMPTS, prompts);
			} else
#endif
				prs(ps1nod.namval);	/* Ignores NULL ptr */

#ifdef TIME_OUT
			alarm(TIMEOUT);
#endif

		}

		trapnote = 0;
		peekc = readwc();
		if (eof) {
			if (endjobs(JOB_STOPPED))
				return;
			eof = 0;
		}

#ifdef TIME_OUT
		alarm(0);
#endif

#ifdef	DO_HASHCMDS
		if (peekc == '#' && (flags2 & hashcmdsflg)) {
			peekc = 0;
			hashcmd();
		} else
#endif
		{
			struct trenod *t;
			t = cmd(NL, MTFLG | SEMIFLG);
#ifdef	PARSE_DEBUG
			prtree(t, "Commandline: ");
#endif
			if (t == NULL && flags & ttyflg) {
				freejobs();
			} else {
				execbrk = 0;
				execute(t, 0, eflag, no_pipe, no_pipe);
			}
		}

		eof |= (flags & oneflg);

	}
}

/*
 * Print secondary prompt if not using the history editor.
 */
void
chkpr()
{
#ifdef	INTERACTIVE
	if ((flags2 & vedflg) == 0)
#endif
	if ((flags & prompt) && standin->fstak == 0)
		prs(ps2nod.namval);
}

void
settmp()
{
	int len;
	serial = 0;
	/*
	 * Should better use %ju and cast to maxint_t,
	 * but then we need to call js_snprintf() for portability.
	 */
	if ((len = snprintf((char *)tmpout, TMPOUTSZ, "/tmp/sh%lu",
	    (unsigned long)mypid)) >= TMPOUTSZ) {
		/*
		 * TMPOUTSZ should be big enough, but if it isn't,
		 * we'll at least try to create tmp files with
		 * a truncated tmpfile name at tmpout.
		 */
		tmpout_offset = TMPOUTSZ - 1;
	} else {
		tmpout_offset = len;
	}
}

/*
 * dup file descriptor fa to fb, close fa and set fb to close-on-exec
 */
static void
Ldup(fa, fb)
	int	fa;
	int	fb;
{
#ifdef RES

	dup(fa | DUPFLG, fb);
	close(fa);
	ioctl(fb, FIOCLEX, 0);

#else

	if (fa >= 0) {
		if (fa != fb) {
			close(fb);
			fcntl(fa, F_DUPFD, fb); /* normal dup */
			close(fa);
		}
		fcntl(fb, F_SETFD, FD_CLOEXEC);	/* autoclose for fb */
	}

#endif
}

void
chkmail()
{
	unsigned char	*s = mailp;
	unsigned char	*save;

	long	*ptr = mod_time;
	unsigned char	*start;
	BOOL	flg;
	struct stat	statb;

	while (*s) {
		start = s;
		save = 0;
		flg = 0;

		while (*s) {
			if (*s != COLON) {
				if (*s == '%' && save == 0)
					save = s;

				s++;
			} else {
				flg = 1;
				*s = 0;
			}
		}

		if (save)
			*save = 0;

		if (*start && stat((const char *)start, &statb) >= 0) {
			if (statb.st_size && *ptr &&
			    statb.st_mtime != *ptr) {
				if (save) {
					prs(save+1);
					newline();
				}
				else
					prs(_gettext(mailmsg));
			}
			*ptr = statb.st_mtime;
		} else if (*ptr == 0)
			*ptr = 1;

		if (save)
			*save = '%';

		if (flg)
			*s++ = COLON;

		ptr++;
	}
}

void
setmail(mailpath)
	unsigned char	*mailpath;
{
	unsigned char	*s = mailpath;
	int		cnt = 1;

	long	*ptr;

	free(mod_time);
	if ((mailp = mailpath) != NULL) {
		while (*s) {
			if (*s == COLON)
				cnt += 1;

			s++;
		}

		ptr = mod_time = (long *)alloc(sizeof (long) * cnt);

		while (cnt) {
			*ptr = 0;
			ptr++;
			cnt--;
		}
	}
}

void
setmode(prof)
	int	prof;
{
	/*
	 * decide whether interactive
	 */

	if ((flags & intflg) ||
	    ((flags&oneflg) == 0 &&
	    isatty(output) &&
	    isatty(input))) {
		dfault(&ps1nod, (unsigned char *)(geteuid() ?
						stdprompt : supprompt));
		dfault(&ps2nod, (unsigned char *)readmsg);
		flags |= ttyflg | prompt;
		if (mailpnod.namflg != N_DEFAULT)
			setmail(mailpnod.namval);
		else
			setmail(mailnod.namval);
		startjobs();
	} else {
		flags |= prof;
		flags &= ~prompt;
	}
}

/*
 * A generic call back routine to output error messages from the
 * policy backing functions called by pfsh.
 *
 * msg must contain '\n' if a new line is to be printed.
 */
void
secpolicy_print(level, msg)
	int		level;
	const char	*msg;
{
	switch (level) {
	case SECPOLICY_WARN:
	default:
		prs(_gettext(msg));
		return;
	case SECPOLICY_ERROR:
		error(msg);
		break;
	}
}
