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
 * @(#)xmktime.c	1.2 19/05/15 Copyright 2007-2019 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)xmktime.c 1.2 19/05/15 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)xmktime.c"
#pragma ident	"@(#)sccs:lib/comobj/xmktime.c"
#endif
#include	<defines.h>

#ifdef	mktime	/* Only defined if this is a 32 bit compilation */
#undef	mktime

/*
 * An attempt to make 32 bit SCCS at least partially work past
 * 2038/01/19 03:14:07 GMT:
 *
 * 1932/11/26 18:31:44 ... 2038/01/19 03:14:07 => normal case
 * 2038/01/19 03:14:08 ... 2068/12/31 23:59:59 => map to 1901/12/13 - 1932/11/25
 *
 * As (within the same century) every 28th year looks the same, we subtract
 * either 883612800 (28 years) or 1767225600 (56 years) from a time past
 * 2038/01/19 03:14:07 to make it a positive integer time_t for conversion.
 * Then the year is corrected. This however requires not to change daylight
 * saving rules past year 2010 to be correct for the functions
 * localtime()/mktime().
 * For this reason, the russian TZs are already known to fail for the last
 * months in Y2039 as russia decided to abandon winter time in 2011.
 *
 * POSIX requires timezones -24 through +25
 * +25 Hours -> 90000 0x15f90
 * 2068/12/31 23:59:59 GMT	-> 0xba37dfff (1932/11/25 17:31:43)
 * 2068/12/31 23:59:59 + 25 h	-> 0xba393f8f (1932/11/26 18:31:43)
 * 2038/01/19 03:14:07 GMT	-> 0x7fffffff
 * 1901/12/13 20:45:52 GMT	-> 0x80000000
 */

time_t
xmktime(tm)
	struct tm *tm;
{
	time_t	t;
	int	oerr = errno;

	errno = 0;
	if (tm->tm_year >= 166) {	/* 2066 */
		tm->tm_year -= 56;	/* 2 * 4 * 7 */
		t = mktime(tm);
		tm->tm_year += 56;	/* fix external data */
		t += 1767225600;	/* 56 years */

	} else if (tm->tm_year >= 138) { /* 2038 */
		tm->tm_year -= 28;	/* 1 * 4 * 7 */
		t = mktime(tm);
		tm->tm_year += 28;	/* fix external data */
		t += 883612800;		/* 28 years */

	} else {
		t = mktime(tm);
	}
	if (errno)
		fatal(gettext("time stamp conversion error (cm19)"));
	else
		errno = oerr;
	return (t);
}
#endif
