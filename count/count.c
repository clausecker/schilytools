/* @(#)count.c	1.31 21/08/20 Copyright 1986-2021 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)count.c	1.31 21/08/20 Copyright 1986-2021 J. Schilling";
#endif
/*
 *	count words, lines, and/or chars in files
 *
 *	Copyright (c) 1986-2021 J. Schilling
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
#include <schily/utypes.h>
#include <schily/standard.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include <schily/nlsdefs.h>
#include <schily/limits.h>	/* for  MB_LEN_MAX	*/
#include <schily/ctype.h>	/* For isprint()	*/
#include <schily/wchar.h>	/* wchar_t		*/
#include <schily/wctype.h>	/* For iswprint()	*/

#define	iswhite(c)	(c == ' ' || c == '\t' || c == '\n')

#define	TABSTOP	8
int	tabstop	= TABSTOP;
/*
 * Make it long long as sums may be always more than 2 GB
 */
Llong	tchars	= (Llong)0;
Llong	tmchars	= (Llong)0;
Llong	twords	= (Llong)0;
Llong	tlines	= (Llong)0;
Llong	tllen	= (Llong)0;
char	flags[]	= "lines,l,words,w,chars,c,mchars,m,C,llen,ll,stat,s,total,t,tab#,help,version";
int	cflg	= 0;
int	mflg	= 0;
int	wflg	= 0;
int	lflg	= 0;
int	llflg	= 0;
int	sflg	= 0;
int	head	= 0;
int	totflg	= 0;
int	nfiles	= 0;
int	help	= 0;
int	prversion = 0;
char	*filename = 0;	/* current file name */

LOCAL	void	usage	__PR((int exitcode));
EXPORT	int	main	__PR((int ac, char **av));
LOCAL	void	count	__PR((FILE * f));
LOCAL	void	phead	__PR((void));
LOCAL	void	p	__PR((Llong val));
LOCAL	int	statfile __PR((FILE * f));

LOCAL void
usage(exitcode)
	int	exitcode;
{
	error("Usage:	count [options] file1...filen\n");
	error("Options:\n");
	error("	-lines	Count lines\n");
	error("	-words	Count words\n");
	error("	-chars	Count characters based on bytes\n");
	error("	-mchars	Count multi byte characters\n");
	error("	-llen	Count max linelen\n");
	error("	-stat	Stat file for character count\n");
	error("	tab=#	Set tabsize to # (default %d)\n", TABSTOP);
	error("	-total	Print only grand total\n");
	error("	-help	Print this help.\n");
	error("	-version Print version number.\n");
	exit(exitcode);
}

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	int	cac;
	char	* const *cav;
	FILE	*f;
#if	defined(USE_NLS)
	char	*dir;
#endif

	save_args(ac, av);

	(void) setlocale(LC_ALL, "");

#if	defined(USE_NLS)
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "count"	/* Use this only if it weren't */
#endif
	dir = searchfileinpath("share/locale", F_OK,
					SIP_ANY_FILE|SIP_NO_PATH, NULL);
	if (dir)
		(void) bindtextdomain(TEXT_DOMAIN, dir);
	else
#if	!defined(INS_BASE)
#define	INS_BASE	"/usr"
#endif
#ifdef	PROTOTYPES
	(void) bindtextdomain(TEXT_DOMAIN, INS_BASE "/share/locale");
#else
	(void) bindtextdomain(TEXT_DOMAIN, "/usr/share/locale");
#endif
	(void) textdomain(TEXT_DOMAIN);
#endif

	cac = --ac;
	cav = ++av;

	if (getallargs(&cac, &cav, flags,
			&lflg, &lflg,
			&wflg, &wflg,
			&cflg, &cflg,
			&mflg, &mflg, &mflg,
			&llflg, &llflg,
			&sflg, &sflg,
			&totflg, &totflg, &tabstop,
			&help, &prversion) < 0) {
		errmsgno(EX_BAD, "Bad Option: '%s'.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (prversion) {
		gtprintf(
		"Count release %s %s (%s-%s-%s) Copyright (C) 1986-2021 %s\n",
				"1.31", "2021/08/20",
				HOST_CPU, HOST_VENDOR, HOST_OS,
				_("Jörg Schilling"));
		exit(0);
	}
	if (!(lflg || wflg || cflg || mflg || llflg)) {
		lflg++;
		wflg++;
		cflg++;
		llflg++;
	}
	if (sflg) {
		lflg = 0;
		wflg = 0;
		cflg++;
		mflg = 0;
		llflg = 0;
	}
#if	MB_LEN_MAX == 1
	if (mflg && !cflg)
		cflg++;
	mflg = 0;
#endif
	cac = ac;
	cav = av;
	if (getfiles(&cac, &cav, flags) == 0) {
		filename = "stdin";
		count(stdin);
	} else do {
		filename = cav[0];
		if (streql(filename, "-")) {
			filename = "stdin";
			count(stdin);
		} else {
			if ((f = fileopen(filename, "r")) == NULL)
				errmsg("Can't open '%s'.\n", filename);
			else
				count(f);
		}
		cav++;
		cac--;
	} while (getfiles(&cac, &cav, flags) > 0);

	if (nfiles > 1 || totflg) {
		if (!head)
			phead();
		if (lflg)
			p(tlines);
		if (wflg)
			p(twords);
		if (cflg)
			p(tchars);
		if (mflg)
			p(tmchars);
		if (llflg)
			p(tllen);
		printf(_(" total\n"));
	}
	exit(0);
	return (0);	/* Keep lint happy */
}

LOCAL void
count(f)
	register FILE *f;
{
	register wint_t	c;
	register BOOL	inword	= FALSE;
	register off_t	hpos	= (off_t)0;
	register off_t	chars	= (off_t)0;
	register off_t	words	= (off_t)0;
	register off_t	lines 	= (off_t)0;
	register off_t	llen	= (off_t)0;
	register off_t	mchars	= (off_t)0;
#if	MB_LEN_MAX > 1
		char	mb[MB_LEN_MAX+1];
	register size_t	nmb;
		int	mlen;
		wchar_t	wc;
#endif

	file_raise(f, FALSE);

#ifdef	HAVE_SETVBUF
	setvbuf(f, NULL, _IOFBF, 32*1024);
#endif

	if (sflg && statfile(f))
		return;

	if (mflg) {
#if	MB_LEN_MAX > 1
		nmb = 0;
		if (wflg == 0 && llflg == 0) {
			while ((c = getc(f)) != EOF) {
				mb[nmb++] = c;
				if ((mlen = mbtowc(&wc, mb, nmb)) < 0) {
					(void) mbtowc(NULL, NULL, 0);
					if (nmb < MB_LEN_MAX)
						continue;
					wc = mb[0] & 0xFF;
					chars++;
					mchars++;
					mb[nmb] = '\0';
					nmb -= 1;
					ovstrcpy(mb, &mb[1]);
					continue;
				} else {
					if (mlen == 0)
						mlen++;
					chars += mlen;
					mchars++;
					if (nmb > mlen) {
						mb[nmb] = '\0';
						nmb -= mlen;
						ovstrcpy(mb, &mb[mlen]);
					} else {
						nmb = 0;
					}
				}
				if (c == '\n') {
					lines++;
				}
			}
		} else while ((c = getc(f)) != EOF) {
			mb[nmb++] = c;
			if ((mlen = mbtowc(&wc, mb, nmb)) < 0) {
				(void) mbtowc(NULL, NULL, 0);
				if (nmb < MB_LEN_MAX)
					continue;
				wc = mb[0] & 0xFF;
				chars++;
				mchars++;
				mb[nmb] = '\0';
				nmb -= 1;
				ovstrcpy(mb, &mb[1]);
				continue;
			} else {
				if (mlen == 0)
					mlen++;
				chars += mlen;
				mchars++;
				if (nmb > mlen) {
					mb[nmb] = '\0';
					nmb -= mlen;
					ovstrcpy(mb, &mb[mlen]);
				} else {
					nmb = 0;
				}
			}

			if (iswhite(wc)) {
				if (wc == '\n') {
					if (hpos > llen)
						llen = hpos;
					hpos = 0;
					lines++;
				} else if (wc == '\t') {
					hpos = (hpos / tabstop) * tabstop + tabstop;
				} else {
					hpos++;
				}
				if (inword)
					words++;
				inword = FALSE;
			} else {
				hpos++;
				inword = TRUE;
			}
		}
		chars += nmb;
		mchars += nmb;
#endif
	} else
	if (wflg == 0 && llflg == 0) {
		while ((c = getc(f)) != EOF) {
			if (c == '\n') {
				lines++;
			}
			chars++;
		}
	} else while ((c = getc(f)) != EOF) {
		chars++;
		if (iswhite(c)) {
			if (c == '\n') {
				if (hpos > llen)
					llen = hpos;
				hpos = 0;
				lines++;
			} else if (c == '\t') {
				hpos = (hpos / tabstop) * tabstop + tabstop;
			} else {
				hpos++;
			}
			if (inword)
				words++;
			inword = FALSE;
		} else {
			hpos++;
			inword = TRUE;
		}
	}
	if (c == EOF && ferror(f))
		errmsg("I/O error on '%s'.\n", filename);

	if (hpos > llen)
		llen = hpos;
	if (f != stdin)
		fclose(f);
	if (!totflg) {
		if (!head)
			phead();
		if (lflg)
			p((Llong)lines);
		if (wflg)
			p((Llong)words);
		if (cflg)
			p((Llong)chars);
		if (mflg)
			p((Llong)mchars);
		if (llflg)
			p((Llong)llen);
		printf(" %s\n", filename);
	}
	tchars += chars;
	tmchars += mchars;
	twords += words;
	tlines += lines;
	if (llen > tllen)
		tllen = llen;
	chars = (off_t)0;
	mchars = (off_t)0;
	lines = 0;
	words = 0;
	nfiles++;
}

LOCAL void
phead()
{
	head++;
	if (lflg)
		printf("%8s", _("lines"));
	if (wflg)
		printf("%8s", _("words"));
	if (cflg)
		printf("%8s", _("chars"));
	if (mflg)
		printf("%8s", _("mchars"));
	if (llflg)
		printf("%8s", _("linelen"));
	printf("\n");
}

LOCAL void
p(val)
	Llong	val;
{
	if (sizeof (val) > sizeof (long))
		printf(" %7lld", val);
	else
		printf(" %7ld", (long)val);
}

#include <schily/stat.h>

LOCAL int
statfile(f)
	FILE	*f;
{
	struct	stat	sb;

	if (fstat(fileno(f), &sb) < 0) {
		errmsg("Can't stat '%s'.\n", filename);
		return (0);
	}
	if ((sb.st_mode & S_IFMT) != S_IFREG)
		return (0);

	if (f != stdin)
		fclose(f);

	if (!totflg) {
		if (!head)
			phead();
		p(sb.st_size);
		printf(" %s\n", filename);
	}
	tchars += sb.st_size;
	nfiles++;
	return (1);
}
