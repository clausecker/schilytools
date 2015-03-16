/* @(#)job.c	1.7 15/03/05 Copyright 1985, 87, 88, 91, 1995-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)job.c	1.7 15/03/05 Copyright 1985, 87, 88, 91, 1995-2015 J. Schilling";
#endif
/*
 *	Copyright (c) 1985, 87, 88, 91, 1995-2015 by J. Schilling
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

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/errno.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/wait.h>
#include <schily/schily.h>
#define	VMS_VFORK_OK
#include <schily/vfork.h>
#include "make.h"
#include "job.h"

EXPORT	job	*newjob		__PR((void));
EXPORT	void	setup_xvars	__PR((void));
extern	void	setup_SHELL	__PR((void));
#ifdef	__needed__
EXPORT	BOOL	cmd_prefix	__PR((char *cmd, int pfx));
EXPORT	BOOL	cmdlist_prefix	__PR((cmd_t *cmd, int pfx));
#endif /* __needed__ */
EXPORT	int	docmd		__PR((char * cmd, obj_t * obj));
EXPORT	void	cmd_run		__PR((job *jobp));
EXPORT	int	cmd_wait	__PR((job *jobp));
LOCAL	BOOL	has_meta	__PR((const char *s));
LOCAL	char	*doecho		__PR((char *s, BOOL));
LOCAL	void	doexec		__PR((const char *shpath, const char *shell,
				    const char *shellflag, const char *cmd,
				    BOOL force_shell));

/*
 * Older POSIX versions require commands to be executed via 'sh -c command'
 * which is wrong as it causes make not to stop on errors if called with
 * complex commands (e.g. commands that contain loops or a ';').
 * Fortunately, POSIX.1-2008 changed this back to 'sh -ce command'.
 *
 * We used to call /bin/sh -ce 'cmd' in former times which was correct,
 * but we got (unfortunately undocumented) problems on QNX and we changed
 * it to sh -c 'cmd' on 00/11/19. In hope that newer versions of QNX have
 * no problems with sh -ce, we changed it back on May 9th 2004.
 *
 * The solution to implement old POSIX compatibility without making smake
 * unusable is to also set "MAKE_SHELL_FLAG=-ce" in the makefile together
 * with .POSIX:
 */
#ifdef	OLD_POSIX
#define	POSIX_SHELL_CEFLAG	"-c"	/* Does not work correctly	*/
#else
#define	POSIX_SHELL_CEFLAG	"-ce"	/* New correct behavior		*/
#endif
#define	SHELL_CEFLAG		"-ce"	/* Needed for correct behavior	*/
#define	SHELL_CFLAG		"-c"	/* Used with make -i		*/

LOCAL	char	pceflag[] = POSIX_SHELL_CEFLAG; /* Cmds in posix mode	*/
LOCAL	char	ceflag[] = SHELL_CEFLAG; /* Used by default for cmds	*/
LOCAL	char	cflag[]  = SHELL_CFLAG;	/* Used with make -i		*/

EXPORT job *
newjob()
{
	job	*new = ___malloc(sizeof (job), "newjob");

	return (new);
}

/*
 * Set up some nice to have extended execution macros.
 */
EXPORT void
setup_xvars()
{
	char	*val;

	val = get_var("MAKE_SHELL_FLAG");
	if (val == NULL || *val == '\0') {
		if (posixmode)
			define_var("MAKE_SHELL_FLAG", pceflag);
		else
			define_var("MAKE_SHELL_FLAG", ceflag);	/* Used for make */
	}

	val = get_var("MAKE_SHELL_IFLAG");
	if (val == NULL || *val == '\0')
		define_var("MAKE_SHELL_IFLAG", cflag);	/* Used for make -i */
}

/*
 * Set up the special macro $(SHELL).
 *
 * On Operating Systems that use "bash" for /bin/sh, we have a real problem.
 * Bash does not stop on failed commands and as bash prevents nested jobs
 * from receiving signals.
 * If /bin/sh is a shell that does not work correctly and if a working
 * Bourne Shell is available as /bin/bosh, we prefer /bin/bosh over /bin/sh.
 *
 * On DJGPP, there is no /bin/sh. We need to find the path for "sh.exe".
 */
EXPORT void
setup_SHELL()
{
	char	*shell = NULL;
#ifdef	__DJGPP__
	char	*shellname;
#endif

#ifdef	BIN_SHELL_BOSH
#define	BOSH_PATH	"/bin/bosh"
#else
#ifdef	OPT_SCHILY_BIN_SHELL_BOSH
#define	BOSH_PATH	"/opt/schily/bin/bosh"
#endif
#endif
#ifdef	NO_BOSH
#undef	BOSH_PATH
#endif

#if (defined(BIN_SHELL_CE_IS_BROKEN) || \
	defined(SHELL_CE_IS_BROKEN) || \
	defined(USE_BOSH)) && \
	defined(BOSH_PATH)

	shell = BOSH_PATH;			/* Broken sh but bosh present */
#else	/* !SHELL_CE_IS_BROKEN */
#ifdef	__DJGPP__
	/*
	 * exec '/bin/sh' does not work under DJGPP.
	 * Strings like 'c:\djgpp\bin\sh.exe' or '/dev/c/djgpp/bin/sh.exe'
	 * must be used instead.
	 *
	 * Notes: c:/djgpp/share/config.site defines 'SHELL' with the required string.
	 *
	 * Using system("sh -ce 'cmd'") and spawn("command.com /c sh -ce 'cmd'")
	 * cause GPF's (not enough memory?)
	 *
	 * Temporary solution: Use DJGPP_SH envp. var. (must be set manually)
	 */
	shellname = get_var("SHELL");		/* On DJGPP too? */
	if (shellname != NULL && *shellname == '\0')
		shellname = NULL;
	if (shellname == NULL)
		shellname = getenv("DJGPP_SH");	/* Backward compat */
	if (shellname == NULL)
		shellname = searchfileinpath("bin/sh.exe", X_OK, TRUE, NULL); /* alloc() */
	if (shellname == NULL)
		shellname = searchfileinpath("sh.exe", X_OK, TRUE, NULL);	/* alloc() */
	if (shellname != NULL)
		shell = shellname;
#endif	/* !__DJGPP__ */
#endif	/* !SHELL_CE_IS_BROKEN */

	if (shell == NULL)
		shell = "/bin/sh";		/* Standard UNIX/POSIX case */
	define_var("SHELL", shell);		/* Needed for POSIX */
}


#define	MAXJ	1
int	curjobs = 0;
int	maxjobs = MAXJ;
job	jobs[MAXJ];

#define	iswhite(c)	((c) == ' ' || (c) == '\t')

#ifdef	__needed__
EXPORT BOOL
cmd_prefix(cmd, pfx)
	register char	*cmd;
	register int	pfx;
{
	register char	c;

	while ((c = *cmd++) != '\0') {
		if (c == ' ' || c == '\t') /* Permit spaces as other makes */
			continue;
		if (c != '@' && c != '-' && c != '+' && c != '?' && c != '!')
			break;
		if (c == pfx)
			return (TRUE);
	}
	return (FALSE);
}

EXPORT BOOL
cmdlist_prefix(cmd, pfx)
	register cmd_t	*cmd;
	register int	pfx;
{
	while (cmd) {
		if (cmd_prefix(cmd->c_line, pfx))
			return (TRUE);
		cmd = cmd->c_next;
	}
	return (FALSE);
}
#endif /* __needed__ */

/*
 * Execute or print a command line.
 * Return exit code from command line.
 */
EXPORT int
docmd(cmd, obj)
	register char	*cmd;
	obj_t		*obj;
{
	int	Silent = Sflag;
	int	NoError = Iflag;
	int	NoExec = Nflag;
	BOOL	foundplus = FALSE;
	BOOL	myecho = FALSE;
	job	*jobp = NULL;

	while (iswhite(*cmd))
		cmd++;

	/*
	 * '@' Silent
	 * '-' Ignore error
	 * '+' Always execute
	 * '?' No command dependency checking,
	 * '!' Force command dependency checking
	 */
	for (; *cmd; cmd++) {
		if (*cmd == '@') {
			Silent = TRUE;
		} else if (*cmd == '-') {
			NoError = TRUE;
		} else if (*cmd == '+') {
			NoExec = FALSE;
			foundplus = TRUE;
		} else if (*cmd == '?') {
			/* EMPTY */;		/* XXX To be defined !!! */
		} else if (*cmd == '!') {
			/* EMPTY */;		/* XXX To be defined !!! */
		} else if (*cmd == ' ' || *cmd == '\t') {
			continue;		/* Permit spaces in prefix */
		} else {
			break;			/* Not a command flag char */
		}
	}
	/*
	 * Check whether we have a leading simple echo command that we may
	 * inline in order to optimize command execution.
	 */
#ifndef	NO_MYECHO
	if (strncmp(cmd, "echo", 4) == 0 &&
	    (cmd[4] == ' ' || cmd[4] == '\t')) {
		char *p = doecho(cmd, FALSE);
		if (p != cmd)
			myecho = TRUE;
	}
#endif
	if (foundplus)
		Silent = FALSE;
	else if (!Silent && is_inlist(".SILENT", obj->o_name))
		Silent = TRUE;
	if (!NoError && is_inlist(".IGNORE", obj->o_name))
		NoError = TRUE;

	if (!Silent || NoExec || Debug > 0) {
/*		error("...%s\n", cmd);*/
		printf("%s%s\n", posixmode?"\t":"...", cmd);	/* POSIX !!! */
	}
	if (NoExec && !found_make)
		return (0);

	jobp = &jobs[0];
	if (curjobs >= maxjobs) {
		(void) cmd_wait(jobp);
	}

	jobp->j_pid = 0;
	jobp->j_flags = 0;
	if (Silent)
		jobp->j_flags |= J_SILENT;
	if (NoError)
		jobp->j_flags |= J_NOERROR;
#ifndef	NO_MYECHO
	if (myecho)
		jobp->j_flags |= J_MYECHO;
#endif
	jobp->j_cmd = cmd;
	jobp->j_obj = obj;
	curjobs++;

	curtarget = obj;

	cmd_run(jobp);
	return (cmd_wait(jobp));
}

EXPORT void
cmd_run(jobp)
	job	*jobp;
{
	pid_t	pid = 0;
	int	retries;
#if defined(USE_SYSTEM) || defined(__DJGPP__) || defined(__MINGW32__) || defined(_MSC_VER)
	int	Exit;
#endif
	int	NoError = (jobp->j_flags & J_NOERROR) != 0;
	obj_t	*shello;
	char	*shell = NULL;
	char	*shellflag;
	char	*cmd = jobp->j_cmd;
	BOOL	force_shell;

	force_shell = objlook(".FORCE_SHELL", FALSE) != NULL;
	if (force_shell)
		jobp->j_flags &= ~J_MYECHO;

	flush();			/* Flush stdout before running cmd */
#ifndef	NO_MYECHO
	if (jobp->j_flags & J_MYECHO) {
		cmd = doecho(cmd, TRUE);
		flush();
		if (*cmd == '\0') {	/* Command was completely inlined */
			jobp->j_flags |= J_NOWAIT;
			jobp->j_excode = 0;
			return;
		}
	}
#endif
	if (*cmd == '\0') {		/* Skip empty command */
		jobp->j_flags |= J_NOWAIT;
		jobp->j_excode = 0;
		return;
	}

	shellflag = get_var(NoError ? "MAKE_SHELL_IFLAG":"MAKE_SHELL_FLAG");
	if (shellflag == NULL || *shellflag == '\0') {
		if (posixmode)
			shellflag = pceflag;
		else
			shellflag = NoError ? cflag:ceflag;
	}

	shello = objlook("SHELL", FALSE);
	shell = get_var("SHELL");
	if (shell == NULL || *shell == '\0') {
		shello = NULL;
		shell = "/bin/sh";
	}

#if	!defined(USE_SYSTEM) &&			/* XXX else system() ??? */ \
	(((defined(HAVE_FORK) || defined(HAVE_VFORK)) && \
		defined HAVE_EXECL && defined HAVE_EXECVP) || defined(JOS))

#if defined(__EMX__) || defined(__DJGPP__) || defined(__MINGW32__) || defined(_MSC_VER)

#ifdef	__EMX__
	pid = spawnl(P_NOWAIT, shell, filename(shell), shellflag,
							cmd, (char *)NULL);
	if (pid < 0)
		comerr("Can't spawn %s.\n", shell);
#else	/* ! __EMX__ */

	if (shello == NULL)
		comerr("Can't find sh.exe.\n");		/* DJGPP setup problem */

	Exit = spawnl(P_WAIT, shell, filename(shell), shellflag,
							cmd, (char *)NULL);
	if (Exit) {
		/* TODO: DOS error code to UNIX error code */
		Exit = 0xFF<<8;
	}
	jobp->j_flags |= J_NOWAIT;
	jobp->j_excode = Exit;
#endif	/* ! __EMX__ */

#else	/* ! __EMX__ && ! __DJGPP__ && ! __MINGW32__ */
	/*
	 *  Do several tries to fork child to allow working on loaded systems.
	 */
	for (retries = 0; retries < 10; retries++) {
		pid = vfork();
		if (pid >= 0)
			break;
		sleep(1L);		/* Wait for resources to become free.*/
	}
	if (pid < 0)
#ifdef	vfork
		comerr("Can't fork.\n");
#else
		comerr("Can't vfork.\n");
#endif

	if (pid == 0) {		/* Child process: do the work. */
		/*
		 * Standard UNIX/POSIX case.
		 * We used to call /bin/sh -ce 'cmd' but we get problems on QNX
		 * and UNIX-98 requests that the command shall be called as in
		 * system() which means /bin/sh -c 'cmd'. As /bin/sh -c 'cmd'
		 * does not work correctly for complex commands, we now support
		 * MAKE_SHELL_IFLAG/MAKE_SHELL_FLAG which by default behaves
		 * like UNIX with /bin/sh -ce 'cmd'.
		 */
		doexec(shell, filename(shell), shellflag, cmd, force_shell);
		/* NOTREACHED */
		/*
		 * The error message is in doxec().
		 */
	}
#endif	/* ! __EMX__ && ! __DJGPP__ && ! __MINGW32__ */

	jobp->j_pid = pid;

#else	/* Use system() */
	Exit = system(cmd);
	jobp->j_flags |= J_NOWAIT;
	jobp->j_excode = Exit;
#endif
}

EXPORT int
cmd_wait(jobp)
	job	*jobp;
{
	int	code;
	pid_t	pid = jobp->j_pid;
	int	Exit;
	int	Silent = (jobp->j_flags & J_SILENT) != 0;
	int	NoError = (jobp->j_flags & J_NOERROR) != 0;
	obj_t	*shello;
	char	*shell = NULL;
	char	*cmd = jobp->j_cmd;
	obj_t	*obj = jobp->j_obj;

	shello = objlook("SHELL", FALSE);
	shell = get_var("SHELL");
	if (shell == NULL || *shell == '\0') {
		shello = NULL;
		shell = "/bin/sh";
	}

	if (jobp->j_flags & J_NOWAIT) {
		Exit = jobp->j_excode;
		goto nowait;
	}

	/* Parent process: wait for child. */
#ifdef	HAVE_WAIT
	if ((code = wait((WAIT_T *)&Exit)) != pid)
		comerrno(Exit, "code %d waiting for %s.\n", geterrno(), shell);
#else
	/*
	 * Some platforms (__DJGPP__, __MINGW32__) do not have wait(), but we
	 * set J_NOWAIT on such platforms and thus should never come here.
	 * WIN-DOS (__MINGW32__) offers a cwait() that is not compatible to
	 * cwait() from UNOS, so WIN-DOS created a clash.
	 */
	/* NOTREACHED */
	errmsgno(EX_BAD,
	"Implementation botch, wait for %s not possible on this platform.\n",
			shell);
#endif

nowait:
	if (Exit) {
		errmsgno(Exit>>8, "*** Code %d from command line for target '%s'%s.\n",
			Exit>>8,
			obj->o_name,
			NoError?" (ignored)":"");
		if (Silent && Debug <= 0) {
			errmsgno(EX_BAD, "The following command caused the error:\n");
			error("%s\n", cmd);
		}
	}
	curjobs--;
	jobp->j_pid = 0;
	jobp->j_obj = (obj_t *)NULL;
	curtarget = (obj_t *)NULL;

	if (NoError)
		return (0);
	return (Exit);
}

/*
 * Check for Shell meta characters
 *
 * Compiler calls frequently contain '=' in arguments but this still
 * do not need a shell. We thus made the check clever enough to detect
 * local environment settings like "NAME=value cmd ..."
 */
LOCAL char *shell_meta = "#|^();&<>*?[]:$`'\"\\\n";

LOCAL BOOL
has_meta(s)
	register const char	*s;
{
	register char	c;
	register int	firstword = TRUE;
	register char	*smeta = shell_meta;

	while (*s == ' ' || *s == '\t')
		s++;
	while ((c = *s++) != '\0') {
		if (strchr(smeta, c)) {
			return (TRUE);
		}
		/*
		 * At this point, we checked already against '\\' and ';',
		 * so the following code can be simple.
		 */
		if (c == ' ') {
			firstword = FALSE;
		} else if (c == '=' && firstword) {
			return (TRUE);
		}
	}
	return (FALSE);
}

/*
 * Our inline implementation for the /bin/echo command.
 * We need to implement the behavior of the command line parser and the
 * behavior of the echo command here.
 * Return a pointer past the first echo command if we can use our inline
 * version.
 * Return a pointer to the original command line if this echo command must
 * be executed by the shell, because there is I/O redirection or a shell
 * variable inside the echo arguments.
 */
#ifndef	NO_MYECHO
LOCAL char *
doecho(s, print)
	register char	*s;
		BOOL	print;
{
	char	*old = s;
	BOOL	singleq = FALSE;
	BOOL	doubleq = FALSE;
	BOOL	nextarg = FALSE;

	s += 4;		/* strlen("echo") */
	while (*s == ' ' || *s == '\t')
		s++;
	for (; *s != '\0'; s++) {
		switch (*s) {
		case '"':
			if (!singleq && !doubleq)
				doubleq = TRUE;
			else if (doubleq)
				doubleq = FALSE;
			break;
		case '\'':
			if (!singleq && !doubleq)
				singleq = TRUE;
			else if (singleq)
				singleq = FALSE;
			break;
		case ' ':
		case '\t':
			if (singleq || doubleq)
				goto normal;
			nextarg = TRUE;
			break;
		case '\\':
			if (singleq)
				goto normal;
			if (doubleq) {
				if (s[1] == '\\' || s[1] == '"')
					s++;
				goto normal;
			}
			if (*++s == '\0')
				goto out;
			goto normal;
		default:
			if (!singleq && !doubleq &&
			    strchr(shell_meta,  *s)) {
				return (old);
			} else if (doubleq && *s == '$') {
				return (old);
			}
		normal:
			if (nextarg) {
				if (print)
					printf(" ");
				nextarg = FALSE;
			}
			if (print) {
				register char c = *s;

				if (c == '\\' && s[1] != '\0') {
					c = *++s;
					switch (c) {
					case 'a': c = '\a'; break;
					case 'b': c = '\b'; break;
					case 'c': print = FALSE; goto ndone;
					case 'f': c = '\f'; break;
					case 'n': c = '\n'; break;
					case 'r': c = '\r'; break;
					case 't': c = '\t'; break;
					case 'v': c = '\v'; break;
					case '\\': break;
					case '0': {	int i = 3;
							int v = 0;

							while (*++s >= '0' &&
								*s <= '7' &&
								--i >= 0) {
								v *= 8;
								v += *s - '0';
							}
							--s;
							c = v;
							break;
						}
					default: printf("\\"); break;
					}
				}
				printf("%c", c);
			}
		ndone:
			break;
		case ';':
			/*
			 * Point past the ';' after the echo command
			 * and skip the following white space.
			 */
			while (*s == ';' || *s == ' ' || *s == '\t')
				s++;
			goto out;
		}
	}
out:
	if (print)
		printf("\n");
	return (s);
}
#endif	/* NO_MYECHO */

/*
 * Call a command via the shell.
 * If the command line does not contain shell specifix meta characters,
 * call the command directly for efficiency.
 */
LOCAL void
doexec(shpath, shell, shellflag, cmd, force_shell)
	const char	*shpath;
	const char	*shell;
	const char	*shellflag;
	const char	*cmd;
	BOOL		force_shell;
{
#define	MAXCMD	NAMEMAX		/* Currently 512 for pdp11, 4096 for others */

	if (!force_shell &&
	    strlen(cmd) <= MAXCMD && !has_meta(cmd)) {	/* Simple, no shell */
		char	buf[MAXCMD+1];
		char	*args[MAXCMD/2+1];	/* No malloc, we use vfork() */
		char	*bp;
		char	**ap;
		int	err;

		strcpy(buf, cmd);
		for (bp = buf, ap = args; *bp; bp++, ap++) {
			while (*bp == ' ' || *bp == '\t')
				bp++;
			*ap = bp;
			while (*bp != '\0' && *bp != ' ' && *bp != '\t')
				bp++;
			if (*bp == '\0') {	/* End of cmd line	*/
				if (**ap)	/* Only increment if	*/
					ap++;	/* arg not empty ("").	*/
				break;
			}
			*bp = '\0';
		}
		*ap = NULL;
		if (args[0] == NULL)		/* Empty command?	*/
			_exit(0);		/* Just signal success	*/
		execvp(args[0], args);
		err = geterrno();
		errmsg("Can't exec %s.\n", args[0]);
		_exit(err);
		/* NOTREACHED */
	} else {				/* Complex cmd, use shell */
		execl(shpath, shell, shellflag, cmd, (char *)NULL);
		comerr("Can't exec %s.\n", shpath);
		/* NOTREACHED */
	}
}
