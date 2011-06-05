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
 * @(#)xlocaltime.c	1.1 11/05/26 Copyright 2007-2011 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)xlocaltime.c 1.1 11/05/26 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)xlocaltime.c"
#pragma ident	"@(#)sccs:lib/comobj/xlocaltime.c"
#endif
# include	<defines.h>

#ifdef	localtime	/* Only defined if this is a 32 bit compilation */
#undef	localtime

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

struct tm *
xlocaltime(t)
	time_t	*t;
{
	time_t	tim = *t;
	struct tm *tm;

	if (tim > ((time_t)0xba393f8f))	/* 2068/12/31 23:59:59 + 25 h */
		return (localtime(t));

	if (tim >= ((time_t)0xb4aadc80)) { /* 0x80000000 + 883612800 */
		tim -= 1767225600;	/* 56 years */
		tm = localtime(&tim);
		tm->tm_year += 56;	/* 2 * 4 * 7 */
	} else {
		tim -= 883612800;	/* 28 years */
		tm = localtime(&tim);
		tm->tm_year += 28;	/* 1 * 4 * 7 */
	}
	return (tm);
}
#endif
