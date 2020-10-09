/* @(#)udiff.c	1.38 20/09/25 Copyright 1985-2020 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)udiff.c	1.38 20/09/25 Copyright 1985-2020 J. Schilling";
#endif
/*
 *	line by line diff for two files
 *
 *	Copyright (c) 1985-2020 J. Schilling
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
 *	It would be possible to have real large file mode in case that udiff is
 *	compiled in 64 bit mode.
 *	In 32 bit mode, there is no need to check for an integer overflow
 *	as the process will run "out of memory" before.
 */

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>	/* Include sys/types.h, makes off_t available */
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/stat.h>
#include <schily/schily.h>

#define	MAXLINE		32768

typedef struct line {
	off_t	off;
	long	hash;
} line;

LOCAL	char	*olbf;		/* "old" line buffer		*/
LOCAL	char	*nlbf;		/* "new" line buffer		*/
LOCAL	size_t	olsz;		/* "old" line buffer size	*/
LOCAL	size_t	nlsz;		/* "new" line buffer size	*/

LOCAL	BOOL	posix;		/* -posix flag */
LOCAL	int	nmatch = 2;
LOCAL	BOOL	bdiffmode = FALSE;

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
LOCAL	void	addline		__PR((char *s, size_t ll, off_t loff, long lc,
				    line *hline));
LOCAL	long	hash		__PR((char *s, size_t ll));
LOCAL	BOOL	linecmp		__PR((line *olp, line *nlp));
LOCAL	int	diff		__PR((void));
LOCAL	void	showdel		__PR((long o, long n, long c));
LOCAL	void	showadd		__PR((long o, long n, long c));
LOCAL	void	showchange	__PR((long o, long n, long c));
LOCAL	void	showxchange	__PR((long o, long n, long oc, long nc));

LOCAL	FILE	*xfileopen	__PR((int idx, char *name, char	*mode));
LOCAL	int	xfileseek	__PR((int idx, off_t off));
LOCAL	off_t	xfilepos	__PR((int idx));
LOCAL	ssize_t	xgetdelim	__PR((char **lineptr, size_t *n,
				    int delim, int idx));
LOCAL	ssize_t	xfileread	__PR((int idx, char **lineptr, size_t n));

LOCAL void
usage(exitcode)
	int	exitcode;
{
	error("Usage:	udiff [options] file1 file2\n");
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
	BOOL	prversion = FALSE;
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
	if (av0[0] == 'f' && av0[1] == 's') {
		bdiffmode = TRUE;
	}
	fac = --ac;
	fav = ++av;
	if (getallargs(&fac, &fav, options, &posix, &nmatch,
	    &help, &prversion) < 0) {
		errmsgno(EX_BAD, "Bad option: '%s'\n", fav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (nmatch <= 0) {
		errmsgno(EX_BAD, "Bad nmatch value: %d\n", nmatch);
		usage(EX_BAD);
	}
	if (prversion) {
		printf("Udiff release %s %s (%s-%s-%s) Copyright (C) 1985-2020 Jörg Schilling\n",
				"1.38", "2020/09/25",
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
	if ((of = xfileopen(0, fav[0], "rb")) == NULL)
		comerr("Cannot open file %s\n", fav[0]);
	fac--, fav++;

	if (getfiles(&fac, &fav, options) <= 0) {
		error("Only one file given.\n");
		usage(EX_BAD);
	}
	newname = fav[0];
	if ((nf = xfileopen(1, fav[0], "rb")) == NULL)
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
		char	*op = (char *)ob;
		char	*np = (char *)nb;
		int	n1;
		int	n2;

		while ((n1 = xfileread(0, &op, sizeof (ob))) > 0) {
			n2 = xfileread(1, &np, sizeof (nb));

			if (n1 != n2)
				goto notsame;
			if (cmpbytes(op, np, n1) < n1)
				goto notsame;
		}
		if (n1 < 0)
			goto notsame;
		goto same;
	}
notsame:
	if (!bdiffmode)
		ret = 1;
	if (xfileseek(0, (off_t)0) == (off_t)-1 ||
	    xfileseek(1, (off_t)0) == (off_t)-1)
		comerr("Cannot seek.\n");

	lineoff = readcommon(of, oldname, nf, newname);
	xfileseek(0, lineoff);
	xfileseek(1, lineoff);

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
		char	*op = (char *)ob;
		char	*np = (char *)nb;
		int	n;
		int	n1;
		int	n2;
		char	*oop;
		char	*nl;

		n1 = xfileread(0, &op, sizeof (ob));
		n2 = xfileread(1, &np, sizeof (nb));
		if (n1 <= 0 || n2 <= 0)
			break;
		if (n2 < n1)
			n1 = n2;
		n = n2 = cmpbytes(op, np, n1);

		/*
		 * Count newlines in common part.
		 */
		oop = op;
		while (n > 0 && (nl = findbytes(op, n, '\n'))) {
			lineoff = readoff + 1 + (nl - (char *)oop);
			n -= nl - op + 1;
			op = nl + 1;
			clc++;
		}
		if (n2 != n1)	/* cmpbytes() signalled a difference	*/
			break;	/* or n1 was less than n2		*/
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
	register ssize_t len;
	register int	lch = -1;	/* Last character from last read line */
		off_t	loff = 0;
		long	lc = 0;
		long	lsize = 0;
static	int		xgrow = 1024;
#define	MAX_GROW		16384

	/*
	 * Get current posision as we skipped over the common parts.
	 */
	loff = xfilepos(f == of ? 0:1);

	/*
	 * Use getdelim() to include the newline in the hash.
	 * This allows to correctly deal with files that do not end in a newline
	 * and this allows to handle nul bytes in the line.
	 */
	if (olsz == 0)		/* If first file is mmap()d and 2nd is not */
		olbf = NULL;	/* could be != NULL from mmap() space */
	for (;;) {
		if ((len = xgetdelim(&olbf, &olsz, '\n', f == of ? 0:1)) < 0) {
			if (ferror(f))
				comerr("Cannot read '%s'.\n", fname);
			break;
		}
		lch = ((unsigned char *)olbf)[len-1];
		if (lc >= lsize) {
			lsize += xgrow;
			*hlinep = ___realloc(*hlinep,
					(lsize) * sizeof (line),
					"new line");
			if (xgrow < MAX_GROW)
				xgrow *= 2;
		}
		addline(olbf, len, loff, lc++, *hlinep);
#ifdef	USE_CRLF
		loff = filepos(f);
#else	/* USE_CRLF */
		loff += len;
#endif	/* USE_CRLF */
	}
	if (lch >= 0 && lch != '\n') {
		error("Warning: missing newline at end of file %s\n", fname);
	}
	return (lc);
}

LOCAL void
addline(s, ll, loff, lc, hline)
	register char	*s;
	size_t	ll;
	off_t	loff;
	long	lc;
	line	*hline;
{
	line	*lp;

	lp = glinep(lc, hline);
	lp->off = loff;
	lp->hash = hash(s, ll);
}


LOCAL long
hash(s, ll)
	register char	*s;
	register size_t	ll;
{
	register long	h;

	for (h = 0; ll-- > 0; ) {
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
	ssize_t	lo;
	ssize_t	ln;

	if (olp->hash != nlp->hash)
		return (FALSE);

	if (ooff != olp->off) {
		xfileseek(0, olp->off);
		ooff = olp->off;
	}
	lo = xgetdelim(&olbf, &olsz, '\n', 0);
	if (lo < 0)
		return (FALSE);
	ooff += lo;
#ifdef	USE_CRLF
	ooff = filepos(of);
#endif

	if (noff != nlp->off) {
		xfileseek(1, nlp->off);
		noff = nlp->off;
	}
	ln = xgetdelim(&nlbf, &nlsz, '\n', 1);
	if (ln < 0)
		return (FALSE);
	noff += ln;
#ifdef	USE_CRLF
	ooff = filepos(nf);
#endif

	if (lo != ln)
		return (FALSE);
	return (cmpbytes(olbf, nlbf, lo) >= lo);
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
	ssize_t	lo;

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
			xfileseek(0, lp->off);
			ooff = lp->off;
		}
		lo = xgetdelim(&olbf, &olsz, '\n', 0);
		ooff += lo;
#ifdef	USE_CRLF
		ooff = filepos(of);
#endif
		if (posix)
			printf("< ");
		filewrite(stdout, olbf, lo);
		if (olbf[lo-1] !=  '\n')
			putchar('\n');
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
	ssize_t	ln;

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
			xfileseek(1, lp->off);
			noff = lp->off;
		}
		ln = xgetdelim(&nlbf, &nlsz, '\n', 1);
		noff += ln;
#ifdef	USE_CRLF
		noff = filepos(nf);
#endif
		if (posix)
			printf("> ");
		filewrite(stdout, nlbf, ln);
		if (nlbf[ln-1] !=  '\n')
			putchar('\n');
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
	ssize_t	lo;
	ssize_t	ln;

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
			xfileseek(0, lp->off);
			ooff = lp->off;
		}
		lo = xgetdelim(&olbf, &olsz, '\n', 0);
		ooff += lo;
#ifdef	USE_CRLF
		ooff = filepos(of);
#endif
		if (posix)
			printf("< ");
		filewrite(stdout, olbf, lo);
		if (olbf[lo-1] !=  '\n')
			putchar('\n');
	}
	if (posix)
		printf("---\n");
	else
		printf("-------- to:\n");
	for (i = 0; i < c; i++) {
		lp = glinep((long)(n+i), newfile);
		if (noff != lp->off) {
			xfileseek(1, lp->off);
			noff = lp->off;
		}
		ln = xgetdelim(&nlbf, &nlsz, '\n', 1);
		noff += ln;
#ifdef	USE_CRLF
		noff = filepos(nf);
#endif
		if (posix)
			printf("> ");
		filewrite(stdout, nlbf, ln);
		if (nlbf[ln-1] !=  '\n')
			putchar('\n');
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
	ssize_t	lo;
	ssize_t	ln;

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
			xfileseek(0, lp->off);
			ooff = lp->off;
		}
		lo = xgetdelim(&olbf, &olsz, '\n', 0);
		ooff += lo;
#ifdef	USE_CRLF
		ooff = filepos(of);
#endif
		if (posix)
			printf("< ");
		filewrite(stdout, olbf, lo);
		if (olbf[lo-1] !=  '\n')
			putchar('\n');
	}
	if (posix)
		printf("---\n");
	else
		printf("-------- to:\n");
	for (i = 0; i < nc; i++) {
		lp = glinep((long)(n+i), newfile);
		if (noff != lp->off) {
			xfileseek(1, lp->off);
			noff = lp->off;
		}
		ln = xgetdelim(&nlbf, &nlsz, '\n', 1);
		noff += ln;
#ifdef	USE_CRLF
		noff = filepos(nf);
#endif
		if (posix)
			printf("> ");
		filewrite(stdout, nlbf, ln);
		if (nlbf[ln-1] !=  '\n')
			putchar('\n');
	}
}

#include <schily/mman.h>
#include <schily/errno.h>

LOCAL	FILE	*xf[2];
LOCAL	off_t	mmsize[2];
LOCAL	char	*mmbase[2];
LOCAL	char	*mmend[2];
LOCAL	char	*mmnext[2];

LOCAL FILE *
xfileopen(idx, name, mode)
	int	idx;
	char	*name;
	char	*mode;
{
	FILE	*f;
	struct stat	sb;

	f = fileopen(name, mode);
	if (f == NULL)
		return (NULL);
	xf[idx] = f;

#ifdef	HAVE_MMAP
	if (fstat(fileno(f), &sb) < 0) {
		fclose(f);
		return (NULL);
	}
	mmsize[idx] = sb.st_size;
	if (sb.st_size > (64*1024*1024))
		return (f);

	if (!S_ISREG(sb.st_mode))	/* FIFO has st_size == 0 */
		return (f);		/* so cannot use mmap()  */

	if (sb.st_size == 0)
		mmbase[idx] = "";
	else
		mmbase[idx] = mmap((void *)0, mmsize[idx],
			PROT_READ, MAP_PRIVATE, fileno(f), (off_t)0);

	if (mmbase[idx] == MAP_FAILED) {
		/*
		 * Silently fall back to the read method.
		 */
		mmbase[idx] = NULL;
	} else if (mmbase[idx]) {
		mmnext[idx] = mmbase[idx];
		mmend[idx] = mmbase[idx] + mmsize[idx];
	}
#endif
	return (f);
}

LOCAL int
xfileseek(idx, off)
	int	idx;
	off_t	off;
{
#ifndef	HAVE_MMAP
	return (fileseek(xf[idx], off));
#else
	if (mmbase[idx] == NULL)
		return (fileseek(xf[idx], off));

	mmnext[idx] = mmbase[idx] + off;

	if (mmnext[idx] > (mmbase[idx] + mmsize[idx])) {
		mmnext[idx] = (mmbase[idx] + mmsize[idx]);
		seterrno(EINVAL);
		return (-1);
	}
	if (mmnext[idx] < mmbase[idx]) {
		mmnext[idx] = mmbase[idx];
		seterrno(EINVAL);
		return (-1);
	}
	return (0);
#endif
}

LOCAL off_t
xfilepos(idx)
	int	idx;
{
#ifndef	HAVE_MMAP
	return (filepos(xf[idx]));
#else
	if (mmbase[idx] == NULL)
		return (filepos(xf[idx]));

	return (mmnext[idx] - mmbase[idx]);
#endif
}

LOCAL ssize_t
xgetdelim(lineptr, n, delim, idx)
	char	**lineptr;
	size_t	*n;
	int	delim;
	int	idx;
{
#ifndef	HAVE_MMAP
	return (getdelim(lineptr, n, delim, xf[idx]));
#else
	char	*p;
	ssize_t	siz;
	ssize_t	amt;

	if (mmbase[idx] == NULL)
		return (getdelim(lineptr, n, delim, xf[idx]));

	*lineptr = mmnext[idx];
	siz = mmend[idx] - mmnext[idx];
	if (siz == 0) {
		*lineptr = "";
		return (-1);
	}
	p = findbytes(mmnext[idx], siz, delim);
	if (p == NULL) {
		amt = siz;
		mmnext[idx] = mmend[idx];
	} else {
		amt = ++p - mmnext[idx];
		mmnext[idx] = p;
	}
	if (amt == 0) {
		*lineptr = "";
		return (-1);
	}
	return (amt);
#endif
}

LOCAL ssize_t
xfileread(idx, lineptr, n)
	int	idx;
	char	**lineptr;
	size_t	n;
{
#ifndef	HAVE_MMAP
	return (fileread(xf[idx], *lineptr, n));
#else
	ssize_t	siz;
	ssize_t	amt;

	if (mmbase[idx] == NULL)
		return (fileread(xf[idx], *lineptr, n));

	*lineptr = mmnext[idx];
	siz = mmend[idx] - mmnext[idx];
	amt = n;
	if (amt > siz) {
		amt = siz;
		mmnext[idx] = mmend[idx];
	} else {
		mmnext[idx] += amt;
	}
	if (amt == 0)
		*lineptr = "";
	return (amt);
#endif
}
