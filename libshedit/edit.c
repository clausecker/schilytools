/* @(#)edit.c	1.5 08/12/22 Copyright 2006-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)edit.c	1.5 08/12/22 Copyright 2006-2008 J. Schilling";
#endif
/*
 *	Copyright (c) 2006-2008 J. Schilling
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
#include <schily/unistd.h>
#include <schily/varargs.h>
#include <schily/stat.h>
#include <stdio.h>
#include "bsh.h"
#include "strsubs.h"
#include <schily/fstream.h>

LOCAL fstream	*instrm = (fstream *) NULL;	/* Alias expanded input stream */
LOCAL fstream	*rawstrm = (fstream *) NULL;	/* Unexpanded input stream */

int	__in__	= STDIN_FILENO;
int	__out__	= STDOUT_FILENO;
int	__err__	= STDERR_FILENO;

LOCAL	void	einit		__PR((void));
LOCAL	int	readchar	__PR((fstream *fsp));
EXPORT	int	egetc		__PR((void));
EXPORT	int	listlen		__PR((Tnode * lp));
EXPORT	void	bsh_treset	__PR((void));
EXPORT	void	bhist		__PR((void));

/*
 * Set up file from where the inout should be read,
 * returns the old FILE * value.
 */
EXPORT FILE *
setinput(f)
	FILE	*f;
{
	if (rawstrm == (fstream *) NULL)
		rawstrm = mkfstream(f, (fstr_fun)0, readchar, (fstr_efun)berror);
	else
		f = fssetfile(rawstrm, f);

	if (instrm == (fstream *) NULL)			/* Pfusch in sgetc */
		instrm = mkfstream((FILE *) rawstrm, NULL, (fstr_rfun)0, (fstr_efun)berror);
	return (f);
}

FILE	*gstd[3];
extern  char    **environ;
char    **evarray;

int	delim;
int	ctlc;
int	ex_status;
int	ttyflg = 1;
int	prflg = 1;
int	prompt;
char	*prompts[2] = { "prompt1 > ", "prompt2 > " };
int	delim;
char	*inithome = ".";


#ifndef	LIB
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

/*	p = getcurenv(homename);*/
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
egetc()
{
	if (!__init)
		einit();
	return (fsgetc(rawstrm));
}

#ifndef	LIB
editloop()
{
	int	c;
	int	i = 0;

	while (c = egetc()) {
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


#ifdef	__BEOS__
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
/*		return (nullstr);*/
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

EXPORT int
listlen(lp)
	register Tnode	*lp;
{
	register int	i;

	for (i = 0; lp != (Tnode *) NULL; lp = lp->tn_right.tn_node)
		i++;
	return (i);
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
/*		if (!no_histflg && ev_eql(savehistname, on))*/
		if (ev_eql("SAVEHISTORY", "on"))
			save_history(FALSE);
#endif
/*		if (firstsh)*/
/*			dofile(concat(inithome, slash, finalname, (char *)NULL),*/
/*							GTAB, gstd, TRUE);*/
	}

#ifdef	INTERACTIVE
	reset_tty_modes();
	reset_line_disc();		/* Line discipline */
	reset_tty_pgrp();
#endif
	exit(excode);
}

EXPORT void
bsh_treset()
{
	if (ev_eql("SAVEHISTORY", "on"))
		save_history(FALSE);
	reset_tty_modes();
	reset_line_disc();		/* Line discipline */
	reset_tty_pgrp();
}

EXPORT void
bhist()
{
	put_history(gstd[1], TRUE);
}
