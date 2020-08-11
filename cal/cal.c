/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
/*
 * Copyright 2004 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*
 * Copyright 2019-2020 J. Schilling
 *
 * @(#)cal.c       1.8 20/07/24 J. Schilling
 *
 * From @(#)cal.c      1.14    05/06/08 SMI
 */

#ifdef	SCHILY_INCLUDES
#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/time.h>
#include <schily/locale.h>
#include <schily/nlsdefs.h>
#else
#include <libintl.h>
#include <langinfo.h>
#include <locale.h>
#include <nl_types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#endif

static int number(char *);
static int jan1(const int);
static void badmonth(void);
static void badyear(void);
static void usage(void);
static void cal(const int, const int, char *, const int);
static int gregoff(int y);
static int parsegreg(char *ep);
static void load_months(void);
static void pstr(char *, const int);

#define	DAYW	" S  M Tu  W Th  F  S"
#define	TITLE	"   %s %u\n"
#define	YEAR	"\n\n\n\t\t\t\t%u\n\n"
#define	MONTH	"\t%4.3s\t\t\t%.3s\t\t%10.3s\n"

static char *months[] = {
	"January", "February", "March", "April",
	"May", "June", "July", "August",
	"September", "October", "November", "December",
};

static char *short_months[] = {
	"Jan", "Feb", "Mar", "Apr",
	"May", "Jun", "Jul", "Aug",
	"Sep", "Oct", "Nov", "Dec",
};

static char _mon[] = {
	0,
	31, 29, 31, 30,
	31, 30, 31, 31,
	30, 31, 30, 31,
};
static char mon[13];

/*
 * Default to the switch done by the catholic parts of the Holy Roman Empire
 * but note that POSIX requires to use the switch date for the Kingdom of
 * Great Britain and their colonies (e.g. the location now called USA).
 *
 * The switch date for most regions are not related to countries from today,
 * for this reason, it is impossible to use locale derived data for the switch
 * date of a specific region.
 *
 * The more a country has been in conflict with the catholic church, the later
 * the switch to the Gregorian calendar happened. The following list uses the
 * first day in Julian calendar notation that is skipped by the switch. So
 * 1582/10/4 is the last day in the HRE where the Julian calendar is used.
 *
 * HRE:     1582/10/5		e.g. Cologne, catholic parts only
 * Prussia: 1612/08/23		e.g. Berlin, spans two months
 * HRE:     1700/02/19		Remaining rest of HRE
 * England: 1752/09/3		POSIX
 * Russia:  1918/02/1		e.g. Moscow
 */
static	int	yg = 1582;	/* Year of gregorian switch	*/
static	int	mg = 10;	/* Month of gregorian switch	*/
static	int	dg = 5; 	/* Day of gregorian switch	*/
static	int	sg = 10;	/* # of days to skip		*/
static	int	lg = 0;		/* Jul leap year affects switch	*/

static char *myname;
static char string[432];
static struct tm *thetime;
static time_t timbuf;

int
main(int argc, char *argv[])
{
	int y, i, j;
	int m;
	char *time_locale;
	char	*ldayw;
	char	*ep;
	char	*lcl;

	myname = argv[0];

	(void) setlocale(LC_ALL, "");
	lcl = setlocale(LC_TIME, "");
	if (lcl == NULL)
		lcl = "C";
#if !defined(TEXT_DOMAIN)
#define	TEXT_DOMAIN	"SYS_TEST"
#endif
	(void) textdomain(TEXT_DOMAIN);


	while (getopt(argc, argv, "") != EOF)
		usage();

	argc -= optind;
	argv  = &argv[optind];

	time_locale = setlocale(LC_TIME, NULL);
	if ((time_locale[0] != 'C') || (time_locale[1] != '\0'))
		load_months();

	if ((ep = getenv("GREGORIAN")) != NULL &&
	    (*ep == '+' || strcmp(lcl, "C") != 0)) {
		/*
		 * If *ep == '+', we enforce to honor "GREGORIAN".
		 */
		if (!parsegreg(ep)) {
			(void) fprintf(stderr,
				gettext("%s: bad gregorian switch '%s'\n"),
				myname, ep);
			usage();
		}
	} else {
		/*
		 * This is the current English (UNIX) default:
		 */
		yg = 1752;
		mg = 9;
		dg = 3;
	}
	/*
	 * Compute the number of leap days when switching from
	 * Julian to Gregorian. This is 10 days in October 1582
	 * and increases by 3 days every 400 years, but the next
	 * increase happens to the end of February when the
	 * Julian calendar has a leap year while the Gregorian does not.
	 */
	sg = gregoff(yg);
	if (yg % 4 == 0) {	/* Julian leap year */
		if (gregoff(yg+1) > gregoff(yg)) {
			if (mg >= 3) {
				/* Switch past February */
				sg += 1;
				lg = 1;
			} else if (mg == 2 && (dg+sg) > 28) {
				/* Switch includes Julian leap day */
				sg += 1;
				lg = 1;
			} else {
				lg = 0;
			}
		}
	}

	/*
	 * TRANSLATION_NOTE
	 * This message is to be used for displaying
	 * the names of the seven days, from Sunday to Saturday.
	 * The length of the name of each one should be two or less.
	 */
	ldayw = dcgettext(NULL, DAYW, LC_TIME);

	switch (argc) {
	case 0:
		timbuf = time(&timbuf);
		thetime = localtime(&timbuf);
		m = thetime->tm_mon + 1;
		y = thetime->tm_year + 1900;
		break;
	case 1:
		goto xlong;
	case 2:
		m = number(argv[0]);
		y = number(argv[1]);
		break;
	default:
		usage();
	}

/*
 *	print out just month
 */

	if (m < 1 || m > 12)
		badmonth();
	if (y < 1 || y > 9999)
		badyear();
	/*
	 * TRANSLATION_NOTE
	 * This message is to be used for displaying
	 * specified month and year.
	 */
	(void) printf(dcgettext(NULL, TITLE, LC_TIME), months[m-1], y);
	(void) printf("%s\n", ldayw);
	cal(m, y, string, 24);
	for (i = 0; i < 6*24; i += 24)
		pstr(string+i, 24);
	return (0);

/*
 *	print out complete year
 */

xlong:
	y = number(argv[0]);
	if (y < 1 || y > 9999)
		badyear();
	/*
	 * TRANSLATION_NOTE
	 * This message is to be used for displaying
	 * specified year.
	 */
	(void) printf(dcgettext(NULL, YEAR, LC_TIME), y);
	for (i = 0; i < 12; i += 3) {
		for (j = 0; j < 6*72; j++)
			string[j] = '\0';
		/*
		 * TRANSLATION_NOTE
		 * This message is to be used for displaying
		 * names of three months per a line and should be
		 * correctly translated according to the display width
		 * of the names of months.
		 */
		(void) printf(
			dcgettext(NULL, MONTH, LC_TIME),
			short_months[i], short_months[i+1], short_months[i+2]);
		(void) printf("%s   %s   %s\n", ldayw, ldayw, ldayw);
		cal(i+1, y, string, 72);
		cal(i+2, y, string+23, 72);
		cal(i+3, y, string+46, 72);
		for (j = 0; j < 6*72; j += 72)
			pstr(string+j, 72);
	}
	(void) printf("\n\n\n");
	return (0);
}

static int
number(char *str)
{
	long	l;
	int	n;
	char	*s = str;

	n = l = strtol(str, &s, 10);
	if (*s)
		return (0);
	if (n != l)
		return (0);

	return (n);
}

static void
pstr(char *str, const int n)
{
	int i;
	char *s;

	s = str;
	i = n;
	while (i--)
		if (*s++ == '\0')
			s[-1] = ' ';
	i = n+1;
	while (i--)
		if (*--s != ' ')
			break;
	s[1] = '\0';
	(void) printf("%s\n", str);
}

static void
cal(const int m, const int y, char *p, const int w)
{
	int d, i;
	int skip = 0;
	char *s;

	s = (char *)p;
	d = jan1(y);

	/*
	 * Reset month table to default
	 */
	for (i = 1; i <= 12; i++)
		mon[i] = _mon[i];

	switch ((jan1(y+1)+7-d)%7) {

	/*
	 *	non-leap year
	 */
	case 1:
		mon[2] = 28;
		break;

	/*
	 *	The Gregorian switch year
	 */
	default:
		break;

	/*
	 *	leap year
	 */
	case 2:
		;
	}
	if (y == yg) {
		/*
		 * If we are in a year that is a Julian leap year but not in
		 * the Gregorian calendar, we need to use a non-leap February,
		 * if the calendar switch is active already at the end of
		 * February or if the skipped days include the leap day.
		 */
		if (!lg)
			mon[2] = 28;

		if (mon[mg] >= (sg + dg)) {
			mon[mg] -= sg;
		} else {
			int	l = mon[mg];

			mon[mg] = dg-1;
			skip = sg - l + mon[mg];
		}
	} else if (((y-1) == yg) && (mg == 12) && (dg+sg) > 32) {
		/*
		 * Compute the number of days to skip in January.
		 */
		skip = (dg+sg) - 32;
	}
	for (i = 1; i < m; i++)
		d += mon[i];
	if (y == yg && m > (mg+1))	/* Correct wday a month after next */
		d -= skip;		/* if skipped days span two months */
	else if (((y-1) == yg) && m == 1) /* January after switch year and */
		d += skip;		/* skipped days span year start	   */

	d %= 7;
	s += 3*d;
	for (i = 1; i <= mon[m]; i++) {
		if (y == yg) {
			if (i == dg && m == mg) {
				i += sg;
				mon[m] += sg;
			}
			if (i == 1 && m == (mg+1))
				i += skip;
		} else if (((y-1) == yg) && m == 1) {
			if (i == 1)
				i += skip;
			
		}
		if (i > 9)
			*s = i/10+'0';
		s++;
		*s++ = i%10+'0';
		s++;
		if (++d == 7) {
			d = 0;
			s = p+w;
			p = s;
		}
	}
}

/*
 *	return day of the week
 *	of jan 1 of given year
 */

static int
jan1(const int yr)
{
	int y, d;

/*
 *	normal gregorian calendar
 *	one extra day per four years
 */

	y = yr;
	d = 4+y+(y+3)/4;

/*
 *	julian calendar
 *	regular gregorian
 *	less three days per 400
 */

	if (y > yg) {
		d -= (y-1601)/100;
		d += (y-1601)/400;
	}

/*
 *	great calendar changeover instant
 */

	if (y > yg)
		d += 4;

	return (d%7);
}

/*
 * Compute the number of skip days when switching from
 * Julian to Gregorian. This is 10 days in October 1582
 * and increases by 3 days every 400 years. The returned
 * value is based on January.
 */
static int
gregoff(y)
	int	y;
{
	int	d = 0;

	if (y < 1582)
		return (0);

	d += (y-1601)/100;
	d -= (y-1601)/400;
	d += 10;
	return (d);
}

static int
parsegreg(ep)
	char	*ep;
{
	char	*p = ep;
	long	y;
	long	m;
	long	d;
	char	c;

	if (*ep == '+')		/* Skip force marker */
		ep++;
	if (*ep == '\0')	/* Default to the predfined HRE switch */
		return (1);

	if (*ep == 'P') {	/* Prussia */
		yg = 1612;
		mg = 8;
		dg = 23;
		return (1);
	}

	y = strtol(ep, &p, 10);
	c = *p++;
	if (y == 0 || (c != '/' && c != '-'))
		return (0);

	m = strtol(p, &p, 10);
	c = *p++;
	if (m == 0 || (c != '/' && c != '-'))
		return (0);	

	d = strtol(p, &p, 10);
	if (d == 0 || *p != '\0')
		return (0);

	/*
	 * In the year 4000, the number of days to skip would be 28. Later,
	 * we could have 3 affected months which is currently not supported.
	 */
	if (y < 1582 || y > 4000)
		return (0);
	if (m < 1 || m > 12)
		return (0);
	if (d < 1 || d > _mon[m])
		return (0);

	yg = y;
	mg = m;
	dg = d;

	return (1);
}

static void
load_months(void)
{
	int month;

#ifdef	MON_1
	for (month = MON_1; month <= MON_12; month++)
		months[month - MON_1] = nl_langinfo(month);
#endif
#ifdef	ABMON_1
	for (month = ABMON_1; month <= ABMON_12; month++)
		short_months[month - ABMON_1] = nl_langinfo(month);
#endif
}

static void
badmonth()
{
	(void) fprintf(stderr, gettext("%s: bad month\n"), myname);
	usage();
}

static void
badyear()
{
	(void) fprintf(stderr, gettext("%s: bad year\n"), myname);
	usage();
}

static void
usage(void)
{
	(void) fprintf(stderr, gettext("usage: %s [ [month] year ]\n"), myname);
	exit(1);
}
