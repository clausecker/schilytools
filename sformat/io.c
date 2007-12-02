/* @(#)io.c	1.25 06/09/13 Copyright 1988-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)io.c	1.25 06/09/13 Copyright 1988-2004 J. Schilling";
#endif
/*
 *	Copyright (c) 1988-2004 J. Schilling
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
#include <schily/varargs.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/utypes.h>
#include <schily/schily.h>
#include <ctype.h>

#include "fmt.h"

LOCAL BOOL	cvt_blocks __PR((char *, long *, long, long, struct disk *));
LOCAL void	prt_std	   __PR((char *, long, long, long, struct disk *));
LOCAL void	prt_blocks __PR((char *, long, long, long, struct disk *));

EXPORT char *
skipwhite(s)
		const char	*s;
{
	register const Uchar	*p = (const Uchar *)s;

	while (*p) {
		if (!isspace(*p))
			break;
		p++;
	}
	return ((char *)p);
}

/* ARGSUSED */
EXPORT BOOL
cvt_std(linep, lp, mini, maxi, dp)
	char	*linep;
	long	*lp;
	long	mini;
	long	maxi;
	struct disk	*dp;
{
	long	l	= -1L;

/*	printf("cvt_std(\"%s\", %d, %d, %d);\n", linep, *lp, mini, maxi);*/

	if (linep[0] == '?') {
		printf("Enter a number in the range from %ld to %ld\n",
								mini, maxi);
		printf("The default radix is 10\n");
		printf("Precede number with '0x' for hexadecimal or with '0' for octal\n");
		printf("Shorthands are:\n");
		printf("\t'^' for minimum value (%ld)\n", mini);
		printf("\t'$' for maximum value (%ld)\n", maxi);
		printf("\t'+' for incrementing value to %ld\n", *lp + 1);
		printf("\t'-' for decrementing value to %ld\n", *lp - 1);
		return (FALSE);
	}
	if (linep[0] == '^' && *skipwhite(&linep[1]) == '\0') {
		l = mini;
	} else if (linep[0] == '$' && *skipwhite(&linep[1]) == '\0') {
		l = maxi;
	} else if (linep[0] == '+' && *skipwhite(&linep[1]) == '\0') {
		if (*lp < maxi)
			l = *lp + 1;
	} else if (linep[0] == '-' && *skipwhite(&linep[1]) == '\0') {
		if (*lp > mini)
			l = *lp - 1;
	} else if (*astol(linep, &l)) {
		printf("Not a number: '%s'.\n", linep);
		return (FALSE);
	}
	if (l < mini || l > maxi) {
		printf("'%s' is out of range.\n", linep);
		return (FALSE);
	}
	*lp = l;
	return (TRUE);
}

LOCAL BOOL
cvt_blocks(linep, lp, mini, maxi, dp)
	char	*linep;
	long	*lp;
	long	mini;
	long	maxi;
	struct disk	*dp;
{
	long	l;
	long	cyls	= 0L;
	long	heads	= 0L;
	long	secs	= 0L;
	char	*hp;
	char	*sp;

	if (linep[0] == '?') {
		printf("To enter blocks as simple block count:\n");
		(void) cvt_std(linep, lp, mini, maxi, dp);
		printf("\tNOTE: '$' will fill this partition to the end of the disk.\n");
		printf("To enter blocks as mega bytes:\n");
		printf("\tEnter size in megabytes as floating point number.\n");
		printf("\t(must be directly followed by 'm' or 'M' e.g. 12.5M)\n");
		printf("To enter blocks as cyls/tracks/sectors:\n");
		printf("\tEnter a number in the form cyls/tracks/secs or cyls/tracks or cyls/\n");
		(void) cvt_bcyls(linep, lp, mini, maxi, dp);
		return (FALSE);
	}
	/*
	 * If line contains no slash
	 * and no character which tells us to use megabytes
	 * convert as blocknumber.
	 */
	if (linep[0] != '>' &&
			!strchr(linep, '/') &&
			!strchr(linep, '.') &&
			!strchr(linep, 'm') && !strchr(linep, 'M'))
		return (cvt_std(linep, lp, mini, maxi, dp));

	sp = hp = strchr(linep, '/');
	if (hp) {
		/*
		 * It's a cyl/track/sec notation.
		 */
		*hp++ = '\0';
		if (!(sp = strchr(hp, '/'))) {
			sp = &hp[-1];
		} else {
			*sp++ = '\0';
		}

		if (linep[0] && !cvt_std(linep, &cyls, 0L, dp->lncyl, dp))
			return (FALSE);
		if (hp[0] && !cvt_std(hp, &heads, 0L, dp->lhead-1L, dp))
			return (FALSE);
		if (sp[0] && !cvt_std(sp, &secs, 0L, dp->lspt-1L, dp))
			return (FALSE);

		l = secs + heads * dp->lspt +
			cyls * (dp->lhead * dp->lspt);

		if (l < mini || l > maxi) {
			printf("'%ld' is out of range.\n", l);
			return (FALSE);
		}
	} else if (linep[0] == '>') {
		/*
		 * It's ---> reach to the next partition's beginning.
		 */
		return (cvt_bcyls(linep, lp, mini, maxi, dp));
	} else {
		int a;
		double d;
/*
 * atof() should be in stdlib.h. If there is no stdlib.h, schily/stdlib.h has the def
 */
/*		extern	double atof();*/
		/*
		 * It's megabytes
		 */
		d = atof(linep);
		d *= 2048.0;	/* Now we have 512 Byte sectors */
		l = d;

		/*
		 * Round up to next cylinder.
		 */
		a = l / (dp->lhead * dp->lspt);
		if (l % (dp->lhead * dp->lspt)) {
			l = ++a * (dp->lhead * dp->lspt);
		}
	}
	*lp = l;
	return (TRUE);
}

/* ARGSUSED */
LOCAL void
prt_std(s, l, mini, maxi, dp)
	char	*s;
	long	l;
	long	mini;
	long	maxi;
	struct disk *dp;
{
	printf("%s %ld (%ld - %ld)/<cr>:", s, l, mini, maxi);
}

LOCAL void
prt_blocks(s, l, mini, maxi, dp)
	char	*s;
	long	l;
	long	mini;
	long	maxi;
	struct disk	*dp;
{
	long	cyl;
	long	head;
	long	sec;

	cyl = l / (dp->lhead*dp->lspt);
	head = (l - (cyl * dp->lhead * dp->lspt)) / dp->lspt;
	sec = l - (cyl * dp->lhead * dp->lspt)
		- (head * dp->lspt);

	printf("%s %ld, %ld/%ld/%ld (%ld - %ld)/<cr>:", s, l, cyl, head, sec,
								mini, maxi);
}

EXPORT BOOL
getvalue(s, lp, mini, maxi, prt, cvt, dp)
	char	*s;
	long	*lp;
	long	mini;
	long	maxi;
	void	(*prt) __PR((char *, long, long, long, struct disk *));
	BOOL	(*cvt) __PR((char *, long *, long, long, struct disk *));
	struct disk	*dp;
{
	char	line[128];
	char	*linep;

	for (;;) {
		(*prt)(s, *lp, mini, maxi, dp);
		flush();
		line[0] = '\0';
		if (getline(line, 80) == EOF)
			exit(EX_BAD);

		linep = skipwhite(line);
		/*
		 * Nicht initialisierte Variablen
		 * duerfen nicht uebernommen werden
		 */
		if (linep[0] == '\0' && *lp != -1L)
			return (FALSE);

		if (strlen(linep) == 0) {
			/* Leere Eingabe */
		} else if ((*cvt)(linep, lp, mini, maxi, dp))
			return (TRUE);
	}
	/* NOTREACHED */
}

EXPORT BOOL
getlong(s, lp, mini, maxi)
	char	*s;
	long	*lp;
	long	mini;
	long	maxi;
{
	return (getvalue(s, lp, mini, maxi, prt_std, cvt_std, (void *)0));
}

EXPORT BOOL
getint(s, ip, mini, maxi)
	char	*s;
	int	*ip;
	int	mini;
	int	maxi;
{
	long	l = *ip;
	BOOL	ret;

	ret = getlong(s, &l, (long)mini, (long)maxi);
	*ip = l;
	return (ret);
}

EXPORT BOOL
getdiskcyls(s, lp, mini, maxi)
	char	*s;
	long	*lp;
	long	mini;
	long	maxi;
{
	return (getvalue(s, lp, mini, maxi, prt_std, cvt_cyls, (void *)0));
}

EXPORT BOOL
getdiskblocks(s, lp, mini, maxi, dp)
	char	*s;
	long	*lp;
	long	mini;
	long	maxi;
	struct disk	*dp;
{
	return (getvalue(s, lp, mini, maxi, prt_blocks, cvt_blocks, dp));
}

/* VARARGS1 */
EXPORT BOOL
#ifdef	PROTOTYPES
yes(char *form, ...)
#else
yes(form, va_alist)
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	char okbuf[10];

again:
#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	printf("%r", form, args);
	va_end(args);
	flush();
	if (getline(okbuf, sizeof (okbuf)) == EOF)
		exit(EX_BAD);
	if (okbuf[0] == '?') {
		printf("Enter 'y', 'Y', 'yes' or 'YES' if you agree with the previous asked question.\n");
		printf("All other input will be handled as if the question has beed answered with 'no'.\n");
		goto again;
	}
	if (streql(okbuf, "y") || streql(okbuf, "yes") ||
	    streql(okbuf, "Y") || streql(okbuf, "YES"))
		return (TRUE);
	else
		return (FALSE);
}

void
prbytes(s, cp, n)
		char		*s;
	register unsigned char	*cp;
	register int		n;
{
	printf(s);
	while (--n >= 0)
		printf(" %02X", *cp++);
	printf("\n");
}

EXPORT char *
permstring(s)
	const char	*s;
{
	char	*p;

	if ((p = malloc(strlen(s)+1)) != NULL)
		strcpy(p, s);
	return (p);
}

EXPORT struct strval *
strval(val, sp)
	int	val;
	struct strval *sp;
{
	while (sp->s_name) {
		if (val == sp->s_val)
			return (sp);
		sp++;
	}
	return ((struct strval *)0);
}

EXPORT struct strval *
namestrval(name, sp)
	const char	*name;
	struct strval	*sp;
{
	while (sp->s_name) {
		if (strcmp(name, sp->s_name) == 0)
			return (sp);
		sp++;
	}
	return ((struct strval *)0);
}

EXPORT BOOL
getstrval(s, lp, sp, deflt)
	const char	*s;
	long		*lp;
	struct strval	*sp;
	long		deflt;
{
	struct strval *csp;
	char	line[128];
	char	*linep;

	for (;;) {
		csp = strval(*lp, sp);
		printf("%s [%s]:", s, csp->s_name);
		flush();
		line[0] = '\0';
		if (getline(line, 80) == EOF)
			exit(EX_BAD);

		linep = skipwhite(line);
		/*
		 * Nicht initialisierte Variablen
		 * duerfen nicht uebernommen werden
		 */
		if (linep[0] == '\0' && *lp != -1L)
			return (FALSE);

		if (strlen(linep) == 0) {
			/* Leere Eingabe */
		} else if (streql(linep, "?") || streql(linep, "help")) {
			printf("Possible values are:\n");
			for (csp = sp; csp->s_name; csp++) {
				printf("%s\t%s\n", csp->s_name, csp->s_text);
			}
		} else if ((csp = namestrval(linep, sp)) != 0) {
			*lp = csp->s_val;
			return (TRUE);
		}
	}
	/* NOTREACHED */
}
