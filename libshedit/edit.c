/* @(#)edit.c	1.19 15/08/01 Copyright 2006-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)edit.c	1.19 15/08/01 Copyright 2006-2015 J. Schilling";
#endif
/*
 *	Copyright (c) 2006-2015 J. Schilling
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

#include <schily/unistd.h>
#include <schily/varargs.h>
#include <schily/stat.h>
#include <schily/stdio.h>
#include "bsh.h"
#include "strsubs.h"
#include <schily/fstream.h>
#include <schily/shedit.h>
#define	toint		shell_toint

LOCAL fstream	*instrm = (fstream *) NULL;	/* Aliasexpanded input stream */
LOCAL fstream	*rawstrm = (fstream *) NULL;	/* Unexpanded input stream */

LOCAL	void	einit		__PR((void));
LOCAL	int	readchar	__PR((fstream *fsp));
EXPORT	int	shedit_egetc	__PR((void));
EXPORT	int	shedit_getdelim	__PR((void));
EXPORT	void	shedit_treset	__PR((void));
EXPORT	void	shedit_bhist	__PR((void));
EXPORT	void	shedit_bshist	__PR((int **ctlcpp));

/*
 * Set up file from where the inout should be read,
 * returns the old FILE * value.
 */
EXPORT FILE *
setinput(f)
	FILE	*f;
{
	if (rawstrm == (fstream *) NULL)
		rawstrm = mkfstream(f,
				(fstr_fun)0, readchar, (fstr_efun)berror);
	else
		f = fssetfile(rawstrm, f);

	if (instrm == (fstream *) NULL)			/* Pfusch in sgetc */
		instrm = mkfstream((FILE *)rawstrm,
				NULL, (fstr_rfun)0, (fstr_efun)berror);
	return (f);
}

extern  char    **environ;
extern	int	delim;
extern	int	prompt;
extern	char	*inithome;

#ifndef	LIB_SHEDIT
int
main(ac, av, ev)
	int	ac;
	char	*av[];
	char	*ev[];
{
	editloop();
}
#endif

LOCAL	int	__init;

LOCAL void
einit()
{
	char	*p;

	gstd[0] = stdin;
	gstd[1] = stdout;
	gstd[2] = stderr;
	evarray = environ;

	p = getcurenv("HOME");
	if (p)
		inithome = p;

	setinput(stdin);
	init_input();
	read_init_history();
	__init = TRUE;
}

LOCAL int
readchar(fsp)
	register fstream	*fsp;
{
#ifdef	INTERACTIVE
	pushline(get_line(prompt++, fsp->fstr_file));
	return (fsgetc(fsp));
#else
	return (getc(fsp->fstr_file));	/* read from FILE */
#endif
}

EXPORT int
shedit_egetc()
{
	if (!__init)
		einit();
	return (fsgetc(rawstrm));
}

EXPORT int
shedit_getdelim()
{
	return (delim);
}

#ifndef	LIB_SHEDIT
editloop()
{
	int	c;
	int	i = 0;

	while (c = shedit_egetc()) {
		printf("%c %o\n", c, c);
		if (c == '\r' || c == '\n') {
			printf("prompt %d\n", prompt);
			prompt = 0;
		}
	if (++i > 100)
		break;
	}
	exitbsh(0);
	return (0);
}
#endif

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

char	*
myhome()
{
	return (makestr(inithome));
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
	(void) fflush(stderr);
	return (ret);
}


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
		return ("");
	} else {
		estr = errmsgstr(err);
		if (estr == NULL) {
			sprintf(errbuf, "%d", err);
			estr = errbuf;
		}
		return (estr);
	}
}

/*
 * Push back a complete line
 */
EXPORT void
pushline(s)
	char	*s;
{
	if (s && rawstrm != (fstream *) NULL) {
		fspushcha(rawstrm, delim);
		fspushstr(rawstrm, s);
	}
}

/*
 * Check the current environment array against name and tval.
 */
EXPORT BOOL
ev_eql(name, tval)
	char	*name;
	char	*tval;
{
	char	*val;

	if ((val = getcurenv(name)) != NULL)
		return (streql(val, tval));
	return (FALSE);
}

EXPORT BOOL
is_dir(name)
	char	*name;
{
	struct	stat	buf;

	if (stat(name, &buf) < 0)
		return (FALSE);
	return ((buf.st_mode & S_IFMT) == S_IFDIR);
}


EXPORT void
exitbsh(excode)
	int	excode;
{
int	sflg = 1;

	if (sflg) {
						/* see if its a top level */
						/* run final file */
#ifdef	INTERACTIVE
		if (ev_eql("SAVEHISTORY", "on"))
			save_history(FALSE);
#endif
	}

#ifdef	INTERACTIVE
	reset_tty_modes();
	reset_line_disc();		/* Line discipline */
	reset_tty_pgrp();
#endif
	exit(excode);
}

EXPORT void
shedit_treset()
{
	if (ev_eql("SAVEHISTORY", "on"))
		save_history(FALSE);
	reset_tty_modes();
	reset_line_disc();		/* Line discipline */
	reset_tty_pgrp();
}

EXPORT void
shedit_bhist()
{
	put_history(gstd[1], TRUE);
}

EXPORT void
shedit_bshist(ctlcpp)
	int	**ctlcpp;
{
	if (ctlcpp)
		*ctlcpp = &ctlc;
	ctlc = 0;
	save_history(1);
}

EXPORT char *
shell_getenv(name)
	char	*name;
{
	extern	char	*(*__get_env)	__PR((char *__name));

	if (__get_env != NULL)
		return (__get_env(name));
	return (getenv(name));
}

EXPORT void
shell_putenv(name)
	char	*name;
{
	extern	void	(*__put_env)	__PR((char *__name));

	if (__put_env != NULL)
		__put_env(name);
	else
		putenv(name);
}

EXPORT void
shedit_getenv(genv)
	char	*(*genv) __PR((char *name));
{
	extern	char	*(*__get_env)	__PR((char *__name));

	__get_env = genv;
}

EXPORT void
shedit_putenv(penv)
	void	(*penv) __PR((char *name));
{
	extern	void	(*__put_env)	__PR((char *__name));

	__put_env = penv;
}

EXPORT void
shedit_setprompts(promptidx, nprompts, newprompts)
	int	promptidx;
	int	nprompts;
	char	*newprompts[];
{
	int	i;

	prompt = promptidx;

	for (i = 0; i < nprompts; i++)
		prompts[i] = newprompts[i];
}
