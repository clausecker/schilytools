/* @(#)count.c	1.24 09/07/11 Copyright 1986-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)count.c	1.24 09/07/11 Copyright 1986-2009 J. Schilling";
#endif
/*
 *	count words, lines, and/or chars in files
 *
 *	Copyright (c) 1986-2009 J. Schilling
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
#include <schily/schily.h>

#define	iswhite(c)	(c == ' ' || c == '\t' || c == '\n')

int	tabstop	= 1;
/*
 * Make it long long as sums may be always more than 2 GB
 */
Llong	tchars	= (Llong)0;
Llong	twords	= (Llong)0;
Llong	tlines	= (Llong)0;
Llong	tllen	= (Llong)0;
char	flags[]	= "lines,l,words,w,chars,c,llen,ll,stat,s,total,t,tab#,help,version";
int	cflg	= 0;
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
	error("	-chars	Count characters\n");
	error("	-llen	Count max linelen\n");
	error("	-stat	Stat file for character count\n");
	error("	tab=#	Set tabsize to # (default 1)\n");
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

	save_args(ac, av);
	cac = --ac;
	cav = ++av;

	if (getallargs(&cac, &cav, flags,
			&lflg, &lflg,
			&wflg, &wflg,
			&cflg, &cflg,
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
		printf("Count release %s (%s-%s-%s) Copyright (C) 1986-2009 Jörg Schilling\n",
				"1.24",
				HOST_CPU, HOST_VENDOR, HOST_OS);
		exit(0);
	}
	if (!(lflg || wflg || cflg || llflg)) {
		lflg++;
		wflg++;
		cflg++;
		llflg++;
	}
	if (sflg) {
		lflg = 0;
		wflg = 0;
		cflg++;
		llflg = 0;
	}
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
		if (llflg)
			p(tllen);
		printf(" total\n");
	}
	exit(0);
	return (0);	/* Keep lint happy */
}

LOCAL void
count(f)
	register FILE *f;
{
	register int	c;
	register BOOL	inword = FALSE;
	register off_t	hpos = (off_t)0;
	register off_t	chars	= (off_t)0;
	register off_t	words	= (off_t)0;
	register off_t	lines 	= (off_t)0;
	register off_t	llen	= (off_t)0;

	file_raise(f, FALSE);

	if (sflg && statfile(f))
		return;

	if (wflg == 0 && llflg == 0) {
		while ((c = getc(f)) != EOF) {
			if (c == '\n') {
				lines++;
			}
			chars++;
		}
	} else while ((c = getc(f)) != EOF) {
		if (iswhite(c)) {
			if (c == '\n') {
				if (hpos > llen)
					llen = hpos;
				chars += hpos+1;
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

	chars += hpos;
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
		if (llflg)
			p((Llong)llen);
		printf(" %s\n", filename);
	}
	tchars += chars;
	twords += words;
	tlines += lines;
	if (llen > tllen)
		tllen = llen;
	chars = (off_t)0;
	lines = 0;
	words = 0;
	nfiles++;
}

LOCAL void
phead()
{
	head++;
	if (lflg)
		printf("%8s", "lines");
	if (wflg)
		printf("%8s", "words");
	if (cflg)
		printf("%8s", "chars");
	if (llflg)
		printf("%8s", "linelen");
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
