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
 * This file contains modifications Copyright 2006-2017 J. Schilling
 *
 * @(#)date_ab.c	1.19 17/07/16 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)date_ab.c 1.19 17/07/16 J. Schilling"
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

int
date_ab(adt, bdt, flags)
char	*adt;
time_t	*bdt;
int	flags;
{
	dtime_t	dt;
	int	ret = date_abz(adt, &dt, flags);

	*bdt = dt.dt_sec;
	return (ret);
}

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
date_abz(adt, bdt, flags)
char	*adt;			/* Begin of date time string	*/
dtime_t	*bdt;			/* Returned time information	*/
int	flags;			/* Flags from packet		*/
{
	int	y, dn, cn, warn = 0;
	int	ns = 0;
	int	tz = DT_NO_ZONE;
	time_t	tim;
	struct tm tm;

	Datep = adt;

	NONBLANK(Datep);

	tm.tm_year = gNp(Datep, &Datep, 4, &dn, &cn);
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

	tm.tm_mon = gNp(Datep, &Datep, 2, &dn, &cn);
	if (tm.tm_mon < 1 || tm.tm_mon > 12) return (-1);
	if (dn != 2 || cn != dn+1 || *Datep != '/') warn = 1;

	tm.tm_mday = gNp(Datep, &Datep, 2, &dn, &cn);
	if (tm.tm_mday < 1 || tm.tm_mday > mosize(y, tm.tm_mon)) return (-1);
	if (dn != 2 || cn != dn+1) warn = 1;

	NONBLANK(Datep);

	tm.tm_hour = gNp(Datep, &Datep, 2, &dn, &cn);
	if (tm.tm_hour < 0 || tm.tm_hour > 23) return (-1);
	if (dn != 2 || cn != dn || *Datep != ':') warn = 1;

	tm.tm_min = gNp(Datep, &Datep, 2, &dn, &cn);
	if (tm.tm_min < 0 || tm.tm_min > 59) return (-1);
	if (dn != 2 || cn != dn+1 || *Datep != ':') warn = 1;

	tm.tm_sec = gNp(Datep, &Datep, 2, &dn, &cn);
	if (tm.tm_sec < 0 || tm.tm_sec > 59) return (-1);
	if (dn != 2 || cn != dn+1) warn = 1;

	tm.tm_mon -= 1;		/* tm_mon is 0..11 */
	tm.tm_isdst = -1;	/* let mktime() find out */

	if (*Datep == '.')
		ns = gns(Datep, &Datep);
	if (*Datep == '+' || *Datep == '-')
		tz = gtz(Datep, &Datep);

#if !(defined(BUG_1205145) || defined(GMT_TIME))
	if (tz != DT_NO_ZONE && (flags & PF_V6)) {
		tim = mklgmtime(&tm);
		tim -= tz;
	} else if (flags & PF_GMT) {
		tim = mklgmtime(&tm);
	} else {
		tim = mktime(&tm);
	}
#else
	tim = mklgmtime(&tm);
#endif
	bdt->dt_sec = tim;
	bdt->dt_nsec = ns;
	bdt->dt_zone = tz;
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

/*
 * This is one of the most time consuming functions, when called from
 * date_baz(). It makes sense to have a separate version that always
 * expects the pointers "next", "digits" and "chars".
 */
int
gNp(str, next, num, digits, chars)
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
		}
	} else {
		c = -2;
	}
	*next = str;
	*digits = n;
	*chars = m + n;

	return (c);
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
static int	nsmult[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 0, 0 };

/*
 * Get nanoseconds of a time stamp.
 * Correct the value depending on how many digits have been read.
 */
int
gns(str, next)
	char	*str;
	char	**next;
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
int
gtz(str, next)
	char	*str;
	char	**next;
{
	register int c;
	register int tz;
		 int sign = 1;

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
