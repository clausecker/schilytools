/* @(#)ptime.c	1.3 18/10/28 Copyright 2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)ptime.c	1.3 18/10/28 Copyright 2018 J. Schilling";
#endif
/*
 *	Parse time in a format similar to ISO 8601
 *
 *	yyyy-mm-ddThh:mm:ss.nnnnnnnnn+0000
 *
 *	We are as forgiving as possible and accept anything that is similar
 *	enough.
 *
 *	Copyright (c) 2018 J. Schilling
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
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/time.h>
#include <schily/string.h>
#include <schily/schily.h>

#include "find_misc.h"

EXPORT	const char	*parsetime	__PR((const char *s,
						struct timespec *ts));
LOCAL	int		mosize		__PR((int y, int m));
LOCAL	int		gnext		__PR((const char *str,
						const char **next, int num,
						int *digits, int *chars));
LOCAL	int		gns		__PR((const char *str,
						const char **next));
LOCAL	int		gtz		__PR((const char *str,
						const char **next));

#undef	numeric
#define	numeric(c)	((c) >= '0' && (c) <= '9')
#define	SKIPBLANK(p)	while (*(p) == ' ' || *(p) == '\t') (p)++

#define	DT_NO_ZONE	1	/* Impossible timezone - no zone found  */
#define	DT_MIN_ZONE	(-89940) /* Minimum zone (-24:59)		*/
#define	DT_MAX_ZONE	93540	/* Maximum zone (+25:59)		*/

#define	dysize(A) (((A)%4)? 365 : (((A)%100) == 0 && ((A)%400)) ? 365 : 366)

static int dmsize[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

EXPORT	const char *
parsetime(s, ts)
	const	char	*s;
	struct timespec	*ts;
{
	char		*p;
	struct	tm	tm;
	struct	tm	tmthis;
	time_t		t;
	int		tz = DT_NO_ZONE;
	int		ns = 999999999;
	int		nchars;
	int		ndigit;
	int		y;

	fillbytes(&tm, sizeof (tm), '\0');
	time(&t);
	tmthis = *localtime(&t);

	SKIPBLANK(s);

	if (streql(s, "now")) {
		getnstimeofday(ts);
		return (&s[3]);
	}

	p = strchr(s, 'T');
	if (p == NULL)
		p = strchr(s, ' ');

	y = tmthis.tm_year + 1900;
	tm.tm_year = tmthis.tm_year;
	tm.tm_mon = tmthis.tm_mon + 1;
	tm.tm_mday = tmthis.tm_mday;

	if (*s == 'T') {
		s++;
		goto gtime;
	}
	if (p && ((p - s) < 4))
		goto gmday;
	if (p && ((p - s) < 6))
		goto gmon;
	if (*s == '-') {
		s++;
		goto gmon;
	}

	tm.tm_year = gnext(s, &s, 4, &ndigit, &nchars);
	if (tm.tm_year < 0)
		return (NULL);
	if ((ndigit != 2 && ndigit != 4) || nchars != ndigit ||
	    (*s != '/' && *s != '-' && *s != '\0'))
		return (NULL);
	if (ndigit <= 2) {
		if (tm.tm_year < 69) {
			tm.tm_year += 100;
		}
	} else {
		tm.tm_year -= 1900;
	}
	y = tm.tm_year + 1900;			/* For Gregorian leap year */

gmon:
	if ((tm.tm_mon = gnext(s, &s, 2, &ndigit, &nchars)) == -2)
		tm.tm_mon = 12;
	if (tm.tm_mon < 1 || tm.tm_mon > 12)
		return (NULL);
	if (ndigit > 0 &&
	    (ndigit != 2 || (nchars != ndigit && nchars != ndigit+1) ||
	    (*s != '/' && *s != '-' && *s != '\0')))
		return (NULL);

gmday:
	if ((tm.tm_mday = gnext(s, &s, 2, &ndigit, &nchars)) == -2)
		tm.tm_mday = mosize(y, tm.tm_mon);
	if (tm.tm_mday < 1 || tm.tm_mday > mosize(y, tm.tm_mon))
		return (NULL);
	if (ndigit > 0 &&
	    (ndigit != 2 || (nchars != ndigit && nchars != ndigit+1)))
		return (NULL);

gtime:
	SKIPBLANK(s);
	if (*s == 'T')
		s++;

	if ((tm.tm_hour = gnext(s, &s, 2, &ndigit, &nchars)) == -2)
		tm.tm_hour = 23;
	if (tm.tm_hour < 0 || tm.tm_hour > 23)
		return (NULL);

	if ((tm.tm_min = gnext(s, &s, 2, &ndigit, &nchars)) == -2)
		tm.tm_min = 59;
	if (tm.tm_min < 0 || tm.tm_min > 59)
		return (NULL);

	if ((tm.tm_sec = gnext(s, &s, 2, &ndigit, &nchars)) == -2)
		tm.tm_sec = 59;
	if (tm.tm_sec < 0 || tm.tm_sec > 59)
		return (NULL);

	tm.tm_mon -= 1;		/* tm_mon is 0..11 */
	tm.tm_isdst = -1;	/* let mktime() find out */

	if (*s == '.')
		ns = gns(s, &s);
	if (*s == '+' || *s == '-') {
		tz = gtz(s, &s);
		if (tz == DT_NO_ZONE)
			return (NULL);
	} else if (*s == 'Z') {
		s++;
		tz = 0;		/* GMT */
	}

	if (tz != DT_NO_ZONE) {
		t = mkgmtime(&tm);
		t -= tz;
	} else {
		t = mktime(&tm);
	}

	ts->tv_sec = t;
	ts->tv_nsec = ns;

	return (s);
}

LOCAL int
mosize(y, m)
	int	y;
	int	m;
{

	if (m == 2 && dysize(y) == 366)
		return (29);
	return (dmsize[m-1]);
}

/*
 * Get next value.
 * Return converted value or -2
 */
LOCAL int
gnext(str, next, num, digits, chars)
	const char *str;		/* The input string		    */
	const char **next;		/* To return the of the parsed string */
	int	num;			/* Number of digits to convert	    */
	int	*digits;		/* To return number of digits conv. */
	int	*chars;			/* To return number of chars used   */
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
		*chars = m + n;
	}

	return (c);
}

/*
 * Multiplicator to get nanoseconds from 9 - "number of digits read".
 * The 11th entry is for the theoretical case when *str == '\0'.
 */
static int	nsmult[] =
	{ 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 0, 0 };

/*
 * Get nanoseconds of a time stamp.
 * Correct the value depending on how many digits have been read.
 */
LOCAL int
gns(str, next)
	const char	*str;
	const char	**next;
{
	register int c = 0;
	register int num = 9;	/* max resolution is ns */

	if (*str++) {		/* Eat leading '.'	*/
		while ((--num >= 0) && numeric(*str)) {
			c = (c * 10) + (*str++ - '0');
		}
	} else {
		str--;
	}
	if (next) {
		*next = str;
	}
	c *= nsmult[++num];
	return (c);
}

/*
 * Get timezone value.
 * Leave unexpected characters for our caller.
 */
LOCAL int
gtz(str, next)
	const char	*str;
	const char	**next;
{
	register int	c;
	register int	tz;
		int	sign = 1;

	c = *str++;
	if (c == '-')
		sign = -1;
	else if (c != '+')	/* Syntax error: return current pos */
		return (DT_NO_ZONE);

	c = 0;
	if (numeric(*str))
		c = (c * 10) + (*str++ - '0');
	else
		return (DT_NO_ZONE);
	if (numeric(*str))
		c = (c * 10) + (*str++ - '0');
	else
		return (DT_NO_ZONE);

	tz = c * 60;
	c = 0;
	if (*str == '\0')
		return (tz);
	if (*str ==  ':')
		str++;
	if (numeric(*str))
		c = (c * 10) + (*str++ - '0');
	else
		return (DT_NO_ZONE);
	if (numeric(*str))
		c = (c * 10) + (*str++ - '0');
	else
		return (DT_NO_ZONE);
	if (c > 59)
		return (DT_NO_ZONE);

	tz += c;
	tz *= 60;
	tz *= sign;

	if (tz < DT_MIN_ZONE || tz > DT_MAX_ZONE)
		tz = DT_NO_ZONE;
	/*
	 * Do not update our read pointer in case zone is not within
	 * the permitted range from -24:59..+25:59.
	 */
	if (tz != DT_NO_ZONE && next) {
		*next = str;
	}
	return (tz);
}
