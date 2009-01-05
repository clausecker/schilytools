/* @(#)translit.c	1.13 08/12/22 Copyright 1985-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)translit.c	1.13 08/12/22 Copyright 1985-2008 J. Schilling";
#endif

/*
 *	translit - translate characters
 *
 *	translit fromset toset file1...filen
 *
 *	Copyright 1985-2008 J. Schilling
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
#include <stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>	/* Include sys/types.h */
#include <schily/utypes.h>
#include <schily/string.h>
#include <schily/schily.h>

#define	TBUFSIZE	4096	/* Scratch buffer size for unescaped chars */
#define	NUMCHARS	256	/* TYPE_MAXVAL(Uchar) + 1		*/

LOCAL	Uchar	trchars[256];	/* Character translation table		*/
LOCAL	Uchar	delchars[256];	/* Chars to delete from output		*/
LOCAL	Uchar	sqchars[256];	/* Multchars to replace w. single char  */
LOCAL	BOOL	cflag = FALSE;
LOCAL	BOOL	foldflag = FALSE;
LOCAL	Uchar	foldchar = '\0';
LOCAL	BOOL	is_translit;

LOCAL	void	usage		__PR((int excode));
EXPORT	int	main		__PR((int ac, char **av));
LOCAL	void	tr		__PR((FILE *f));
LOCAL	void	buildtabs	__PR((Uchar *fromset, Uchar *toset,
							Uchar *sqset));
LOCAL	int	buildset	__PR((Uchar *inp, Uchar *buf, int bsize,
						char *tname, BOOL notflg));
LOCAL	char	unesc		__PR((Uchar **cpp));
LOCAL	int	inset		__PR((char c, Uchar *buf, int len));
LOCAL	int	etoolarge	__PR((char *s));
LOCAL	const char *filename	__PR((const char *name));

LOCAL void
usage(excode)
	int	excode;
{
	error("Usage:	translit [options] fromset toset [file1...filen]\n");
	error("	-help	Print this help.\n");
	error("	-version Print version number.\n");
	error("	-c	Complement the set of values specified in 'fromset'.\n");
	error("	-d	Delete all characters specified in 'fromset'.\n");
	error("	-s	Replace repeated characters by a single character.\n");
	error("Standard in is used if no files are given.\n");
	exit(excode);
}

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	FILE	*f;
	char	*opts = "help,version,c,d,s";
	Uchar	*fromset = NULL;
	Uchar	*toset = NULL;
	Uchar	*sqset = NULL;
	BOOL	help = FALSE;
	BOOL	prversion = FALSE;
	BOOL	delflg = FALSE;
	BOOL	sqflg = FALSE;
	int	cac;
	char * const* cav;

	save_args(ac, av);
	is_translit = streql(filename(av[0]), "translit");
	cac = --ac;
	cav = ++av;
	file_raise((FILE *)NULL, FALSE);

	if (getallargs(&cac, &cav, opts, &help, &prversion,
					&cflag, &delflg, &sqflg) < 0) {
		errmsgno(EX_BAD, "Bad flag: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (prversion) {
		printf(
"Translit release %s (%s-%s-%s) Copyright (C) 1985-2008 Jörg Schilling\n",
				"1.13",
				HOST_CPU, HOST_VENDOR, HOST_OS);
		exit(0);
	}

	cac = ac;
	cav = av;
	if (getfiles(&cac, &cav, opts) <= 0) {
		errmsgno(EX_BAD, "No 'from' string given.\n");
		usage(EX_BAD);
	}
	fromset = (Uchar *)cav[0];
	cac--, cav++;

	if (!(delflg ^ sqflg)) {
		if (getfiles(&cac, &cav, opts) <= 0) {
			errmsgno(EX_BAD, "No 'to' string given.\n");
			usage(EX_BAD);
		}
		toset = (Uchar *)cav[0];
		cac--, cav++;
		if (sqflg)
			sqset = (Uchar *)toset;
	}
	if (delflg)
		toset = (Uchar *)"";
	else if (sqflg)
		sqset = (Uchar *)fromset;

	buildtabs(fromset, toset, sqset);

	if (getfiles(&cac, &cav, opts) > 0) {
		for (; getfiles(&cac, &cav, opts) > 0; cac--, cav++) {
			if (cav[0][0] == '-' && cav[0][1] == '\0') {
				f = stdin;
			} else {
				f = fileopen(cav[0], "r");
				if (f == NULL)
					comerr("Cannot open '%s'.\n", cav[0]);
			}
			tr(f);
			if (f != stdin)
				(void) fclose(f);
		}
	} else {
		tr(stdin);
	}
	return (0);
}

LOCAL void
tr(f)
	register FILE	*f;
{
	register int	lastc = EOF;
	register int	c;
	register int	oc;

	while ((c = getc(f)) >= 0) {
		if (sqchars[c & 255]) {
			oc = c;
			if (oc != lastc)
				(void) putchar(oc);
			lastc = oc;
		} else if (!delchars[c & 255]) {
			oc = trchars[c & 255] & 255;

			if (!foldflag || oc != lastc || oc != foldchar) {
				(void) putchar(oc);
			}
			lastc = oc;
		}
	}
	if (feof(f))
		return;
	if (ferror(f))
		comerr("Read error on input.\n");
}

LOCAL void
buildtabs(fromset, toset, sqset)
	Uchar 		*fromset;
	Uchar		*toset;
	Uchar		*sqset;
{
	Uchar		frombuf[256];
	Uchar		tobuf[256];
	Uchar		sqbuf[256];
	int		fromcnt;
	int		tocnt;
	int		sqcnt;
	register int	i;

	/*
	 * Initialize all tables.
	 */
	for (i = 0; i < 256; i++) {
		trchars[i] = (Uchar) i;
		delchars[i] = FALSE;
		sqchars[i]  = FALSE;
	}
	fromcnt = buildset(fromset, frombuf, sizeof (frombuf), "from", cflag);
	tocnt = buildset(toset, tobuf, sizeof (tobuf), "to", FALSE);
	sqcnt = buildset(sqset, sqbuf, sizeof (sqbuf), "squeeze", FALSE);
	if (tocnt > fromcnt) {
		comerrno(EX_BAD, "'to' set larger than 'from' set.\n");
	} else if (tocnt == 0) {
		for (i = 0; i < fromcnt; i++)
			delchars[frombuf[i & 255] & 255] = TRUE;
	} else {
		foldchar = tobuf[tocnt-1];
		for (i = 0; i < fromcnt; i++) {
			if (tocnt >= 0 && i >= tocnt) {
				foldflag = TRUE;
				trchars[frombuf[i & 255] & 255] = foldchar;
			} else {
				trchars[frombuf[i & 255] & 255] = tobuf[i];
			}
		}
	}
	for (i = 0; i < sqcnt; i++) {
		sqchars[sqbuf[i & 255] & 255] = TRUE;
	}
	if (!is_translit)
		foldflag = FALSE;
}

#define	put(c, p, l, tn)	((((l)-- <= 0) && etoolarge(tn)), \
							*(p)++ = (c) & 255)

LOCAL int
buildset(inp, buf, bsize, tname, notflg)
	Uchar	*inp;
	Uchar	*buf;
	int	bsize;
	char	*tname;
	BOOL	notflg;
{
	Uchar	set[TBUFSIZE];
	Uchar	*setp = set;
	int	setsize = TBUFSIZE;
register int	i;
register int	to;

	if (inp == NULL)
		return (-1);
	buf[0] = '\0';
	set[0] = '\0';
	if (is_translit && !notflg) {
		if ((notflg = (*inp == '^')) != 0)
			inp++;
	}
	for (; *inp != '\0'; inp++) {
		switch (*inp) {

		case '[':			/* Start of character class */

			if (inp[1] == '\0') {		/* End of string */
				put(*inp, setp, setsize, tname);
				break;
			}

			for (inp++; *inp != '\0'; inp++) {

				if (*inp == ']' || *inp == '\0')
					break;
				else if (*inp == '\\' && inp[1] != '\0')
					put(unesc(&inp), setp, setsize, tname);
				else
					put(*inp, setp, setsize, tname);

				if (inp[1] == '-' &&
				    inp[2] != '\0' &&
				    inp[2] != ']') {
					inp += 2;
					i = setp[-1];
					if (*inp == '\\' && inp[1] != '\0')
						to = unesc(&inp);
					else
						to = *inp;
					i &= 255;
					to &= 255;
					if (i > to) {
						for (i--; i >= to; i--) {
							put(i, setp, setsize,
									tname);
						}
					} else {
						for (i++; i <= to; i++) {
							put(i, setp, setsize,
									tname);
						}
					}
				}
			}
			if (*inp != ']')
				comerrno(EX_BAD, "Missing ']'.\n");
			break;

		case '\\':
			if (inp[1] != '\0') {
				put(unesc(&inp), setp, setsize, tname);
				break;
			}
			/* FALLTHROUGH */

		default:
			put(*inp, setp, setsize, tname);
			break;
		}
	}
	setsize = TBUFSIZE - setsize;	/* Convert remaining to content size */
	if (notflg) {
		int	n = 0;

		for (n = 0, i = 0; i < 256; i++) {
			if (!inset(i, set, setsize)) {
				n++;
				put(i, buf, bsize, tname);
			}
		}
		setsize = n;
	} else {
		for (i = 0; i < setsize; i++)
			put(set[i], buf, bsize, tname);
	}
	return (setsize);
}

LOCAL char
unesc(cpp)
	Uchar	**cpp;
{
	char	c;
	int	result = 0;
	int	ndig = 0;
#define	octal(c)	(c >= '0' && c <= '7')

	(*cpp)++;		/* Skip '\\' */
	switch (c = **cpp) {

	case 'a':
		return ('\a');
	case 'b':
		return ('\b');
	case 'f':
		return ('\f');
	case 'n':
		return ('\n');
	case 'r':
		return ('\r');
	case 't':
		return ('\t');
	case 'v':
		return ('\v');
	default:
		if (octal(c)) {
			for (; ndig < 3 && octal(c);
			    c = *(++(*cpp)), ndig++) {
				result = result * 8 + c - '0';
			}
			(*cpp)--;
		} else {
			result = c;
		}
		return (result & 255);
	}
}

#ifdef	PROTOTYPES
LOCAL int
inset(char c, Uchar *buf, int len)
#else
LOCAL int
inset(c, buf, len)
	char	c;
	Uchar	*buf;
	int	len;
#endif
{
	while (len-- > 0)
		if (c == *buf++)
			return (TRUE);
	return (FALSE);
}


LOCAL int
etoolarge(s)
	char	*s;
{
	comerrno(EX_BAD, "'%s' set too large.\n", s);
	/* NOTREACHED */
	return (0);
}

LOCAL const char *
filename(name)
	const char	*name;
{
	char	*p;

	if ((p = strrchr(name, '/')) == NULL)
		return (name);
	return (++p);
}
