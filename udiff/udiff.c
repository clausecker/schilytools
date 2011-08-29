/* @(#)udiff.c	1.28 11/08/13 Copyright 1985-2011 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)udiff.c	1.28 11/08/13 Copyright 1985-2011 J. Schilling";
#endif
/*
 *	line by line diff for two files
 *
 *	Copyright (c) 1985-2011 J. Schilling
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

/*
 *	Remarks:
 *
 *	The amount of memory "in use" largely depends on the allocation
 *	algorithm as the code intensively uses realloc().
 *
 *	If the files differ, we always allocate
 *	(sizeof (off_t) + sizeof (long)) * (lines(old) + lines(new))
 *	In largefile mode we typically allocate ~ 50% more
 *	than in non-largefile mode. It seems that in largefile mode, the
 *	amount of space need is typically ~
 *		(sizeof (long long) + sizeof (long)) *
 *		(lines(old) + lines(new)).
 *
 *	Largefile mode is also neeeded in order to be able to deal with files
 *	in the 64 bit inode # range on ZFS.
 *
 *	If the code currently uses "long" for line numbers, which is sufficient
 *	It would be possible to have real large file mode in case that diff is
 *	compiled in 64 bit mode.
 *	In 32 bit mode, there is no need to check for an integer overflow
 *	as the process will run "out of memory" before.
 */

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>	/* Include sys/types.h to make off_t available */
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/stat.h>
#include <schily/schily.h>

#define	MAXLINE		32768

typedef struct line {
	off_t	off;
	long	hash;
} line;

LOCAL	BOOL	posix;		/* -posix flag */
LOCAL	int	nmatch = 2;

LOCAL	line	*oldfile;	/* Offsets and hashes for old file */
LOCAL	char	*oldname;	/* Old file name */
LOCAL	FILE	*of = 0;	/* File pointer for old file */
LOCAL	long	olc = 0;	/* Line count for old file */
LOCAL	line	*newfile;	/* Offsets and hashes for new file */
LOCAL	char	*newname;	/* New file name */
LOCAL	FILE	*nf = 0;	/* File pointer for new file */
LOCAL	long	nlc = 0;	/* Line count for new file */
LOCAL	long	clc = 0;	/* Common line count */
LOCAL	off_t	ooff;		/* Saved seek offset for old file */
LOCAL	off_t	noff;		/* Saved seek offset for new file */

#define	glinep(i, a)	(&((a)[(i)]))
#define	compare(o, n)	(linecmp(glinep((o), oldfile), glinep((n), newfile)))

LOCAL	void	usage		__PR((int exitcode));
EXPORT	int	main		__PR((int ac, char **av));
LOCAL	const char *filename	__PR((const char *name));
LOCAL	off_t	readcommon	__PR((FILE *ofp, char *oname,
				    FILE *nfp, char *nname));
LOCAL	long	readfile	__PR((FILE *f, char *fname, line **hline));
LOCAL	void	addline		__PR((char *s, off_t loff, long lc, line *hline));
LOCAL	long	hash		__PR((char *s));
LOCAL	BOOL	linecmp		__PR((line *olp, line *nlp));
LOCAL	int	diff		__PR((void));
LOCAL	void	showdel		__PR((long o, long n, long c));
LOCAL	void	showadd		__PR((long o, long n, long c));
LOCAL	void	showchange	__PR((long o, long n, long c));
LOCAL	void	showxchange	__PR((long o, long n, long oc, long nc));


LOCAL void
usage(exitcode)
	int	exitcode;
{
	error("Usage:	diff [options] file1 file2\n");
	error("	-help	Print this help.\n");
	error("	-version Print version number.\n");
	error("	-posix	Print diffs in POSIX mode.\n");
	error("	nmatch=# Set number of matching lines for resync (default = %d).\n",
		nmatch);
	exit(exitcode);
}


EXPORT int
main(ac, av)
	int	ac;
	char	**av;
{
	char	*options = "posix,nmatch#,help,version";
	BOOL	help = FALSE;
	BOOL prversion = FALSE;
	int	ret = 0;
	int	fac;
	char	* const *fav;
	const char *av0 = filename(av[0]);
	struct stat sb1;
	struct stat sb2;
	off_t	lineoff;

	save_args(ac, av);
	if (av0[0] != 'u') {
		nmatch = 1;
		posix = 1;
	}
	fac = --ac;
	fav = ++av;
	if (getallargs(&fac, &fav, options, &posix, &nmatch,
	    &help, &prversion) < 0) {
		errmsgno(EX_BAD, "Bad option: '%s'\n", av[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (nmatch <= 0) {
		errmsgno(EX_BAD, "Bad nmatch value: '%s'\n", av[0]);
		usage(EX_BAD);
	}
	if (prversion) {
		printf("Udiff release %s (%s-%s-%s) Copyright (C) 1985-2011 Jörg Schilling\n",
				"1.28",
				HOST_CPU, HOST_VENDOR, HOST_OS);
		exit(0);
	}

	fac = ac;
	fav = av;
	if (getfiles(&fac, &fav, options) <= 0) {
		error("No files given.\n");
		usage(EX_BAD);
	}
	oldname = fav[0];
	if ((of = fileopen(fav[0], "r")) == NULL)
		comerr("Cannot open file %s\n", fav[0]);
	fac--, fav++;

	if (getfiles(&fac, &fav, options) <= 0) {
		error("Only one file given.\n");
		usage(EX_BAD);
	}
	newname = fav[0];
	if ((nf = fileopen(fav[0], "r")) == NULL)
		comerr("Cannot open file %s\n", fav[0]);
	fac--, fav++;

	if (getfiles(&fac, &fav, options) > 0) {
		error("Too many files given.\n");
		usage(EX_BAD);
	}
	fac--, fav++;

	if (filestat(of, &sb1) < 0)
		comerr("Cannot stat '%s'\n", oldname);
	if (filestat(nf, &sb2) < 0)
		comerr("Cannot stat '%s'\n", newname);
	if (sb1.st_ino == sb2.st_ino && sb1.st_dev == sb2.st_dev)
		goto same;

#ifdef	HAVE_SETVBUF
	setvbuf(of, NULL, _IOFBF, MAXLINE);
	setvbuf(nf, NULL, _IOFBF, MAXLINE);
#endif
	if (sb1.st_size == sb2.st_size) {
		long	ob[MAXLINE / sizeof (long)];
		long	nb[MAXLINE / sizeof (long)];
		int	n1;
		int	n2;

		while ((n1 = fileread(of, ob, sizeof (ob))) > 0) {
			n2 = fileread(nf, nb, sizeof (nb));

			if (n1 != n2)
				goto notsame;
			if (cmpbytes(ob, nb, n1) < n1)
				goto notsame;
		}
		if (n1 < 0)
			goto notsame;
		goto same;
	}
notsame:
	ret = 1;
	if (fileseek(of, (off_t)0) == (off_t)-1 ||
	    fileseek(nf, (off_t)0) == (off_t)-1)
		comerr("Cannot seek.\n");

	lineoff = readcommon(of, oldname, nf, newname);
	fileseek(of, lineoff);
	fileseek(nf, lineoff);

	olc = readfile(of, oldname, &oldfile);
	nlc = readfile(nf, newname, &newfile);

	(void) diff();
same:
	fclose(of);
	fclose(nf);
	return (ret);
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

LOCAL off_t
readcommon(ofp, oname, nfp, nname)
	FILE	*ofp;
	char	*oname;
	FILE	*nfp;
	char	*nname;
{
	off_t	lineoff = 0;
	off_t	readoff = 0;

	clc = 0;
	for (;;) {
		long	ob[MAXLINE / sizeof (long)];
		long	nb[MAXLINE / sizeof (long)];
		int	n;
		int	n1;
		int	n2;
		char	*op;
		char	*nl;

		n1 = fileread(ofp, ob, sizeof (ob));
		n2 = fileread(nfp, nb, sizeof (nb));
		if (n1 <= 0 || n2 <= 0)
			break;
		if (n2 < n1)
			n1 = n2;
		n = n2 = cmpbytes(ob, nb, n1);

		op = (char *)ob;
		while (n > 0 && (nl = findbytes(op, n, '\n'))) {
			lineoff = readoff + 1 + (nl - (char *)ob);
			n -= nl - op + 1;
			op = nl + 1;
			clc++;
		}
		if (n2 < n1)
			break;
		readoff += n2;
	}
	return (lineoff);
}

LOCAL long
readfile(f, fname, hlinep)
	register FILE	*f;
		char	*fname;
		line	**hlinep;
{
		char	lbuf[MAXLINE];
	register char	*rb = lbuf;
	register int	len;
	register int	maxlen = sizeof (lbuf);
		off_t	loff = 0;
		long	lc = 0;

	/*
	 * Get current posision as we skipped over the common parts.
	 */
	loff = filepos(f);

	/*
	 * Use fgetstr() to include the newline in the hash.
	 * This allows to correctly deal with files that do not end in a newline
	 */
	for (;;) {
		if ((len = fgetstr(f, rb, maxlen)) < 0) {
			if (ferror(f))
				comerr("Cannot read '%s'.\n", fname);
			break;
		}
		if (lc % 1024 == 0)
			*hlinep = ___realloc(*hlinep,
					(lc + 1024) * sizeof (line),
					"new line");
		addline(rb, loff, lc++, *hlinep);
#ifdef	USE_CRLF
		loff = filepos(f);
#else	/* USE_CRLF */
#ifndef	tos
		if (len < maxlen)
			loff += len;
		else
#endif
			loff = filepos(f);
#endif	/* USE_CRLF */
	}
	return (lc);
}

LOCAL void
addline(s, loff, lc, hline)
	register char	*s;
	off_t	loff;
	long	lc;
	line	*hline;
{
	line	*lp;

	lp = glinep(lc, hline);
	lp->off = loff;
	lp->hash = hash(s);
}


LOCAL long
hash(s)
	register char	*s;
{
	register long	h;

	for (h = 0; *s; ) {
		if (h < 0) {
			h <<= 1;
			h += (*s++ & 255)+1;	/* rotate sign bit */
		} else {
			h <<= 1;
			h += (*s++ & 255);
		}
	}
	return (h);
}


LOCAL BOOL
linecmp(olp, nlp)
	line	*olp;
	line	*nlp;
{
	long	olb[MAXLINE / sizeof (long)];
	long	nlb[MAXLINE / sizeof (long)];

	if (olp->hash != nlp->hash)
		return (FALSE);

	if (ooff != olp->off) {
		fileseek(of, olp->off);
		ooff = olp->off;
	}
	ooff += 1 +
		fgetline(of, (char *)olb, sizeof (olb));
#ifdef	USE_CRLF
	ooff = filepos(of);
#endif

	if (noff != nlp->off) {
		fileseek(nf, nlp->off);
		noff = nlp->off;
	}
	noff += 1 +
		fgetline(nf, (char *)nlb, sizeof (nlb));
#ifdef	USE_CRLF
	ooff = filepos(nf);
#endif

	return (streql((char *)olb, (char *)nlb));
}


LOCAL int
diff()
{
	register long	oln;
	register long	nln;
	register long	k;
	register long	l;
	register long	i;
	register long	j;
		long	mx;
		BOOL	b;
		BOOL	m;
		int	ret = 0;

	ooff = noff = -1;
	oln = 0, nln = 0;
	while (oln < olc && nln < nlc) {
		if (compare(oln, nln)) {
			oln++;
			nln++;
			continue;	/* Nothing changed */
		}

		ret = 1;
		mx = (olc-oln)+(nlc-nln);
		m = FALSE;
		for (k = 1; k < mx; k++)
		    for (l = 0; l <= k; l++) {
			if (oln+l >= olc)
				break;
			if (nln+k-l >= nlc) {
				l = nln+k - nlc;
				continue;
			}
			if (compare((long)(oln+l), (long)(nln+k-l))) {
				for (j = 1, b = FALSE;
					(j < nmatch) &&
					(oln+l+j < olc) && (nln+k-l+j < nlc);
								j++) {
					if (!compare((long)(oln+l+j), (long)(nln+k-l+j))) {
						b = TRUE;
						break;
					}
				}
				if (!b) {
					if (l == 0)
						showadd(oln, nln, k);
					else if (k-l == 0)
						showdel(oln, nln, l);
					else if (l == k-l)
						showchange(oln, nln, l);
					else
						showxchange(oln, nln, l, (long)(k-l));
					oln += l;
					nln += k-l;
					m = TRUE;
					goto out;
				}
			}
		}
	    out:
		if (!m)
			break;
	}
	i = olc-oln;
	j = nlc-nln;

	if (i == 0 && j == 0)
		return (ret);
	else if (i == j)
		showchange(oln, nln, i);
	else if (j && i)
		showxchange(oln, nln, i, j);
	else if (i)
		showdel(oln, nln, i);
	else if (j)
		showadd(oln, nln, j);
	ret = 1;
	return (ret);
}


LOCAL void
showdel(o, n, c)
	long	o;
	long	n;
	long	c;
{
	long	i;
	line	*lp;
	char	lbuf[MAXLINE];

	o += clc;
	n += clc;
	if (posix) {
		if (c == 1)
			printf("%ldd%ld\n", o+1, n);
		else
			printf("%ld,%ldd%ld\n", o+1, o+c, n);
	} else if (c == 1)
		printf("\n-------- 1 line deleted at %ld:\n", o);
	else
		printf("\n-------- %ld lines deleted at %ld:\n", c, o);
	o -= clc;
	n -= clc;

	for (i = 0; i < c; i++) {
		lp = glinep((long)(o+i), oldfile);
		if (ooff != lp->off) {
			fileseek(of, lp->off);
			ooff = lp->off;
		}
		ooff += 1 +
		fgetline(of, lbuf, sizeof (lbuf));
#ifdef	USE_CRLF
		ooff = filepos(of);
#endif
		if (posix)
			printf("< ");
		printf("%s\n", lbuf);
	}
}


LOCAL void
showadd(o, n, c)
	long	o;
	long 	n;
	long	c;
{
	long	i;
	line	*lp;
	char	lbuf[MAXLINE];

	o += clc;
	n += clc;
	if (posix) {
		if (c == 1)
			printf("%lda%ld\n", o, n+1);
		else
			printf("%lda%ld,%ld\n", o, n+1, n+c);
	} else if (c == 1)
		printf("\n-------- 1 line added at %ld:\n", o);
	else
		printf("\n-------- %ld lines added at %ld:\n", c, o);
	o -= clc;
	n -= clc;

	for (i = 0; i < c; i++) {
		lp = glinep((long)(n+i), newfile);
		if (noff != lp->off) {
			fileseek(nf, lp->off);
			noff = lp->off;
		}
		noff += 1 +
		fgetline(nf, lbuf, sizeof (lbuf));
#ifdef	USE_CRLF
		noff = filepos(nf);
#endif
		if (posix)
			printf("> ");
		printf("%s\n", lbuf);
	}
}


LOCAL void
showchange(o, n, c)
	long	o;
	long	n;
	long	c;
{
	long	i;
	line	*lp;
	char	lbuf[MAXLINE];

	o += clc;
	n += clc;
	if (posix) {
		if (c == 1)
			printf("%ldc%ld\n", o+1, n+1);
		else
			printf("%ld,%ldc%ld,%ld\n", o+1, o+c, n+1, n+c);
	} else if (c == 1)
		printf("\n-------- 1 line changed at %ld from:\n", o);
	else
		printf("\n-------- %ld lines changed at %ld-%ld from:\n",
							c, o, o+c-1);
	o -= clc;
	n -= clc;

	for (i = 0; i < c; i++) {
		lp = glinep((long)(o+i), oldfile);
		if (ooff != lp->off) {
			fileseek(of, lp->off);
			ooff = lp->off;
		}
		ooff += 1 +
		fgetline(of, lbuf, sizeof (lbuf));
#ifdef	USE_CRLF
		ooff = filepos(of);
#endif
		if (posix)
			printf("< ");
		printf("%s\n", lbuf);
	}
	if (posix)
		printf("---\n");
	else
		printf("-------- to:\n");
	for (i = 0; i < c; i++) {
		lp = glinep((long)(n+i), newfile);
		if (noff != lp->off) {
			fileseek(nf, lp->off);
			noff = lp->off;
		}
		noff += 1 +
		fgetline(nf, lbuf, sizeof (lbuf));
#ifdef	USE_CRLF
		noff = filepos(nf);
#endif
		if (posix)
			printf("> ");
		printf("%s\n", lbuf);
	}
}


LOCAL void
showxchange(o, n, oc, nc)
	long	o;
	long	n;
	long	oc;
	long	nc;
{
	long	i;
	line	*lp;
	char	lbuf[MAXLINE];

	o += clc;
	n += clc;
	if (posix) {
		if (oc == 1)
			printf("%ldc%ld,%ld\n", o+1, n+1, n+nc);
		else if (nc == 1)
			printf("%ld,%ldc%ld\n", o+1, o+oc, n+1);
		else
			printf("%ld,%ldc%ld,%ld\n", o+1, o+oc, n+1, n+nc);
	} else if (oc == 1)
		printf("\n-------- 1 line changed to %ld lines at %ld from:\n",
							nc, o);
	else if (nc == 1)
		printf("\n-------- %ld lines changed to 1 line at %ld-%ld from:\n",
							oc, o, o+oc-1);
	else
		printf("\n-------- %ld lines changed to %ld lines at %ld-%ld from:\n",
							oc, nc, o, o+oc-1);
	o -= clc;
	n -= clc;

	for (i = 0; i < oc; i++) {
		lp = glinep((long)(o+i), oldfile);
		if (ooff != lp->off) {
			fileseek(of, lp->off);
			ooff = lp->off;
		}
		ooff += 1 +
		fgetline(of, lbuf, sizeof (lbuf));
#ifdef	USE_CRLF
		ooff = filepos(of);
#endif
		if (posix)
			printf("< ");
		printf("%s\n", lbuf);
	}
	if (posix)
		printf("---\n");
	else
		printf("-------- to:\n");
	for (i = 0; i < nc; i++) {
		lp = glinep((long)(n+i), newfile);
		if (noff != lp->off) {
			fileseek(nf, lp->off);
			noff = lp->off;
		}
		noff += 1 +
		fgetline(nf, lbuf, sizeof (lbuf));
#ifdef	USE_CRLF
		noff = filepos(nf);
#endif
		if (posix)
			printf("> ");
		printf("%s\n", lbuf);
	}
}
