/* @(#)readfile.c	1.59 09/10/22 Copyright 1985-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)readfile.c	1.59 09/10/22 Copyright 1985-2009 J. Schilling";
#endif
/*
 *	Make program
 *	File/string reading routines
 *
 *	Copyright (c) 1985 by J. Schilling
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
#include <schily/types.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/schily.h>
#include "make.h"

LOCAL	int	fillrdbuf	__PR((void));
EXPORT	char	*peekrdbuf	__PR((void));
EXPORT	char	*getrdbuf	__PR((void));
EXPORT	int	getrdbufsize	__PR((void));
EXPORT	void	setincmd	__PR((BOOL isincmd));
EXPORT	void	getch		__PR((void));
EXPORT	int	peekch		__PR((void));
EXPORT	void	skipline	__PR((void));
EXPORT	void	readstring	__PR((char *str, char *strname));
EXPORT	void	readfile	__PR((char *name, BOOL must_exist));
EXPORT	void	doinclude	__PR((char *name, BOOL must_exist));
EXPORT	void	makeincs	__PR((void));

#if	defined(unix) || defined(IS_UNIX)
#	define	RDBUF_SIZE	1024
#else
#	define	RDBUF_SIZE	512
#endif

/*
 * Several variables needed for reading with look ahead
 * to allow easy parsing of make files.
 */
EXPORT	int	lastc		= 0;		/* last input character	    */
EXPORT	int	firstc		= 0;		/* first character in line  */
LOCAL	FILE	*mfp		= (FILE *)NULL;	/* currently open make file */
EXPORT	char	*mfname 	= NULL;		/* name of current make file */
LOCAL	int	olineno		= 1;		/* old line number (include) */
EXPORT	int	lineno		= 1;		/* current line number	    */
EXPORT	int	col		= 0;		/* current column	    */
LOCAL	BOOL	incmd		= FALSE;	/* cmd list line starts \n\t */
LOCAL	char	*readbfp;			/* current read buf pointer  */
LOCAL	char	*readbfstart;			/* start of current read buf */
LOCAL	char	*readbfend;			/* end of current read buf  */
LOCAL	char	rdbuf[RDBUF_SIZE];		/* the real read buffer	    */
LOCAL	char	*rd_buffer	= rdbuf;	/* a pointer to start of buf */

/*
 * Get or peek a character from current Makefile.
 */
#define	mygetc()	((readbfp >= readbfend) ? fillrdbuf() : *readbfp++)
#define	mypeekc()	((readbfp >= readbfend) ? (fillrdbuf() == EOF ?	\
						EOF:*--readbfp) : *readbfp)

/*
 * Fill or refill the read buffer that is used by the mygetc() CPP macro.
 */
LOCAL int
fillrdbuf()
{
	if (mfp == (FILE *) NULL)	/* EOF while reading from a string. */
		return (EOF);
	readbfp = rd_buffer;
	readbfstart = rd_buffer;	/* used for better error reporting */
	readbfend = rd_buffer + fileread(mfp, rd_buffer, RDBUF_SIZE);
	if (readbfp >= readbfend)
		return (EOF);
	return ((int) *readbfp++);
}

EXPORT char *
peekrdbuf()
{
	return (readbfp);
}

EXPORT char *
getrdbuf()
{
	return (readbfstart);
}

EXPORT int
getrdbufsize()
{
	return (readbfend - readbfstart);
}

/*
 * Switch the bahaviour of the reader for parsing commandlines/others.
 */
EXPORT void
setincmd(isincmd)
	BOOL	isincmd;
{
	incmd = isincmd ? TRUE:FALSE;
}

/*
 * Get a character.
 * Handle backslash-newline combinations and special conditions
 * for comment and command lines.
 * Count lines for error messages.
 */
EXPORT void
getch()
{
	col++;
	lastc = mygetc();
	if (lastc == EOF)
		return;
	if (lastc == '\n') {
		firstc = mypeekc();
		lineno++;
		col = 0;
		return;
	} else if (lastc == '\\' && !incmd && mypeekc() == '\n') {
		lastc = mygetc();
		firstc = mypeekc();
		lineno++;
		col = 0;
		for (;;) {		/* Skip white space at start of line */
			register int	c;

			c = mypeekc();
			if (c != ' ' && c != '\t') {
				lastc = ' ';
				return;
			}
			mygetc();
			col++;
		}
	}

	if (lastc == '#' && !incmd) {
		if (mfp == (FILE *) NULL)	/* Do not skip past # when */
			return;			/* reading from string.	   */
		skipline();
	}
}

EXPORT int
peekch()
{
	return (mypeekc());
}

/*
 * Fast method to skip to the end of a commented out line.
 * Always use the POSIX method (skip to next un-escaped new line).
 */
EXPORT void
skipline()
{
	register int	c = lastc;

	if (c == '\n')
		return;

	while (c != EOF) {
		c = mygetc();
		if (c == '\n') {
			lineno++;
			col = 0;
			lastc = c;
			firstc = mypeekc();
			return;
		} else if (c == '\\' && mypeekc() == '\n') {
			lineno++;
			col = 0;
			c = mygetc();
		}
	}
	firstc = lastc = c;
}

/*
 * Parse input from a string.
 */
EXPORT void
readstring(str, strname)
	char	*str;
	char	*strname;
{
	mfname = strname;
	readbfp = str;
	readbfstart = str;	/* used for better error reporting */
	readbfend = str + strlen(str);
	firstc = *str;
	incmd = FALSE;
	parsefile();
	mfname = NULL;
}

/*
 * Parse input from the current Makefile.
 */
EXPORT void
readfile(name, must_exist)
	char	*name;
	BOOL	must_exist;
{
	/*
	 * Diese Meldung ist noch falsch (Rekursion/Makefiles)
	 */
	if (Do_Warn)
		error("Reading file '%s' in line %d of '%s'\n", name,
			olineno, mfname);

	if (streql(name, "-")) {
		mfp = stdin;
		name = "Standard in";
	} else {
		if ((mfp = fileopen(name, "ru")) == (FILE *)NULL && must_exist)
			comerr("Can not open '%s'.\n", name);
	}
	mfname = name;
	readbfp = readbfend;		/* Force immediate call of fillrdbuf.*/
	firstc = mypeekc();
	incmd = FALSE;
	if (mfp) {
		parsefile();
		fclose(mfp);
	}
	mfp = (FILE *) NULL;
	mfname = NULL;
	col = 0;
}

list_t	*Incs;
list_t	**inctail = &Incs;

/*
 * Handle the "include" directive in makefiles.
 * If an include file does not exists, first try to find a rule to make it.
 * If this does not help, try to call include failure exception handling.
 * This exception handling enables some automake features of smake in allowing
 * the to call a shell script that will create the missing (may be architecture
 * dependant) include file on the fly with something that will at least allow
 * smake to continue on this platform.
 */
EXPORT void
doinclude(name, must_exist)
	char	*name;
	BOOL	must_exist;
{
	int	slc = lastc;
	int	sfc = firstc;
	FILE	*smf = mfp;
	char	*smfn = mfname;
	int	slineno = lineno;
	int	scol = col;
	char	*srbp = readbfp;
	char	*srbs = readbfstart;
	char	*srbe = readbfend;
	char	*srbf = rd_buffer;
	char	include_buf[RDBUF_SIZE];
	obj_t	*st = default_tgt;
	obj_t	*o;
	list_t	*lp;

	olineno = lineno-1;
	lastc	= 0;
	firstc	= 0;
	lineno	= 1;
	col	= 0;
	rd_buffer = include_buf;

	setup_dotvars();
	name = substitute(name, NullObj, 0, 0);
	name = strsave(name);

	xmake(name, FALSE);
	default_tgt = st;

	o = objlook(name, TRUE);

	/*
	 * In order to work around a gmake bug, we need to write Makefiles that
	 * make an included file to depend on a previously included file in
	 * order to make gmake believe that a rule exists to make the included
	 * file. This is otherwise nonsense but it is in conflict with our
	 * strategy to reset o->o_date after the file has been included in
	 * order to force to re-evaluate the complete set of rules after
	 * everything has been read. In this special case, it looks as if the
	 * file could not be made as it depends on a "nonexistent" target.
	 * A solution is to fetch the time again before we decide how to go on.
	 */
	if (o->o_date == 0) {
		o->o_date = gftime(name);	/* Check if file is present */
	}

	if (Debug > 1)
		error("doinclude(%s, %d)= date: %s level: %d\n",
			name, must_exist, prtime(o->o_date), o->o_level);

	if (must_exist && o->o_date == 0 && IncludeFailed) {
		list_t	l;

		o->o_date = newtime;		/* Force to be out of date  */
		l.l_next = (list_t *)0;		/* Only one element:	    */
		l.l_obj = o;			/* The file to be included  */
		IncludeFailed->o_list = &l;	/* Make it $^		    */
		IncludeFailed->o_date = (date_t)0;
		omake(IncludeFailed, FALSE);	/* Try to apply rules	    */
		o->o_date = gftime(name);	/* Check if file is present */
	}

	if (must_exist || o->o_date != 0) {
		char	includename[TYPICAL_NAMEMAX];
		char	*iname;

		if (Prdep)
			error("Reading file '%s' from '%s'\n", name, mfname);

		/*
		 * Zurücksetzen des Datums bewirkt Neuauswertung
		 * der Abhängigkeitsliste.
		 * XXX Das kann Probleme bei make depend geben.
		 */
		o->o_date = 0;
		/*
		 * Now add this object to the list of objects that must be
		 * remade to force integrity of uour lists before we start
		 * to make the real targets.
		 */
		lp = (list_t *) fastalloc(sizeof (*lp));
		lp->l_obj = o;
		*inctail = lp;
		inctail = &lp->l_next;
		lp->l_next = 0;

		iname = build_path(o->o_level, o->o_name, o->o_namelen,
					includename, sizeof (includename));
/*error("include '%s' -> '%s' %s\n", o->o_name, iname, prtime(o->o_date));*/
		if (iname != NULL) {
			readfile(iname, must_exist);
			if (iname != o->o_name && iname != includename)
				free(iname);
		} else {
			comerrno(EX_BAD,
				"Cannot build path for 'include %s'.\n",
				o->o_name);
		}
	}

	lastc = slc;
	firstc = sfc;
	mfp = smf;
	mfname = smfn;
	lineno = slineno;
	col = scol;
	readbfp = srbp;
	readbfstart = srbs;
	readbfend = srbe;
	rd_buffer = srbf;
}

/*
 * Re-make the included files.
 * This must be done because after they have been made the first time,
 * the dependency list may have changed. If we don't remake the included
 * files, the xxx.d dependency files will not be remade after we touch a file
 * that is not in the primary source list.
 */
EXPORT void
makeincs()
{
	list_t	*l;

	for (l = Incs; l != 0; l = l->l_next) {
/*		printf("inc(%s)\n", l->l_obj->o_name);*/
		omake(l->l_obj, TRUE);
	}
}
