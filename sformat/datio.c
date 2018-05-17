/* @(#)datio.c	1.26 09/12/19 Copyright 1988-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)datio.c	1.26 09/12/19 Copyright 1988-2009 J. Schilling";
#endif
/*
 *	IO routines for database
 *
 *	Copyright (c) 1988-2009 J. Schilling
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
/*#include <errno.h>*/
#include <schily/varargs.h>
#include <schily/unistd.h>	/* Include sys/types.h to make off_t available */
#include <schily/string.h>
#include <schily/utypes.h>
#include <schily/schily.h>
#include <schily/ctype.h>

#include "fmt.h"

extern	int	xdebug;

LOCAL FILE	*dat_file;
LOCAL char	*dat_fname;
LOCAL int	line;
LOCAL char	linebuf[512];
LOCAL char	*linep;
LOCAL char	*wordendp;
LOCAL char	wordendc;
LOCAL BOOL	initial_tab;
LOCAL BOOL	found_sig;

LOCAL	char	worddelim[] = "=:,";
LOCAL	char	nulldelim[] = "";

EXPORT	BOOL	opendatfile	__PR((char *));
EXPORT	char	*datfilename	__PR((void));
EXPORT	BOOL	past_df_sig	__PR((void));
EXPORT	BOOL	closedatfile	__PR((void));
EXPORT	int	rewinddatfile	__PR((void));
EXPORT	BOOL	datfile_chksum	__PR((void));
LOCAL	char	*setup_line	__PR((void));
EXPORT	char	*nextline	__PR((void));
EXPORT	BOOL	firstitem	__PR((void));
EXPORT	int	getlineno	__PR((void));
EXPORT	char	*curword	__PR((void));
EXPORT	char	*peekword	__PR((void));
LOCAL	void	ovstrcpy	__PR((char *p2, char *p1));
LOCAL	char	*markword	__PR((char *));
LOCAL	char	*getnextitem	__PR((char *));
EXPORT	char	*nextword	__PR((void));
EXPORT	char	*nextitem	__PR((void));
LOCAL	char	*next_table	__PR((char *));
EXPORT	char	*scanforline	__PR((char *, char *));
EXPORT	BOOL	scanfortable	__PR((char *, char *));
EXPORT	BOOL	checkequal	__PR((void));
EXPORT	BOOL	checkcomma	__PR((void));
EXPORT	BOOL	garbage		__PR((char *));
EXPORT	BOOL	isval		__PR((char *));
EXPORT	void	skip_illvar	__PR((char *, char *));
EXPORT	BOOL	set_stringvar	__PR((char *, char *, int));
EXPORT	int	datfileerr	__PR((char *, ...));

char	*datpath[] = {
		"",
		"/opt/schily/etc/",
		"/usr/bert/etc/",
		"/etc/",
		"/usr/etc/",
		"/opt/schily/etc/",
		NULL
	};

EXPORT BOOL
opendatfile(filename)
	char	*filename;
{
	FILE	*f	= (FILE *)NULL;
	static char	namebuf[128];
	char	*pp;
	int	pi	= -1;

	if (filename == NULL)
		filename = "sformat.dat";

	while (f == (FILE *)NULL) {
		if ((pp = datpath[++pi]) == NULL)
			break;
		sprintf(namebuf, "%s%.80s", pp, filename);
		if (xdebug)
			printf("namebuf: %s\n", namebuf);
		f = fileopen(namebuf, "r");
	}

	if (f == (FILE *)NULL) {
		errmsg("Can't open '%s'.\n", namebuf);
		return (FALSE);
	}
	dat_file = f;
	dat_fname = namebuf;
	line = 0;
	return (TRUE);
}

EXPORT char *
datfilename()
{
	return (dat_fname);
}

EXPORT BOOL
past_df_sig()
{
	return (found_sig);
}

EXPORT BOOL
closedatfile()
{
	if (fclose(dat_file) != 0)
		return (FALSE);

	dat_file = (FILE *)NULL;
	dat_fname = NULL;
	return (TRUE);
}

EXPORT int
rewinddatfile()
{
	if (dat_file == NULL)
		return (-1);
	if (fileseek(dat_file, 0L) < 0)
		return (-1);
	line = 0;
	found_sig = FALSE;
	return (0);
}

#define	sum1(s, x, p)	((x) ^= (*p), (x)++, (s) += (*p++))
#define	sum8(s, x, p)	{	sum1((s), (x), (p)); sum1((s), (x), (p)); \
				sum1((s), (x), (p)); sum1((s), (x), (p)); \
				sum1((s), (x), (p)); sum1((s), (x), (p)); \
				sum1((s), (x), (p)); sum1((s), (x), (p)); }

EXPORT BOOL
datfile_chksum()
{
	char	*word;
	register unsigned char	*p;
	register long	sum = 0L;
	register long	xsum = 0L;
	register long	n;
	BOOL	chksum_ok = FALSE;

	while ((n = fgetline(dat_file, linebuf, sizeof (linebuf))) >= 0) {
		line++;
		p = (unsigned char *)linebuf;
		if (*p == 's') {
			if (strncmp((char *)p, "signature", 9) == 0)
				break;
		}
		while (n >= 8) {
			sum8(sum, xsum, p);
			n -= 8;
		}
		while (--n >= 0)
			sum1(sum, xsum, p);
	}
	n = sum|(xsum<<24);
	p = (unsigned char *)bmap(bcrypt(n));
	(void) setup_line();
	word = nextword();
	if (streql(word, "signature")) {
		word = nextword();
		if (streql(word, "=")) {
			word = nextword();
			if (streql(word, (char *)p))
				chksum_ok = TRUE;
		}
	}

	if (bsecurity(1))
		fprintf(stderr, "sum: %ld xsum: %ld n: %ld '%s' ok: %d (%s)\n",
							sum, xsum, n,
							p, chksum_ok, word);
	rewinddatfile();
	return (chksum_ok);
}

LOCAL char *
setup_line()
{
	register char	*p;

	initial_tab = linebuf[0] == '\t';

	if ((p = strchr(linebuf, '#')) != NULL)
		*p = '\0';

	if (linebuf[0] == 's' && strncmp(linebuf, "signature", 8) == 0)
		found_sig = TRUE;

	wordendp = linep = linebuf;
	wordendc = *linep;
	return (linebuf);
}

EXPORT char *
nextline()
{
	do {
		fillbytes(linebuf, sizeof (linebuf), '\0');
		if (fgetline(dat_file, linebuf, sizeof (linebuf)) < 0)
			return (NULL);
		line++;
	} while (linebuf[0] == '#');

	return (setup_line());
}

EXPORT BOOL
firstitem()
{
	return (wordendp == linebuf);
}

EXPORT int
getlineno()
{
	return (line);
}

EXPORT char *
curword()
{
	return (linep);
}


EXPORT char *
peekword()
{
	return (&wordendp[1]);
}

/*
 * A strcpy() that works with overlapping buffers
 */
LOCAL void
ovstrcpy(p2, p1)
	register char	*p2;
	register char	*p1;
{
	while ((*p2++ = *p1++) != '\0')
		;
}

LOCAL char *
markword(delim)
	char	*delim;
{
	register	BOOL	quoted = FALSE;
	register	Uchar	c;
	register	Uchar	*s;

	for (s = (Uchar *)linep; (c = *s) != '\0'; s++) {
		if (c == '"') {
			quoted = !quoted;
			ovstrcpy((char *)s, (char *)&s[1]);
			c = *s;
		}
		if (!quoted && isspace(c))
			break;
		if (!quoted && strchr(delim, c) && s > (Uchar *)linep)
			break;
	}
	wordendp = (char *)s;
	wordendc = (char)*s;
	*s = '\0';

	return (linep);
}

LOCAL char *
getnextitem(delim)
	char	*delim;
{
	*wordendp = wordendc;

	linep = skipwhite(wordendp);
	return (markword(delim));
}

EXPORT char *
nextword()
{
	return (getnextitem(worddelim));
}

EXPORT char *
nextitem()
{
	return (getnextitem(nulldelim));
}

LOCAL char *
next_table(name)
	char	*name;
{
	do {
		if (!nextline())
			return (NULL);

		if (initial_tab)
			continue;

		markword(worddelim);

	} while (!streql(linep, name));
	return (linep);
}

EXPORT char *
scanforline(wanted, stopon)
	char	*wanted;
	char	*stopon;
{
	char	*word;

	do {
		if (!initial_tab)
			return ((char *)0);
		if (firstitem())
			word = nextitem();
		else
			word = linep;
		if (*word == 0)
			continue;
		if (!wanted)
			return (word);
		if (streql(word, wanted))
			break;
		if (stopon && streql(word, stopon))
			return (word);
		datfileerr("Expecting '%s' found : '%s'", wanted, word);
	} while ((word = nextline()) != NULL);

	return (word);
}

EXPORT BOOL
scanfortable(table, name)
	char	*table;
	char	*name;
{
	char	*word;

	while ((word = next_table(table)) != NULL) {
		if (xdebug)
			printf("curword: '%s' wordendp: '%c%s'\n",
				curword(), wordendc, peekword());
		if (!checkequal())
			continue;
		word = nextword();
		if (*word == '\0') {
			datfileerr("missing arg for '%s'", table);
			continue;
		}
		if (name == NULL)
			return (TRUE);
		if (streql(word, name))
			break;
	}
	if (!word)
		return (FALSE);

	(void) garbage(skipwhite(peekword()));
	return (TRUE);
}

EXPORT BOOL
checkequal()
{
	char	*word;

	word = nextword();
	if (!streql(word, "=")) {
		datfileerr("expected '=', got '%s'", word);
		return (FALSE);
	}
	return (TRUE);
}

EXPORT BOOL
checkcomma()
{
	char	*word;

	word = nextword();
	if (!streql(word, ",")) {
		datfileerr("expected ',', got '%s'", word);
		return (FALSE);
	}
	return (TRUE);
}

EXPORT BOOL
garbage(word)
	char	*word;
{
	if (*word) {
		datfileerr("Garbage '%s'", word);
		return (TRUE);
	}
	return (FALSE);

}

EXPORT BOOL
isval(word)
	char	*word;
{
	if (!*word) {
		datfileerr("Missing val");
		return (FALSE);
	}
	return (TRUE);
}

EXPORT void
skip_illvar(name, word)
	char	*name;
	char	*word;
{
	datfileerr("illegal %s var '%s'", name, word);

	if (xdebug) printf("skip_illvar: B: peekword() = '%s'\n", peekword());
	word = skipwhite(peekword());
	if (word && *word == '=') {
		(void) nextword();			/* skip '=' */
		for (;;) {
			word = skipwhite(peekword());
			if (word && *word != ':')
				(void) nextword();	/* skip args */
			else
				break;
		}
	}
	if (xdebug) printf("skip_illvar: E: peekword() = '%s'\n", peekword());
}

EXPORT BOOL
set_stringvar(name, np, len)
	char	*name;
	char	*np;
	int	len;
{
	char	*word;

	if (!checkequal())
		return (FALSE);
	word = nextword();
	if (xdebug) printf("%s: '%s'\n", name, word);

	if ((int)strlen(word) > len) {
		datfileerr("%s '%s' too long\n", name, word);
		return (FALSE);
	}
	strcpy(np, word);
	return (TRUE);
}

/* VARARGS1 */
EXPORT int
#ifdef	PROTOTYPES
datfileerr(char *fmt, ...)
#else
datfileerr(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	ret = errmsgno(EX_BAD, "%r on line %d in file '%s'.\n", fmt, args,
							line, dat_fname);
	va_end(args);
	return (ret);
}
