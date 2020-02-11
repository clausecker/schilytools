/* @(#)match.c	1.39 19/12/11 Copyright 1985-2019 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)match.c	1.39 19/12/11 Copyright 1985-2019 J. Schilling";
#endif
/*
 *	search file(s) for a pattern
 *
 *	Copyright (c) 1985-2019 J. Schilling
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
#include <schily/stdlib.h>
#include <schily/unistd.h>	/* Include sys/types.h to make off_t available */
#include <schily/string.h>
#include <schily/utypes.h>
#include <schily/patmatch.h>
#include <schily/standard.h>
#include <schily/ctype.h>
#include <schily/schily.h>

#define	UC	(unsigned char *)

#ifndef	HAVE_VALLOC
#define	valloc(a)	malloc(a)
#endif

#define	BUFSIZE	8192
#define	MAXLINE	8192

LOCAL	char	mchars[] = { ALT, REP, NIL, STAR, LBRACK, RBRACK,
			LCLASS, RCLASS, QUOTE, ANY, START, END,
			0,
};
LOCAL	char	notletter[] = "{^!$![^_A-Za-z0-9]}";

LOCAL	char	*buf;			/* buffer		*/
LOCAL	int	rblen;			/* read buffer len	*/
LOCAL	int	linelen;		/* line buffer len	*/
#define	rbuf	(&buf[linelen])		/* read buffer		*/
#define	line	(&buf[0])		/* line buffer		*/
LOCAL	char	*lcasebuf;		/* low case line buffer	*/

LOCAL	int	notflag = 0;
LOCAL	int	igncase = 0;
LOCAL	int	magic = 0;
LOCAL	int	nomagic = 0;
LOCAL	int	wordflag = 0;
LOCAL	int	xflag = 0;
LOCAL	int	cntflag = 0;
LOCAL	int	lflag = 0;
LOCAL	int	Lflag = 0;
LOCAL	int	Vflag = 0;
LOCAL	int	sflag = 0;
LOCAL	int	hflag = 0;
LOCAL	int	nflag = 0;
LOCAL	int	bflag = 0;
LOCAL	int	debug = 0;
LOCAL	int	dosimple = 0;

LOCAL	void	usage		__PR((int exitcode));
EXPORT	int	main		__PR((int ac, char **av));
LOCAL	int	domatch		__PR((FILE *file, char *name, char *pat, int plen, int *aux, int alt, int *state));
LOCAL	void	strlower	__PR((char *s, int slen));

LOCAL	BOOL	issimple	__PR((char *p));
LOCAL	int	smatch		__PR((char *linep, int llen, char *pat, int plen));
LOCAL	void	printpat	__PR((char *pat, int plen, int alt, int *aux));
LOCAL	BOOL	pmatch		__PR((char *linep, int llen, char *pat, int *aux, int alt, int *state));

LOCAL void
usage(exitcode)
	int	exitcode;
{
	error("Usage:	match [options] pattern [file1...filen]\n");
	error("Options:\n");
	error("	-not,-v	Print all lines that do not match\n");
	error("	-i	Ignore the case of letters\n");
	error("	-m	Force not to use the magic mode\n");
	error("	-M	Force to use the magic mode\n");
	error("	-w	Search for pattern as a word\n");
	error("	-x	Display only those lines which match exactly\n");
	error("	-c	Display matching count for each file\n");
	error("	-V	Display name of each file whith no matches\n");
	error("	-l	Display name of each file which matches\n");
	error("	-L	Display first matching line of each file which matches\n");
	error("	-s	Be silent indicate match in exitcode\n");
	error("	-h	Do not display filenames\n");
	error("	-n	Precede matching lines with line number\n");
	error("	-b	Precede matching lines with block number\n");
	error("	-help	Print this help.\n");
	error("	-version Print version number.\n");
	error("	Standard in is used if no files are specified.\n");
	exit(exitcode);
}

EXPORT int
main(ac, av)
	int ac;
	char **av;
{
	FILE *f;
	char *pat;
	int *aux   = NULL;
	int *state = NULL;
	int alt = 0;
	char *options = "not,v,V,i,M,m,w,x,c,l,L,s,h,n,b,help,version,d";
	int help = 0;
	int	cac		= ac;
	char	* const *cav	= av;
	char	*name;
	int	plen;
	int	matches;
	int	anymatch = 0;
	BOOL	prversion = 0;

	save_args(ac, av);

	if (getallargs(&cac, &cav, options,
			&notflag, &notflag,
			&Vflag,
			&igncase,
			&magic,
			&nomagic,
			&wordflag,
			&xflag,
			&cntflag, &lflag, &Lflag, &sflag,
			&hflag, &nflag, &bflag, &help, &prversion,
			&debug) < 0) {
		errmsgno(EX_BAD, "Bad flag: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (prversion) {
		printf("Match release %s (%s-%s-%s) Copyright (C) 1985-2019 Jörg Schilling\n",
				"1.39",
				HOST_CPU, HOST_VENDOR, HOST_OS);
		exit(0);
	}

	if (Vflag)
		sflag++;	/* Be silent while searching */

	cac = ac;
	cav = av;
	cac--, cav++;
	if (getfiles(&cac, &cav, options) <= 0) {
		errmsgno(EX_BAD, "No pattern given.\n");
		usage(EX_BAD);
	}
	pat = cav[0];
	cac--, cav++;

	plen = strlen(pat);
	if (magic)
		nomagic = 0;
	if (wordflag) {
		if (nomagic)
			comerrno(EX_BAD,
				"Cannot match words in nomagic mode.\n");
		plen += 2 * (sizeof (notletter) - 1);
		if ((name = malloc(plen+1)) == NULL)
			comerrno(EX_BAD, "No memory for pattern");
		strcatl(name, notletter, pat, notletter, (char *)NULL);
		pat = name;
	}
	if (igncase)
		strlower(pat, plen);
	if (nomagic || (!magic && issimple(pat))) {
		dosimple = TRUE;
	} else {
		aux = malloc(sizeof (int)*plen);
		state = malloc(sizeof (int)*(plen+1));
		if (aux == NULL || state == NULL)
			comerrno(EX_BAD, "No memory for pattern compiler.");

		if ((alt = patcompile(UC pat, plen, aux)) == 0)
			comerrno(EX_BAD, "Bad pattern: '%s'.\n", pat);
	}
	if (debug)
		printpat(pat, plen, alt, aux);

	while (rblen < BUFSIZE)
		rblen += getpagesize();
	while (linelen < MAXLINE)
		linelen += getpagesize();

	buf = valloc(linelen+rblen);
	lcasebuf = valloc(linelen);
	if (buf == NULL || lcasebuf == NULL)
		comerr("No memory for read buffer.\n");

	if (getfiles(&cac, &cav, options) <= 0) {	/* match stdin */
		name = "stdin";
		hflag++;
#ifdef	_FASCII		/* Mark Williams C 	*/
		stdin->_ff &= ~_FASCII;
#endif
		if ((matches = domatch(stdin, name, pat, plen, aux, alt, state)) != 0)
			anymatch++;
		if (cntflag)
			printf("%s:%d\n", name, matches);
		else if (Vflag && !matches)
			printf("%s\n", name);
		else if (lflag && matches)
			printf("%s\n", name);
	} else for (; getfiles(&cac, &cav, options); cac--, cav++) {
		name = cav[0];
		f = fileopen(name, "ru");
		if (f == NULL)
			errmsg("Cannot open '%s'.\n", name);
		else {
#ifdef	_FASCII		/* Mark Williams C 	*/
			f->_ff &= ~_FASCII;
#endif
			file_raise(f, FALSE);
			if ((matches = domatch(f, name, pat, plen, aux, alt, state)) != 0)
				anymatch++;
			fclose(f);
			if (cntflag)
				printf("%s:%d\n", name, matches);
			else if (Vflag && !matches)
				printf("%s\n", name);
			else if (lflag && matches)
				printf("%s\n", name);
		}
	}
	exit(anymatch ? 0 : 1);
	return (anymatch ? 0 : 1);	/* Keep lint happy */
}

/*
 * Search one file for a pattern.
 */
LOCAL int
domatch(f, name, pat, plen, aux, alt, state)
	register FILE *f;
	char *name;
	char *pat;
	register int plen;
	int *aux;
	int alt;
	int *state;
{
	register char *linep;		/* pointer to fill up line */
	register char *pbuf = rbuf;	/* pointer to read buffer */
	register int lbuf;		/* chars in read buffer */
	register int llen;
	register char c;		/* temp */
	off_t total = 0;		/* total number of bytes read */
	int lineno = 0;			/* current line number */
	int matches = 0;		/* current match count */
	BOOL matched = TRUE;		/* last line has match */
	BOOL eof = FALSE;
	int nl = 0;			/* line has nl */
	int r;

	lbuf = 0;
	for (;;) {
		if (!matched && !eof && nl == 0 && plen > 1) {
			/*
			 * If we are going to continue matching and the last
			 * match was for a long line (llen > linelen) then
			 * move the unmatched part of our line buffer to the
			 * beginning.
			 */
			linep = movebytes(line-plen+linelen+1, line, plen-1);
			llen = linelen+1-plen;
		} else {
			/*
			 * Start filling up a new line.
			 */
			linep = line;
			llen = linelen;
		}
		matched = FALSE;
		nl = 0;
		for (;;) {
			if (--lbuf < 0) {
				lbuf = ffileread(f, rbuf, rblen);
				if (lbuf < 0) {
					/*
					 * This may happen on NFS-mounted
					 * directories or OS that do not allow
					 * to read(2) directories, so we have
					 * to tolerate it.
					 */
					errmsg("Cannot read '%s'.\n", name);
					return (matches);
				}
				if (lbuf == 0) {	/* read hit EOF */
					eof = TRUE;
					if (linep != line)
						break;
					else
						return (matches);
				}
				pbuf = rbuf;
				total += lbuf;
				lbuf--;
			}
			if ((c = *pbuf++) == '\n') {
				nl = 1;
				lineno++;
				break;
			}
			if (--llen >= 0) {
				*linep++ = c;
			} else {
				lbuf++;
				pbuf--;
				break;
			}
		}
		/**plin = 0;*/
		llen = llen < 0 ? linelen : linelen - llen;

		if ((r = dosimple	? smatch(line, llen, pat, plen)
					: pmatch(line, llen, pat, aux, alt, state)) != 0) {
			if (notflag)
				continue;
		} else {
			if (!notflag)
				continue;
		}
		matches++;
		matched = TRUE;
		if (lflag)
			return (1);
		if (cntflag || sflag)
			continue;
		if (name && !hflag)
			printf("%s:", name);
		if (nflag)
			printf("%d:", lineno);
		if (bflag)
			printf("%lld:", (Llong)((total-lbuf-r-nl)/512));
		(void) filewrite(stdout, line, llen);
		putchar('\n');
		flush();
		if (Lflag)
			return (1);
	}
}

/*
 * Convert a string in place to lower case.
 */
LOCAL void
strlower(s, slen)
	register char	*s;
	register int	slen;
{
	register Uchar	c;

	while (--slen >= 0) {
		c = (Uchar)*s;
		if (isupper(c))
			*s = (char)tolower(c);
		s++;
	}
}

/*
 * Check whether the pattern only has non-magic chars.
 */
LOCAL BOOL
issimple(p)
	register char *p;
{
	while (*p) {
		if (strchr(mchars, *p++))
			return (FALSE);
	}
	return (TRUE);
}

/*
 * Simple (non regular expression) match.
 *
 * Check one line (or the buffer if no newline was found) for matches.
 */
LOCAL int
smatch(linep, llen, pat, plen)
	register char	*linep;
	register int	llen;
		char	*pat;
		int	plen;
{
	register char	*lp;		/* Line pointer		*/
	register char	*pp;		/* Pattern pointer	*/
	register char	*rpat = pat;
	register char	c = *pat;

	if (igncase) {
		movebytes(linep, lcasebuf, llen);
		strlower(linep = lcasebuf, llen);
	}
#ifdef	MDEBUG
	printf("llen0 %d %.*s\n", llen, llen, linep);
#endif
	if (xflag) {
		if (llen == 0)
			return (*rpat == '\0');
		if (llen != plen)
			return (0);
		for (lp = linep, pp = rpat; --llen >= 0; )
			if (*lp++ != *pp++)
				return (0);
		return (1);

		/* CSTYLED */
	} else for (llen -= plen-2; --llen > 0; ) {
#ifdef	MDEBUG
		printf("llen1 %d %.*s\n", llen, llen, linep);
#endif
		/*
		 * With a linelength of 16 and above, findbytes() is faster
		 */
		if (llen < 16) {
			while (llen > 0 && *linep != c) {
				linep++;
				llen--;
			}
			if (llen <= 0)
				return (0);
		} else {
			lp = findbytes(linep, llen, c);
			if (lp == NULL)
				return (0);
			llen -= lp - linep;
			linep = lp;
		}
#ifdef	MDEBUG
		printf("llen2 %d %.*s\n", llen, llen, linep);
#endif

		for (lp = linep++, pp = rpat; ; )
			if (*pp == 0)
				return (llen+plen);
#ifdef	__needed__
			else if (*lp == 0)
				return (0);
#endif
			else if (*pp++ != *lp++)
				break;
	}
	return (0);
}

LOCAL void
printpat(pat, plen, alt, aux)
	char	*pat;
	int	plen;
	int	alt;
	int	aux[];
{
	register int	i;

	printf("pattern: '%s'.\n", pat);
	printf("patlen : %d.\n", plen);
	if (!dosimple) {
		printf("alt    : %d.\n", alt);
		printf("aux    :");
		for (i = 0; i < plen; i++)
			printf(" %d", aux[i]);
		printf(".\n");
	}
}

/*
 * Pattern (using regular expressions) match.
 *
 * Check one line (or the buffer if no newline was found) for matches.
 */
LOCAL BOOL
pmatch(linep, llen, pat, aux, alt, state)
	char	*linep;
	int	llen;
	char	*pat;
	int	*aux;
	int	alt;
	int	*state;
{
	if (igncase) {
		movebytes(linep, lcasebuf, llen);
		strlower(linep = lcasebuf, llen);
	}
	if (xflag) {
		return (((long)((char *)patmatch(UC pat, aux, UC linep, 0, llen, alt, state) - linep))
								== llen);
	} else {
		return (patlmatch(UC pat, aux, UC linep, 0, llen, alt, state) != 0);
	}
}
