/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * This file contains modifications Copyright 2006-2011 J. Schilling
 *
 * @(#)date_ab.c	1.14 11/06/27 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)date_ab.c 1.14 11/06/27 J. Schilling"
#endif
/*
 * @(#)date_ab.c 1.8 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)date_ab.c"
#pragma ident	"@(#)sccs:lib/comobj/date_ab.c"
#endif
# include	<defines.h>


#define	dysize(A) (((A)%4)? 365 : (((A)%100) == 0 && ((A)%400)) ? 365 : 366)

char *Datep;
static int dmsize[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/*
 *	Function to convert date in the form "[yy|yyyy]/mm/dd hh:mm:ss" to
 *	standard UNIX time (seconds since Jan. 1, 1970 GMT).
 *
 *	The function corrects properly for leap year,
 *	daylight savings time, offset from Greenwich time, etc.
 *
 *	Function returns -1 if bad time is given.
 */
int
date_ab(adt, bdt, flags)
char	*adt;
time_t	*bdt;
int	flags;
{
	int	y, dn, cn, warn = 0;
	time_t	tim;
	struct tm tm;

	Datep = adt;

	NONBLANK(Datep);

	tm.tm_year = gN(Datep, &Datep, 4, &dn, &cn);
	if (tm.tm_year < 0) return (-1);
	if ((dn != 2 && dn != 4) || cn != dn || *Datep != '/') warn = 1;
	if (dn <= 2) {
		if (tm.tm_year < 69) {
			tm.tm_year += 100;
		}
	} else {
#if SIZEOF_TIME_T == 4
		if (tm.tm_year < 1933) {
			return (-1);
		}
#endif
		tm.tm_year -= 1900;
	}
	y = tm.tm_year + 1900;			/* For Gregorian leap year */

	tm.tm_mon = gN(Datep, &Datep, 2, &dn, &cn);
	if (tm.tm_mon < 1 || tm.tm_mon > 12) return (-1);
	if (dn != 2 || cn != dn+1 || *Datep != '/') warn = 1;

	tm.tm_mday = gN(Datep, &Datep, 2, &dn, &cn);
	if (tm.tm_mday < 1 || tm.tm_mday > mosize(y, tm.tm_mon)) return (-1);
	if (dn != 2 || cn != dn+1) warn = 1;

	NONBLANK(Datep);

	tm.tm_hour = gN(Datep, &Datep, 2, &dn, &cn);
	if (tm.tm_hour < 0 || tm.tm_hour > 23) return (-1);
	if (dn != 2 || cn != dn || *Datep != ':') warn = 1;

	tm.tm_min = gN(Datep, &Datep, 2, &dn, &cn);
	if (tm.tm_min < 0 || tm.tm_min > 59) return (-1);
	if (dn != 2 || cn != dn+1 || *Datep != ':') warn = 1;

	tm.tm_sec = gN(Datep, &Datep, 2, &dn, &cn);
	if (tm.tm_sec < 0 || tm.tm_sec > 59) return (-1);
	if (dn != 2 || cn != dn+1) warn = 1;

	tm.tm_mon -= 1;		/* tm_mon is 0..11 */
	tm.tm_isdst = -1;	/* let mktime() find out */

#if !(defined(BUG_1205145) || defined(GMT_TIME))
	if (flags & PF_GMT)
		tim = mklgmtime(&tm);
	else
		tim = mktime(&tm);
#else
	tim = mklgmtime(&tm);
#endif
	*bdt = tim;
	return (warn);
}

/*
 *	Function to convert date in the form "yymmddhhmmss" to
 *	standard UNIX time (seconds since Jan. 1, 1970 GMT).
 *	Units left off of the right are replaced by their
 *	maximum possible values.
 *
 *	We permit "yyyy/mmdd..." for 4-digit year numbers.
 *
 *	The function corrects properly for leap year,
 *	daylight savings time, offset from Greenwich time, etc.
 *
 *	Function returns -1 if bad time is given (i.e., "730229").
 */
int
parse_date(adt, bdt, flags)
char	*adt;
time_t	*bdt;
int	flags;
{
	int	y;
	time_t	tim;
	struct tm tm;
	char	*sl;

	sl = strchr(adt, '/');
	if (sl && sl - adt == 4) {		/* Permit 4-digit cutoff year */
		tm.tm_year = gN(adt, &adt, 4, &y, NULL);
		if (y != 4 || *adt != '/')
			return (-1);
		adt++;				/* Skip '/'		*/
#if SIZEOF_TIME_T == 4
		if (tm.tm_year < 1933) {	/* Unsupported in 32bit mode */
			return(-1);		/* see xlocaltime.c	*/
		}
#endif
		tm.tm_year -= 1900;
	} else {
		if ((tm.tm_year = gN(adt, &adt, 2, NULL, NULL)) == -2) tm.tm_year = 99;
		if (tm.tm_year < 69) tm.tm_year += 100;
	}
	y = tm.tm_year + 1900;			/* For Gregorian leap year */

	if ((tm.tm_mon = gN(adt, &adt, 2, NULL, NULL)) == -2) tm.tm_mon = 12;
	if (tm.tm_mon < 1 || tm.tm_mon > 12) return (-1);

	if ((tm.tm_mday = gN(adt, &adt, 2, NULL, NULL)) == -2) tm.tm_mday = mosize(y, tm.tm_mon);
	if (tm.tm_mday < 1 || tm.tm_mday > mosize(y, tm.tm_mon)) return (-1);

	if ((tm.tm_hour = gN(adt, &adt, 2, NULL, NULL)) == -2) tm.tm_hour = 23;
	if (tm.tm_hour < 0 || tm.tm_hour > 23) return (-1);

	if ((tm.tm_min = gN(adt, &adt, 2, NULL, NULL)) == -2) tm.tm_min = 59;
	if (tm.tm_min < 0 || tm.tm_min > 59) return (-1);

	if ((tm.tm_sec = gN(adt, &adt, 2, NULL, NULL)) == -2) tm.tm_sec = 59;
	if (tm.tm_sec < 0 || tm.tm_sec > 59) return (-1);

	tm.tm_mon -= 1;		/* tm_mon is 0..11 */
	tm.tm_isdst = -1;	/* let mktime() find out */

	if (flags & PF_GMT)
		tim = mklgmtime(&tm);
	else
		tim = mktime(&tm);

	*bdt = tim;
	return (0);
}

int
mosize(y, t)
int y, t;
{

	if (t == 2 && dysize(y) == 366) return (29);
	return (dmsize[t-1]);
}

int
gN(str, next, num, digits, chars)
	char	*str;
	char	**next;
	int	num;
	int	*digits;
	int	*chars;
{
	register int c = 0;
	register int n = 0;
	register int m = 0;

	while (*str && !numeric(*str)) {
		str++;
		m++;
	}
	if (*str) {
		while ((num-- > 0) && numeric(*str)) {
			c = (c * 10) + (*str++ - '0');
			n++;
			m++;
		}
	} else {
		c = -2;
	}
	if (next) {
		*next = str;
	}
	if (digits) {
		*digits = n;
	}
	if (chars) {
		*chars = m;
	}

	return (c);
}
