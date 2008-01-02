/* @(#)fstream.c	1.24 08/01/02 Copyright 1985-2008 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)fstream.c	1.24 08/01/02 Copyright 1985-2008 J. Schilling";
#endif
/*
 *	Stream filter module
 *
 *	Copyright (c) 1985-2008 J. Schilling
 *
 *	Exported functions:
 *		mkfstream(f, fun, rfun, efun)	Construct new fstream
 *		fsclose(fsp)			Close a fstream
 *		fssetfile(fsp, f)		Replace file pointer in fstream
 *		fsgetc(fsp)			Get one character from fstream
 *		fspushcha(fsp, c)		Push one character on fstream
 *		fspushstr(fsp, str)		Push a string on fstream
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

#define	WSTRINGS
#include <schily/mconfig.h>
#include <stdio.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include <schily/string.h>
#include <schily/stdlib.h>
#include <schily/fstream.h>

EXPORT	fstream *mkfstream	__PR((FILE * f, fstr_fun sfun, fstr_rfun rfun, fstr_efun efun));
EXPORT	void	fsclose		__PR((fstream *fsp));
EXPORT	FILE	*fssetfile	__PR((fstream * fsp, FILE * f));
EXPORT	int	fsgetc		__PR((fstream * fsp));
EXPORT	void	fspushstr	__PR((fstream * fsp, char *ss));
EXPORT	void	fspushcha	__PR((fstream * fsp, int c));
LOCAL	void	s_ccpy		__PR((CHAR *ts, unsigned char *ss));
LOCAL	void	s_scpy		__PR((CHAR *ts, CHAR *ss));
LOCAL	void	s_scat		__PR((CHAR *ts, CHAR *ss));

EXPORT fstream *
mkfstream(f, sfun, rfun, efun)
	FILE		*f;	/* The "file" parameter may be a fstream * */
	fstr_fun	sfun;	/* The fstream transfer/filter function	   */
	fstr_rfun	rfun;	/* The read/input function		   */
	fstr_efun	efun;	/* The error function used by this func	   */
{
	register fstream *fsp;

	if ((fsp = (fstream *)malloc(sizeof (fstream))) == (fstream *)NULL) {
		if (efun)
			efun("no memory for new fstream");
		return ((fstream *)NULL);
	}
	fsp->fstr_bp = fsp->fstr_buf = fsp->fstr_sbuf;
	*fsp->fstr_bp  = '\0';
	fsp->fstr_file = f;
	fsp->fstr_func = sfun;
	fsp->fstr_rfunc = rfun;
	return (fsp);
}

EXPORT void
fsclose(fsp)
	fstream	*fsp;
{
	if (fsp->fstr_buf != fsp->fstr_sbuf)
		free(fsp->fstr_buf);
	free((char *)fsp);
}

EXPORT FILE *
fssetfile(fsp, f)
	register fstream	*fsp;
		FILE		*f;
{
	FILE *tmp = fsp->fstr_file;

	fsp->fstr_file = f;
	return (tmp);
}

EXPORT int
fsgetc(fsp)
	register fstream	*fsp;
{
	while (*fsp->fstr_bp == '\0') {			/* buffer is empty  */
		if (fsp->fstr_func != (fstr_fun)0) {	/* call function    */
			if ((*fsp->fstr_func)(fsp, fsp->fstr_file) == EOF)
				return (EOF);
		} else if (fsp->fstr_file == (FILE *)NULL) { /* no file	    */
			return (EOF);
		} else {				/* read from FILE   */
#ifdef DEBUG
			printf("character from file at %06x\n", fsp->fstr_file);
#endif
			if (fsp->fstr_rfunc != (fstr_rfun)0)
				return ((*fsp->fstr_rfunc)(fsp));
			return (EOF);
		}
	}
#ifdef DEBUG
	printf("character '%c' from buffer\n", *fsp->fstr_bp);
#endif
	return (*fsp->fstr_bp++);			/* char from buffer  */
}

EXPORT void
fspushstr(fsp, ss)
	register fstream *fsp;
	register char	*ss;
{
	register CHAR	*tp;
		CHAR	*ts;
		CHAR	tbuf[STR_SBUF_SIZE + 1];
	unsigned	len;

	for (tp = fsp->fstr_bp; *tp; tp++);	/* Wide char strlen() */
	len = tp - fsp->fstr_bp + strlen(ss);
/*	len = strlen(fsp->fstr_bp) + strlen(ss);*/
	if (len > STR_SBUF_SIZE) {			/* realloc !!! */
		if ((ts = (CHAR *)malloc(sizeof (*ts)*(len+1))) == NULL)
			raisecond("fspushstr", (long)NULL);
#ifdef	WSTRINGS
		s_ccpy(ts, (unsigned char *)ss);
		s_scat(ts, fsp->fstr_bp);
#else
		strcatl(ts, ss, fsp->fstr_bp, (char *)NULL);
#endif
		if (fsp->fstr_buf != fsp->fstr_sbuf)
			free(fsp->fstr_buf);
		fsp->fstr_buf = fsp->fstr_bp = ts;
	} else {
#ifdef	WSTRINGS
		s_scpy(tbuf, fsp->fstr_bp);
#else
		strcatl(tbuf, fsp->fstr_bp, (char *)NULL);
#endif
		if (fsp->fstr_buf != fsp->fstr_sbuf) {
			free(fsp->fstr_buf);
			fsp->fstr_buf = fsp->fstr_sbuf;
		}
#ifdef	WSTRINGS
		s_ccpy(fsp->fstr_buf, (unsigned char *)ss);
		s_scat(fsp->fstr_buf, tbuf);
#else
		strcatl(fsp->fstr_buf, ss, tbuf, (char *)NULL);
#endif
		fsp->fstr_bp = fsp->fstr_buf;
	}
}

EXPORT void
fspushcha(fsp, c)
	fstream	*fsp;
	int	c;
{
	char t[2];

	t[0] = (char)c;
	t[1] = 0;
	fspushstr(fsp, t);
#ifdef	WSTRINGS
	/*
	 * Solange es kein fspushstr mit SHORT * gibt, wird zuerst Platz
	 * geschafft und dann der korrekte Buchstabe eingetragen.
	 */
	*fsp->fstr_bp = c;
#endif
}

/*
 * Copy from narrow char string to wide char string
 */
LOCAL void
s_ccpy(ts, ss)
	register CHAR	*ts;
	register unsigned char	*ss;
/*	register char	*ss;*/
{
	while (*ss)
		*ts++ = *ss++;
	*ts = 0;
}

/*
 * Copy from wide char string to wide char string
 */
LOCAL void
s_scpy(ts, ss)
	register CHAR	*ts;
	register CHAR	*ss;
{
	while (*ss)
		*ts++ = *ss++;
	*ts = 0;
}

#ifdef	__needed__
LOCAL void
s_ccat(ts, ss)
	register string	ts;
	register unsigned char	*ss;
/*	register char	*ss;*/
{
	while (*ts)
		ts++;
	while (*ss)
		*ts++ = *ss++;
	*ts = 0;
}
#endif

LOCAL void
s_scat(ts, ss)
	register CHAR	*ts;
	register CHAR	*ss;
{
	while (*ts)
		ts++;
	while (*ss)
		*ts++ = *ss++;
	*ts = 0;
}
