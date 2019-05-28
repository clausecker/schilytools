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
 *
 * @(#)xtzset.c	1.7 19/05/15  Copyright 2006-2019 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)xtzset.c 1.7 19/05/15 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)xtzset.c"
#pragma ident	"@(#)sccs:lib/comobj/xtzset.c"
#endif
#include	<defines.h>

#define	TMCHK()	{ if (tm == NULL)					\
		fatal(gettext("time stamp conversion error (cm19)")); }

#ifdef	MAIN
#undef	localtime
#undef	gmtime
#undef	mktime
#define	fatal	printf
#endif

time_t	Y2069;
time_t	Y2038;
time_t	Y1969;

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
	int	oerr = errno;

#ifdef	HAVE_TZSET
#undef	tzset
	tzset();
#endif
#ifdef	HAVE_VAR_TIMEZONE
	if (timezone != 0) {
		if (Y2038 == 0) {
#if defined(BUG_1205145) || defined(GMT_TIME)
			Y2069 = _Y2069;
			Y2038 = _Y2038;
			Y1969 = _Y1969;
#else
			Y2069 = _Y2069 + timezone;
			Y2038 = _Y2038 + timezone;
			Y1969 = _Y1969 + timezone;
#endif
		}
		return;
	}
#endif

	t = time((time_t *)0);	/* Current time in GMT since Jan 1 1970 */

	errno = 0;
#if	defined(HAVE_GMTIME) && defined(HAVE_LOCALTIME) && defined(HAVE_MKTIME)
	tm = gmtime(&t);	/* struct tm from current time in GMT */
	TMCHK();
	t -= tm->tm_mon * 30 * 24 * 3600;	/* shift to aprox. winter */
	tm = gmtime(&t);	/* struct tm from last winter time in GMT */
	TMCHK();
	t2 = mktime(tm);	/* GMT assuming tm is local time */
	tm = localtime(&t);
	TMCHK();
	t3 = mktime(tm);	/* t3 should be == t */
#else
#if	defined(HAVE_GMTIME) && defined(HAVE_TIMELOCAL) && defined(HAVE_TIMEGM)
	tm = gmtime(&t);	/* struct tm from current time in GMT */
	TMCHK();
	t -= tm->tm_mon * 30 * 24 * 3600;	/* shift to aprox. winter */
	tm = gmtime(&t);	/* struct tm from last winter time in GMT */
	TMCHK();
	t2 = timelocal(tm);	/* GMT assuming tm is local time */
	t3 = timegm(tm);	/* GMT assuming tm is GMT	 */
#endif
#endif
	if (errno)
		fatal(gettext("time stamp conversion error (cm19)"));
	else
		errno = oerr;

	timezone = t2 - t3;

#ifdef	HAVE_FTIME
	if (timezone == 0) {
		ftime(&timeb);
		timezone = timeb.timezone * 60;
	}
#endif
	if (Y2038 == 0) {
#if defined(BUG_1205145) || defined(GMT_TIME)
		Y2069 = _Y2069;
		Y2038 = _Y2038;
		Y1969 = _Y1969;
#else
		Y2069 = _Y2069 + timezone;
		Y2038 = _Y2038 + timezone;
		Y1969 = _Y1969 + timezone;
#endif
	}
}

#ifdef	MAIN
main()
{
	xtzset();

	printf("timezone: %ld\n", (long)timezone);
}
#endif
