/* @(#)compare.c	1.27 21/08/20 Copyright 1985, 88, 96-99, 2000-2021 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)compare.c	1.27 21/08/20 Copyright 1985, 88, 96-99, 2000-2021 J. Schilling";
#endif
/*
 *	compare two file for identical contents
 *	returns:
 *		0	files are the same
 *		1	file 1 is longer
 *		2 	file 2 is longer
 *		3	files differ before EOF is reached on either
 *		4	wrong number of arguments
 *		5	cannot open one of the files
 *		6	I/O error on one of the files
 *
 *	Copyright (c) 1985, 88, 96-99, 2000-2021 J. Schilling
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
#include <schily/types.h>
#include <schily/stat.h>
#include <schily/fcntl.h>	/* O_BINARY */
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/utypes.h>
#include <schily/standard.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include <schily/io.h>		/* for setmode() prototype */
#include <schily/nlsdefs.h>

char	*n1;
char	*n2;
int	silent	= 0;
int	allflg	= 0;
int	lineflg	= 0;

char	buf1[8*1024];
char	buf2[8*1024];

char *options =
	"help,version,s,silent,b&,begin&,b1&,begin1&,b2&,begin2&,c&,count&,all,a,long,l,lines,L";

LOCAL	void	usage	__PR((int exitcode));
EXPORT	int	main	__PR((int ac, char **av));
LOCAL	int	fsame	__PR((FILE *f1, FILE *f2));
LOCAL	void	skip	__PR((FILE * f, off_t pos));
LOCAL	void	compare	__PR((FILE * f1, FILE * f2, off_t pos1, off_t pos2,
				off_t count));
LOCAL	char	*printc	__PR((int c, char *bp));
LOCAL	void	prc	__PR((int c, char *bp));
LOCAL	void	prchar	__PR((int c, char *bp));
LOCAL	FILE	*ofile	__PR((char *s));
LOCAL	int	cntnl	__PR((const char *p, int size));

LOCAL void
usage(exitcode)
	int	exitcode;
{
	error("Usage:	compare [options] file1 [file2]\n");
	error("Options:\n");
	error("\t-silent \tbe silent\n");
	error("\tbegin=# \toffset for both files\n");
	error("\tbegin1=#\toffset for file1\n");
	error("\tbegin2=#\toffset for file2\n");
	error("\tcount=# \tcompare # bytes\n");
	error("\t-all,-a \tcompare to end of files\n");
	error("\t-long,-l\tcompare to end of files\n");
	error("\t-lines,-L\tcount lines while comparing (is slower)\n");
	error("\t-help\t\tPrint this help.\n");
	error("\t-version\tPrint version number.\n");
	error("If only one file is given it is compared to stdin\n");
	exit(exitcode);
}

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	FILE	*f1;
	FILE	*f2;
	int	help	= 0;
	int	prversion = 0;
	Llong	bpos	= 0;
	Llong	bpos1	= 0;
	Llong	bpos2	= 0;
	Llong	count	= 0;
	int	ex;
	int	cac;
	char	* const * cav;

	save_args(ac, av);

	(void) setlocale(LC_ALL, "");

#ifdef  USE_NLS
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "compare"	/* Use this only if it weren't */
#endif
	{ char	*dir;
	dir = searchfileinpath("share/locale", F_OK,
					SIP_ANY_FILE|SIP_NO_PATH, NULL);
	if (dir)
		(void) bindtextdomain(TEXT_DOMAIN, dir);
	else
#if defined(PROTOTYPES) && defined(INS_BASE)
	(void) bindtextdomain(TEXT_DOMAIN, INS_BASE "/share/locale");
#else
	(void) bindtextdomain(TEXT_DOMAIN, "/usr/share/locale");
#endif
	(void) textdomain(TEXT_DOMAIN);
	}
#endif 	/* USE_NLS */

	cac	= --ac;
	cav	= ++av;

	if (getallargs(&cac, &cav, options, &help, &prversion,
		&silent, &silent,
		getllnum, &bpos,
		getllnum, &bpos,
		getllnum, &bpos1,
		getllnum, &bpos1,
		getllnum, &bpos2,
		getllnum, &bpos2,
		getllnum, &count,
		getllnum, &count,
		&allflg, &allflg,
		&allflg, &allflg,
		&lineflg, &lineflg) < 0) {
		error("Bad flag: '%s'\n", cav[0]);
		usage(4);
	}
	if (help)
		usage(0);
	if (prversion) {
		gtprintf("Compare release %s %s (%s-%s-%s) Copyright (C) 1985, 88, 96-99, 2000-2021 %s\n",
				"1.27", "2021/08/20",
				HOST_CPU, HOST_VENDOR, HOST_OS,
				_("Jörg Schilling"));
		exit(0);
	}

	if (silent)
		allflg = FALSE;

	cac	= ac;
	cav	= av;
	if (getfiles(&cac, &cav, options) <= 0) {
		error("No files given.\n");
		usage(4);
	}
	n1 = cav[0];
	f1 = ofile(cav[0]);
	cac--, cav++;
	if (getfiles(&cac, &cav, options) <= 0) {
		n2 = "stdin";
		f2 = stdin;
		setmode(STDIN_FILENO, O_BINARY);
	} else {
		n2 = cav[0];
		f2 = ofile(cav[0]);
	}
	cac--, cav++;
	if (getfiles(&cac, &cav, options) > 0) {
		error("Too many files given.\n");
		usage(4);
	}
	cac--, cav++;
	file_raise(f1, FALSE);
	file_raise(f2, FALSE);
	setbuf(f1, 0);
	setbuf(f2, 0);
#ifdef	_FASCII		/* Mark Williamc C */
	f1->_ff &= ~_FASCII;
	f2->_ff &= ~_FASCII;
#endif
	/* XXX attention if we use bpos1 && bpos2 */
	if (bpos && bpos1 == 0)
		bpos1 = bpos;
	if (bpos && bpos2 == 0)
		bpos2 = bpos;
	ex = fsame(f1, f2);
	switch (ex) {

	case -1:
		/* Cannot stat try do run diff anyway */
		break;
	case 0:
		if (!silent)
			gtprintf("files are the same\n");
		exit(0);
		/* NOTREADCHED */

#ifdef	Length_is_more_important_than_content
	case 1:
	case 2:
		if (silent) {
			exit(ex);
			/* NOTREADCHED */
		}
#endif
	}
	if (bpos1)
		skip(f1, (off_t)bpos1);
	if (bpos2)
		skip(f2, (off_t)bpos2);
	compare(f1, f2, (off_t)bpos1, (off_t)bpos2, (off_t)count);
	/* NOTREADCHED */
	return (0);	/* Keep lint happy */
}

LOCAL int
fsame(f1, f2)
	FILE	*f1;
	FILE	*f2;
{
	struct	stat	sb1;
	struct	stat	sb2;

	if (filestat(f1, &sb1) < 0)
		return (-1);
	if (filestat(f2, &sb2) < 0)
		return (-1);

	if (sb1.st_ino == sb2.st_ino &&
	    sb1.st_dev == sb2.st_dev)
		return (0);		/* Files are the same */

	if (sb1.st_size > sb2.st_size)	/* File 1 is longer */
		return (1);
	if (sb1.st_size < sb2.st_size)	/* File 2 is longer */
		return (2);

	return (3);			/* Files may differ */
}

#undef	min
#define	min(a, b)	((a) > (b) ? (b) : (a))

LOCAL void
skip(f, pos)
	register FILE	*f;
	register off_t	pos;
{
	register off_t	i;
	register long	n;

	i = fileseek(f, pos);
	if (i >= (off_t)0)
		return;

	i = (off_t)0;
	while (i < pos) {
		n = ffileread(f, buf1, (int)min(sizeof (buf1), pos - i));
		if (n <= 0)
			break;
		i += n;
	}
}

LOCAL void
compare(f1, f2, pos1, pos2, count)
	FILE	*f1;
	FILE	*f2;
	off_t	pos1;
	off_t	pos2;
	off_t	count;
{
	register unsigned char	*p1 = NULL;
	register unsigned char	*p2 = NULL;
	register long	n;
	register long	l1	= 0;
	register long	l2	= 0;
		off_t	i	= (off_t)0;
		long	cnt;
		off_t	line	= (off_t)0;
		int	exitcode = 0;
		char	cb1[4];
		char	cb2[4];

	for (;;) {
		if (count && i >= count)
			break;
		if (l1 <= 0) {
			l1 = ffileread(f1, buf1, sizeof (buf1));
			p1 = (unsigned char *)buf1;
		}
		if (l2 <= 0) {
			l2 = ffileread(f2, buf2, sizeof (buf2));
			p2 = (unsigned char *)buf2;
		}
		if (l1 > 0 && l2 > 0) {
			cnt = min(l1, l2);
			if (count)
				cnt = min(cnt, count - i);
			n = cmpbytes(p1, p2, cnt);
			i += n;
			pos1 += n;
			pos2 += n;
			l1 -= n;
			l2 -= n;
			p1 += n;
			p2 += n;
			if (lineflg)
				line += cntnl((char *)p1, n);
			if (n >= cnt) {
				continue;
			}
		}
		if (l1 < 0 || ferror(f1)) {
			if (sizeof (pos1) > sizeof (long)) {
				errmsg("Error reading '%s', at %lld (0x%llx)\n",
						n1, (Llong)pos1, (Llong)pos1);
			} else {
				errmsg("Error reading '%s', at %ld (0x%lx)\n",
						n1, (long)pos1, (long)pos1);
			}
			exit(6);
		}
		if (l2 < 0 || ferror(f2)) {
			if (sizeof (pos1) > sizeof (long)) {
				errmsg("Error reading '%s', at %lld (0x%llx)\n",
						n2, (Llong)pos2, (Llong)pos2);
			} else {
				errmsg("Error reading '%s', at %ld (0x%lx)\n",
						n2, (long)pos2, (long)pos2);
			}
			exit(6);
		}
		if (l1 <= 0 || feof(f1)) {
			if (!feof(f2) && l2 > 0) {
				if (!silent) {
					if (sizeof (pos1) > sizeof (long)) {
						gtprintf("%s is longer than %s at %lld (0x%llx)\n",
							n2, n1, (Llong)pos1, (Llong)pos1);
					} else {
						gtprintf("%s is longer than %s at %ld (0x%lx)\n",
							n2, n1, (long)pos1, (long)pos1);
					}
				}
				if (!exitcode)
					exitcode = 2;
			}
			break;
		} else if (l2 <= 0 || feof(f2)) {
			if (!silent) {
				if (sizeof (pos1) > sizeof (long)) {
					gtprintf("%s is longer than %s at %lld (0x%llx)\n",
						n1, n2, (Llong)pos2, (Llong)pos2);
				} else {
					gtprintf("%s is longer than %s at %ld (0x%lx)\n",
						n1, n2, (long)pos2, (long)pos2);
				}
			}
			if (!exitcode)
				exitcode = 1;
			break;
		} else if (*p1 != *p2) {
			if (!silent) {
				if (lineflg) {
					if (sizeof (line) > sizeof (long)) {
						gtprintf("line: %lld ", (Llong)line);
					} else {
						gtprintf("line: %ld ", (long)line);
					}
				}
				if (!allflg)
					gtprintf("files differ at byte ");
				if (pos1 != pos2) {
				if (sizeof (pos1) > sizeof (long)) {
					printf("%6lld  (0x%06llx) / %6lld  (0x%06llx)\t0x%02x != 0x%02x%6s%6s",
						(Llong)pos1, (Llong)pos1,
						(Llong)pos2, (Llong)pos2,
						*p1, *p2,
						printc(*p1, cb1), printc(*p2, cb2));
				} else {
					printf("%6ld  (0x%06lx) / %6ld  (0x%06lx)\t0x%02x != 0x%02x%6s%6s",
						(long)pos1, (long)pos1,
						(long)pos2, (long)pos2,
						*p1, *p2,
						printc(*p1, cb1), printc(*p2, cb2));
				}
				} else {
				if (sizeof (pos1) > sizeof (long)) {
					printf("%6lld  (0x%06llx)\t0x%02x != 0x%02x%6s%6s",
						(Llong)pos1, (Llong)pos1, *p1, *p2,
						printc(*p1, cb1), printc(*p2, cb2));
				} else {
					printf("%6ld  (0x%06lx)\t0x%02x != 0x%02x%6s%6s",
						(long)pos1, (long)pos1, *p1, *p2,
						printc(*p1, cb1), printc(*p2, cb2));
				}
				}
				putchar('\n');
			}
			pos1++;
			pos2++;
			i++;
			l1--;
			l2--;
			p1++;
			p2++;
			exitcode = 3;
			if (!allflg)
				break;
		}
	}
	if (!exitcode && !silent)
		gtprintf("files are the same\n");
	exit(exitcode);
}

LOCAL char
*printc(c, bp)
	register int	c;
		char	*bp;
{
	prc(c, bp);
	return (bp);
}

LOCAL void
prc(c, bp)
	register int	c;
		char	*bp;
{
	if (c > 0177)
		*bp++ = '~';
	prchar(c & 0177, bp);
}

LOCAL void
prchar(c, bp)
	register int	c;
		char	*bp;
{
	if (c >= 040 && c != 0177) {
		*bp++	= c;
		*bp	= '\0';
		return;
	}
	*bp++	= '^';
	*bp++	= c ^ 0100;
	*bp	= '\0';
}

LOCAL FILE *
ofile(s)
	char	*s;
{
	FILE	*f;

	if ((f = fileopen(s, "rb")) == (FILE *) NULL) {
		if (!silent)
			errmsg("Can't open '%s'.\n", s);
		exit(5);
		/* NOTREACHED */
	}
	return (f);
}

#define	DO8(a)	a; a; a; a; a; a; a; a;

LOCAL int
cntnl(p, size)
	register const char	*p;
	register int		size;
{
	register int n	= 0;
	register char nl = '\n';

	while ((size -= 8) >= 0) {
		DO8(
			if (*p++ == nl)
				n++;
		);
	}
	size += 8;
	while (--size >= 0)
		if (*p++ == nl)
			n++;
	return (n);
}
