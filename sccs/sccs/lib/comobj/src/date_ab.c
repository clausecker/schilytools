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
 * @(#)date_ab.c	1.8 11/04/27 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)date_ab.c 1.8 11/04/27 J. Schilling"
#endif
/*
 * @(#)date_ab.c 1.8 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)date_ab.c"
#pragma ident	"@(#)sccs:lib/comobj/date_ab.c"
#endif
# include	<defines.h>

# include	<macros.h>
#if !(defined(BUG_1205145) || defined(GMT_TIME))
/*
 * time.h is already includes from defines.h
 */
/*# include	<time.h>*/
#endif

#define	dysize(A) (((A)%4)? 365 : (((A)%100) == 0 && ((A)%400)) ? 365 : 366)
/*
 * Return the number of leap years since 0 AD assuming that the Gregorian
 * calendar applies to all years.
 */
#define	LEAPS(Y) 	((Y) / 4 - (Y) / 100 + (Y) / 400)
/*
 * Return the number of days since 0 AD
 */
#define	YRDAYS(Y)	(((Y) * 365L) + LEAPS(Y))
/*
 * Return the number of days between Januar 1 1970 and the end of the year
 * before the the year used as argument.
 */
#define	DAYS_SINCE_70(Y) (YRDAYS((Y)-1) - YRDAYS(1970-1))

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
date_ab(adt, bdt)
char	*adt;
time_t	*bdt;
{
	int dn, cn, warn = 0;
	time_t	tim;
	struct tm tm;

#if !(defined(BUG_1205145) || defined(GMT_TIME))
	tzset();
#endif
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
		if (tm.tm_year < 1969) {
			return (-1);
		}
		tm.tm_year -= 1900;
	}

	tm.tm_mon = gN(Datep, &Datep, 2, &dn, &cn);
	if (tm.tm_mon < 1 || tm.tm_mon > 12) return (-1);
	if (dn != 2 || cn != dn+1 || *Datep != '/') warn = 1;

	tm.tm_mday = gN(Datep, &Datep, 2, &dn, &cn);
	if (tm.tm_mday < 1 || tm.tm_mday > mosize(tm.tm_year, tm.tm_mon)) return (-1);
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
	tim = mktime(&tm);
#else
	tim = mkgmtime(&tm);
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
 *	The function corrects properly for leap year,
 *	daylight savings time, offset from Greenwich time, etc.
 *
 *	Function returns -1 if bad time is given (i.e., "730229").
 */
int
parse_date(adt, bdt)
char	*adt;
time_t	*bdt;
{
	time_t	tim;
	struct tm tm;

	tzset();

	if ((tm.tm_year = gN(adt, &adt, 2, NULL, NULL)) == -2) tm.tm_year = 99;
	if (tm.tm_year < 69) tm.tm_year += 100;

	if ((tm.tm_mon = gN(adt, &adt, 2, NULL, NULL)) == -2) tm.tm_mon = 12;
	if (tm.tm_mon < 1 || tm.tm_mon > 12) return (-1);

	if ((tm.tm_mday = gN(adt, &adt, 2, NULL, NULL)) == -2) tm.tm_mday = mosize(tm.tm_year, tm.tm_mon);
	if (tm.tm_mday < 1 || tm.tm_mday > mosize(tm.tm_year, tm.tm_mon)) return (-1);

	if ((tm.tm_hour = gN(adt, &adt, 2, NULL, NULL)) == -2) tm.tm_hour = 23;
	if (tm.tm_hour < 0 || tm.tm_hour > 23) return (-1);

	if ((tm.tm_min = gN(adt, &adt, 2, NULL, NULL)) == -2) tm.tm_min = 59;
	if (tm.tm_min < 0 || tm.tm_min > 59) return (-1);

	if ((tm.tm_sec = gN(adt, &adt, 2, NULL, NULL)) == -2) tm.tm_sec = 59;
	if (tm.tm_sec < 0 || tm.tm_sec > 59) return (-1);

	tm.tm_mon -= 1;		/* tm_mon is 0..11 */
	tm.tm_isdst = -1;	/* let mktime() find out */

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

Llong
mkgmtime(tmp)
	struct tm	*tmp;
{
	Llong	tim = (time_t)0L;
	int	y = tmp->tm_year + 1900;
	int	t = tmp->tm_mon + 1;

	tim = DAYS_SINCE_70(y);
	while (--t)
		tim += mosize(y, t);
	tim += tmp->tm_mday - 1;
	tim *= 24;
	tim += tmp->tm_hour;
	tim *= 60;
	tim += tmp->tm_min;
	tim *= 60;
	tim += tmp->tm_sec;
	return (tim);
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

#ifdef	HAVE_FTIME
#include <sys/timeb.h>
#endif

void
xtzset()
{
	time_t	t;
	time_t	t2 = 0;
	time_t	t3 = 0;
	struct tm *tm = NULL;
#ifdef	HAVE_FTIME
	struct timeb timeb;
#endif

#ifdef	HAVE_TZSET
#undef	tzset
	tzset();
#endif
#ifdef	HAVE_VAR_TIMEZONE
	if (timezone != 0)
		return;
#endif

	t = time((time_t *)0);	/* Current time in GMT since Jan 1 1970 */

#if	defined(HAVE_GMTIME) && defined(HAVE_LOCALTIME) && defined(HAVE_MKTIME)
	tm = gmtime(&t);	/* struct tm from current time in GMT */
	t -= tm->tm_mon * 30 * 24 * 3600;	/* shift to aprox. winter */
	tm = gmtime(&t);	/* struct tm from last winter time in GMT */
	t2 = mktime(tm);	/* GMT assuming tm is local time */
	tm = localtime(&t);
	t3 = mktime(tm);	/* t3 should be == t */
#else
#if	defined(HAVE_GMTIME) && defined(HAVE_TIMELOCAL) && defined(HAVE_TIMEGM)
	tm = gmtime(&t);	/* struct tm from current time in GMT */
	t -= tm->tm_mon * 30 * 24 * 3600;	/* shift to aprox. winter */
	tm = gmtime(&t);	/* struct tm from last winter time in GMT */
	t2 = timelocal(tm);	/* GMT assuming tm is local time */
	t3 = timegm(tm);	/* GMT assuming tm is GMT	 */
#endif
#endif
	timezone = t2 - t3;

#ifdef	HAVE_FTIME
	if (timezone == 0) {
		ftime(&timeb);
		timezone = timeb.timezone * 60;
	}
#endif
}
