/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
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

#pragma ident	"@(#)main.c	1.37	06/06/16 SMI"

/*
 * This file contains modifications Copyright 2008 J. Schilling
 *
 * @(#)main.c	1.8 08/03/28 2008 J. Schilling
 */
#ifndef lint
static	char sccsid[] =
	"@(#)main.c	1.8 08/03/28 2008 J. Schilling";
#endif

/*
 * UNIX shell
 */
#ifdef	SCHILY_BUILD
#include	"defs.h"
#include	"sym.h"
#include	"hash.h"
#include	"timeout.h"
#include	<schily/types.h>
#include	<schily/stat.h>
#include	<schily/fcntl.h>
#include	<schily/wait.h>
#include	"dup.h"
#include	"sh_policy.h"
#undef	feof
#else
#include	"defs.h"
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
	unsigned char	*flagc = flagadr;
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
		signal(SIGXCPU, SIG_DFL);
		signal(SIGXFSZ, SIG_DFL);

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
	if (c > 0 && (eq("pfsh", simple((unsigned char *)*v)) || eq("-pfsh", simple((unsigned char *)*v)))) {
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
	if (n = findnam((unsigned char *)"SHELL")) {
		if (eq("rsh", simple(n->namval)))
			rsflag = 0;
	}

	/*
	 * a shell is also restricted if the simple name of argv(0) is
	 * rsh or -rsh in its simple name
	 */

#ifndef RES

	if (c > 0 && (eq("rsh", simple((unsigned char *)*v)) || eq("-rsh", simple((unsigned char *)*v))))
		rflag = 0;

#endif

	if (eq("jsh", simple((unsigned char *)*v)) || eq("-jsh", simple((unsigned char *)*v)))
		flags |= monitorflg;

	hcreate();
	set_dotpath();


	/*
	 * look for options
	 * dolc is $#
	 */
	dolc = options(c, (unsigned char **)v);

	if (dolc < 2) {
		flags |= stdflg;
		{

			while (*flagc)
				flagc++;
			*flagc++ = STDFLG;
			*flagc = 0;
		}
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

	/* initialize OPTIND for getopt */

	n = lookup((unsigned char *)"OPTIND");
	assign(n, (unsigned char *)"1");
	/*
	 * make sure that option parsing starts
	 * at first character
	 */
	_sp = 1;

	if ((beenhere++) == FALSE)	/* ? profile */
	{
		if ((login_shell == TRUE) && (flags & privflg) == 0) {

			/* system profile */

#ifndef RES

			if ((input = pathopen((unsigned char *)nullstr,
					(unsigned char *)sysprofile)) >= 0)
				exfile(rflag);		/* file exists */

#endif
			/* user profile */

			if ((input = pathopen(homenod.namval,
					(unsigned char *)profile)) >= 0) {
				exfile(rflag);
				flags &= ~ttyflg;
			}
		}
		if (rsflag == 0 || rflag == 0) {
			if ((flags & rshflg) == 0) {
				while (*flagc)
					flagc++;
				*flagc++ = 'r';
				*flagc = '\0';
			}
			flags |= rshflg;
		}

		/*
		 * open input file if specified
		 */
		if (comdiv) {
			estabf(comdiv);
			input = -1;
		}
		else
		{
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
				input = chkopen(cmdadr, 0);
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
	time_t 	curtime = 0;

	/*
	 * move input
	 */
	if (input > 0) {
		Ldup(input, INIO);
		input = INIO;
	}


	setmode(prof);

	if (setjmp(errshell) && prof) {
		close(input);
		(void) endjobs(0);
		return;
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
		tdystak(0);
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
			{ extern int bsh_prompt;
			extern	char  *prompts[];
			bsh_prompt = 0;
			prompts[0] = (char *)ps1nod.namval;
			prompts[1] = (char *)ps2nod.namval;
			}
#else
			prs(ps1nod.namval);
#endif

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

		{
			struct trenod *t;
			t = cmd(NL, MTFLG);
			if (t == NULL && flags & ttyflg)
				freejobs();
			else
				execute(t, 0, eflag, no_pipe, no_pipe);
		}

		eof |= (flags & oneflg);

	}
}

void
chkpr()
{
#ifndef	INTERACTIVE
	if ((flags & prompt) && standin->fstak == 0)
		prs(ps2nod.namval);
#endif
}

void
settmp()
{
	int len;
	serial = 0;
	if ((len = snprintf((char *)tmpout, TMPOUTSZ, "/tmp/sh%u", mypid)) >=
	    TMPOUTSZ) {
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
			fcntl(fa, 0, fb); /* normal dup */
			close(fa);
		}
		fcntl(fb, 2, 1);	/* autoclose for fb */
	}

#endif
}

void
chkmail()
{
	unsigned char 	*s = mailp;
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
	int 		cnt = 1;

	long	*ptr;

	free(mod_time);
	if (mailp = mailpath) {
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
	    isatty(input)))

	{
		dfault(&ps1nod, (unsigned char *)(geteuid() ? stdprompt : supprompt));
		dfault(&ps2nod, (unsigned char *)readmsg);
		flags |= ttyflg | prompt;
		if (mailpnod.namflg != N_DEFAULT)
			setmail(mailpnod.namval);
		else
			setmail(mailnod.namval);
		startjobs();
	}
	else
	{
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