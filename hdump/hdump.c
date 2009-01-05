/* @(#)hdump.c	1.20 08/12/23 Copyright 1986-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)hdump.c	1.20 08/12/23 Copyright 1986-2008 J. Schilling";
#endif
/*
 *	hex dump for files
 *
 *	Copyright (c) 1986-2008 J. Schilling
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
#include <schily/unistd.h>	/* Include sys/types.h to make off_t available */
#include <schily/fcntl.h>	/* O_BINARY */
#include <schily/utypes.h>
#include <schily/schily.h>

#ifdef	NEED_O_BINARY
#include <io.h>					/* for setmode() prototype */
#endif

#define	octdig(x)	(x >= '0' && x <= '7')
#ifndef	TRUE
#define	TRUE		1
#define	FALSE		0
#endif
int	curradix;
int	lradix = 16;
char	*llfmt;
char	*lfmt;
off_t	pos = (off_t)0;
BOOL	dflag = FALSE;
BOOL	oflag = FALSE;
BOOL	aflag = FALSE;
BOOL	bflag = FALSE;
BOOL	cflag = FALSE;
BOOL	fflag = FALSE;
BOOL	Fflag = FALSE;
BOOL	lflag = FALSE;
BOOL	uflag = FALSE;
BOOL	lenflag = FALSE;
BOOL	vflag = FALSE;
char	buffer[BUFSIZ];	/* buffer for standard out */

LOCAL	void	usage	__PR((int exitcode));
EXPORT	int	main	__PR((int ac, char **av));
LOCAL	void	dump	__PR((FILE * f, char *name, off_t len));
LOCAL	void	prbuf	__PR((int cnt, char *obuf));
LOCAL	void	prbytes	__PR((int cnt, char *buf));
LOCAL	void	prascii	__PR((int cnt, char *buf));
LOCAL	void	prlong	__PR((int cnt, long *obuf));
LOCAL	void	prshort	__PR((int cnt, short *obuf));
#ifndef	NO_FLOATINGPOINT
LOCAL	void	prdouble __PR((int cnt, double *obuf));
#endif
LOCAL	BOOL	bufeql	__PR((long *b1, long *b2));
LOCAL	Llong	myatoll	__PR((char *s));

LOCAL void
usage(exitcode)
	int	exitcode;
{
	error("Usage:	hdump [options] [file [starting address[.][b|B] [count]]]\n");
	error("Options:\n");
	error("\t-a\tDisplay content also in characters\n");
	error("\t-b\tDisplay content in bytes\n");
	error("\t-c\tDisplay content in quoted characters\n");
	error("\t-d\tDisplay content in decimal\n");
#ifndef	NO_FLOATINGPOINT
	error("\t-f\tDisplay content longs as floats\n");
	error("\t-F\tDisplay content double longs as doubles\n");
#endif
	error("\t-l\tDisplay content as longs\n");
	error("\t-o\tDisplay content in octal\n");
	error("\t-u\tDisplay content as unsigned\n");
	error("\t-v\tShow all data even if it is identical\n");
	error("\t-help\tPrint this help.\n");
	error("\t-version\tPrint version number.\n");
	error("Address label radix depends on starting address radix (decimal if ends with .)\n");
	error("'b' after starting address multiplies with 512 'B' with 1024\n");
	exit(exitcode);
}

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	off_t	len = (off_t)0;
	FILE	*infile;
	char	*inname;
	char	*options = "a,b,c,d,f,F,l,o,u,v,help,version";
	BOOL	help = FALSE;
	BOOL	prversion = FALSE;
	int	cac;
	char	* const * cav;

	save_args(ac, av);
	cac = --ac;
	cav = ++av;

	if (getallargs(&cac, &cav, options,
			&aflag, &bflag, &cflag, &dflag, &fflag, &Fflag,
			&lflag, &oflag, &uflag, &vflag,
						&help, &prversion) < 0) {
		error("Bad flag: '%s'\n", cav[0]);
		usage(1);
	}
	if (help)
		usage(0);
	if (prversion) {
		printf("Hdump release %s (%s-%s-%s) Copyright (C) 1986-2008 Jörg Schilling\n",
				"1.20",
				HOST_CPU, HOST_VENDOR, HOST_OS);
		exit(0);
	}

	setbuf(stdout, buffer);

	cac = ac;
	cav = av;
	if (getfiles(&cac, &cav, options) <= 0) {
		infile = stdin;
		inname = "stdin";
#ifdef	NEED_O_BINARY
		setmode(STDIN_FILENO, O_BINARY);
#endif
	} else {
		inname = cav[0];
		if (inname[0] == '-' && inname[1] == '\0') {
			infile = stdin;
			inname = "stdin";
#ifdef	NEED_O_BINARY
			setmode(STDIN_FILENO, O_BINARY);
#endif
		} else {
			infile = fileopen(cav[0], "rb");
		}
		if (infile == 0)
			comerr("Can't open '%s'.\n", cav[0]);
		file_raise(infile, FALSE);
		cac--, cav++;
	}
#ifdef	_FASCII		/* Mark Williams C */
	infile->_ff &= ~_FASCII;
#endif
	if (getfiles(&cac, &cav, options) > 0) {
		(void) fileseek(infile, pos = (off_t)myatoll(cav[0]) & ~((off_t)1));
		lradix = curradix;
		cac--, cav++;
	}
	if (getfiles(&cac, &cav, options) > 0) {
		len = (off_t)myatoll(cav[0]);
		lenflag = TRUE;
		cac--; cav++;
	}

	if (getfiles(&cac, &cav, options) > 0) {
		errmsgno(EX_BAD, "Unexpected argument '%s'.\n", cav[0]);
		usage(1);
	}
	llfmt = lradix == 8 ? "%06llo: " : (lradix == 10 ? "%6lld: " : "%6llx: ");
	lfmt  = lradix == 8 ? "%06lo: " : (lradix == 10 ? "%6ld: " : "%6lx: ");
	dump(infile, inname, len);
	return (0);
}

LOCAL void
dump(f, name, len)
		FILE	*f;
		char	*name;
	register off_t	len;
{
	long	obuf1[16 / sizeof (long)];
	long	obuf2[16 / sizeof (long)];
	register long	*buf = obuf1;
	register long	*oldbuf = obuf2;
	register long	*temp;
	register int	cnt;

	do {
		if (lenflag && len <= 0)
			return;
		cnt = fileread(f, (char *)buf, (int)(lenflag?(len > 16 ? 16:len):16));
		if (cnt < 0)
			comerr("Error reading '%s'.\n", name);
		if (vflag || !bufeql((long *)buf, (long *)oldbuf) || cnt < 16)
			prbuf(cnt, (char *)buf);
		pos += cnt;
		len -= cnt;

		temp = oldbuf;
		oldbuf = buf;
		buf = temp;

	} while (cnt > 0);
/*	printf("%6X: \n", pos);*/
	if (sizeof (pos) > sizeof (long))
		printf(llfmt, pos);
	else
		printf(lfmt, pos);
	printf("\n");
}

LOCAL void
prbuf(cnt, obuf)
		int	cnt;
	register char	*obuf;
{
	if (cnt <= 0)
		return;
/*	printf("%6X: ", pos);*/
	if (sizeof (pos) > sizeof (long))
		printf(llfmt, pos);
	else
		printf(lfmt, pos);

	if (aflag)
		prascii(cnt, (char *) obuf);
	else if (bflag | cflag)
		prbytes(cnt, (char *) obuf);
#ifndef	NO_FLOATINGPOINT
	else if (Fflag)
		prdouble(cnt, (double *)obuf);
#endif
	else if (lflag || fflag)
		prlong(cnt, (long *)obuf);
	else
		prshort(cnt, (short *)obuf);
}

LOCAL void
prbytes(cnt, buf)
	register int	cnt;
	register char	buf[16];
{
	register short i;

	if (cflag) {
		for (i = 0; i < cnt; i++) {
			if (buf[i] < ' ' || buf[i] >= '\177')
				printf("  \\%02X", 0377&buf[i]);
			else
				printf("    %c", buf[i]);
			if (i == 7 && cnt > 8)
				printf("\n        ");
		}
		putchar('\n');
	}
	if (cflag & bflag)
		printf("        ");
	if (bflag) {
		for (i = 0; i < cnt; i++) {
			if (dflag) {
				if (uflag)
					printf("  %4d", 0377&buf[i]);
				else
					printf("  %4d", buf[i]);
			} else if (oflag) {
				printf(" %04o", 0377&buf[i]);
			} else {
				printf("   %02X", 0377&buf[i]);
			}
			if (i == 7 && cnt > 8)
				printf("\n        ");
		}
		putchar('\n');
	}
}

LOCAL void
prascii(cnt, buf)
	register int	cnt;
	register char	*buf;
{
	register short	i;
	register char	c;

	for (i = 0; i < 16; i++) {
		if (i >= cnt)
			printf("   ");
		else if (dflag) {
			if (uflag)
				printf("%4u", 0377&buf[i]);
			else
				printf("%4d", buf[i]);
		} else if (oflag) {
			printf(" %03o", 0377&buf[i]);
		} else {
			printf(" %02X", 0377&buf[i]);
		}
		if (i == 7)
			printf("  ");
	}
	if (dflag || oflag)
		printf("\n         ");
	else
		printf("   ");
	for (i = 0; i < cnt; i++) {
		c = buf[i];
		putchar(c < ' ' || c >= 0177 ? '.' : c);
	}
	putchar('\n');
}

LOCAL void
prlong(cnt, obuf)
		int	cnt;
	register long	obuf[4];
{
	register short	i;
	register int	n = cnt;
#ifndef	NO_FLOATINGPOINT
	register float	*fp;
#endif

	n /= sizeof (long);
	for (i = 0; i < n; i++) {
last:
		if (dflag) {
			if (uflag)
				printf("%12lu", obuf[i]);
			else
				printf("%12ld", obuf[i]);
#ifndef	NO_FLOATINGPOINT
		} else if (fflag) {
			fp = (float *)&obuf[i];
			printf("%14.7e", *fp);
#endif
		} else if (oflag) {
			printf(" %012lo", obuf[i]);
		} else {
			printf("   %08lX", obuf[i]);
		}
	}
	if ((i = (cnt % sizeof (long))) != 0) {
		fillbytes(&((char *)obuf)[cnt], sizeof (long)-i, '\0');
		cnt -= i;
		i = cnt / sizeof (long);
		goto last;
	}
	putchar('\n');
}

LOCAL void
prshort(cnt, obuf)
		int	cnt;
	register short	obuf[8];
{
	register short	i;
	register int	n = cnt;

	n /= sizeof (short);
	for (i = 0; i < n; i++) {
last:
		if (dflag) {
			if (uflag)
				printf("%7hu", obuf[i]);
			else
				printf("%7hd", obuf[i]);
		} else if (oflag) {
			printf(" %06ho", obuf[i]);
		} else {
			printf("   %04hX", obuf[i]);
		}
	}
	if ((i = (cnt % sizeof (short))) != 0) {
		fillbytes(&((char *)obuf)[cnt], sizeof (short)-i, '\0');
		cnt -= i;
		i = cnt / sizeof (short);
		goto last;
	}
	putchar('\n');
}

#ifndef	NO_FLOATINGPOINT
LOCAL void
prdouble(cnt, obuf)
		int	cnt;
	register double	obuf[2];
{
	register short	i;
	register int	n = cnt;

	n /= sizeof (double);
	for (i = 0; i < n; i++) {
last:
		printf("%21.14e", obuf[i]);
	}
	if ((i = (cnt % sizeof (double))) != 0) {
		fillbytes(&((char *)obuf)[cnt], sizeof (double)-i, '\0');
		cnt -= i;
		i = cnt / sizeof (double);
		goto last;
	}
	putchar('\n');
}
#endif

LOCAL BOOL
bufeql(b1, b2)
	register long	*b1;
	register long	*b2;
{
	register int	i;
	static	BOOL	dont_print;

	for (i = 16 / sizeof (long); --i >= 0; )
		if (*b1++ != *b2++)
			return (dont_print = FALSE);
	if (!dont_print) {
		printf("     *\n");
		fflush(stdout);
	}
	return (dont_print = TRUE);
}

LOCAL Llong
myatoll(s)
	char	*s;
{
	char	*p;
	Llong	val = 0;

	if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
		curradix = 16;
	else if (s[0] == '0' && !streql("0", s))
		curradix = 8;
	else
		curradix = 0;

	p = astoll(s, &val);

	if (*p != '\0') {
		if (*p == '.') {
			p++;
			curradix = 10;
		}
		if (*p && (streql(p, "b") || streql(p, "B"))) {
			if (*p == 'b') val *= 512;
			if (*p == 'B') val *= 1024;
		} else if (*p)
			comerrno(EX_BAD, "Bad numeric argument '%s'.\n", s);
	}

	return (val);
}
