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
 * @(#)strptim.c	1.12 11/08/26 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)strptim.c 1.12 11/08/26 J. Schilling"
#endif
/*
 * @(#)strptim.c 1.7 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)strptime.c"
#pragma ident	"@(#)sccs:lib/comobj/strptime.c"
#endif
#include <defines.h>

/*
 * Convert a datetime string to struct tm.
 * The conversion is similar to strptime(p, "%y/%m/%d %H:%M:%S", tp).
 *
 * We assume that p looks like: "91/04/13 23:23:46" if val = 1
 * We assume that p looks like: "910413232346" if val = 0
 *
 * If val == 0, we are converting cutoff times from command line.
 * If val == 1, we are converting delta table entries for prs(1).
 */
int
mystrptime(p, t, val)
	char		*p;
	struct tm	*t;
	int		val;
{
	int	y, dn, cn, warn = 0;
	int	ns = 0;
	int	tz = DT_NO_ZONE;
#if defined(BUG_1205145) || defined(GMT_TIME)
	time_t	gtime;
#endif

	/*
	 * If 'year' field is less than 70 then we actually have 
	 * 21-st century. Add 100 to the t->tm_year because 
	 * tm_year should be a year since 1900.
	 * For example, the string 02/09/05 should give tm_year
	 * equal to 102, not 2.
	 */
	memset(t, 0, sizeof(*t));
	if (val) {
		NONBLANK(p);

		t->tm_year=gNp(p, &p, 4, &dn, &cn);
		if(t->tm_year<0) return(-1);
		if((dn!=2 && dn!=4) || cn!=dn || *p!='/') warn=1;
		if(dn<=2) {
			if(t->tm_year<69) {
				t->tm_year += 100;
			}
		} else {
			if (t->tm_year<1969) {
				return(-1);
			}
			t->tm_year -= 1900;
		}
		y = t->tm_year + 1900;		/* For Gregorian leap year */

		t->tm_mon=gNp(p, &p, 2, &dn, &cn);
		if(t->tm_mon<1 || t->tm_mon>12) return(-1);
		if(dn!=2 || cn!=dn+1 || *p!='/') warn=1;

		t->tm_mday=gNp(p, &p, 2, &dn, &cn);
		if(t->tm_mday<1 || t->tm_mday>mosize(y,t->tm_mon)) return(-1);
		if(dn!=2 || cn!=dn+1) warn=1;
		t->tm_mon -= 1;			/* tm_mon is 0..11 */

		NONBLANK(p);

		t->tm_hour=gNp(p, &p, 2, &dn, &cn);
		if(t->tm_hour<0 || t->tm_hour>23) return(-1);
		if(dn!=2 || cn!=dn || *p!=':') warn=1;

		t->tm_min=gNp(p, &p, 2, &dn, &cn);
		if(t->tm_min<0 || t->tm_min>59) return(-1);
		if(dn!=2 || cn!=dn+1 || *p!=':') warn=1;

		t->tm_sec=gNp(p, &p, 2, &dn, &cn);
		if(t->tm_sec<0 || t->tm_sec>59) return(-1);
		if(dn!=2 || cn!=dn+1) warn=1;

		if (*p == '.')
			ns = gns(p, &p);
		if (*p == '+' || *p == '-')
			tz = gtz(p, &p);

#if defined(BUG_1205145) || defined(GMT_TIME)
		gtime = mktime(t);		/* local time -> GMT time_t */
		if (gtime == -1) return(-1);
		*t = *(gmtime(&gtime));		/* GMT time_t -> GMT tm *   */
#endif
	} else {
		char *sl = strchr(p, '/');

		if (sl && sl - p == 4) {	/* Permit 4-digit cutoff year */
			t->tm_year = gNp(p, &p, 4, &dn, &cn);
			if (dn != 4 || *p != '/')
				return (-1);
			p++;			/* Skip '/'		*/
#if SIZEOF_TIME_T == 4
			if (t->tm_year < 1933) { /* Unsupported in 32bit mode */
				return(-1);	 /* see xlocaltime.c	*/
			}
#endif
			t->tm_year -= 1900;
		} else {
			if((t->tm_year=gN(p, &p, 2, NULL, NULL)) == -2) t->tm_year = 99;
			if (t->tm_year<69) t->tm_year += 100;
		}
		y = t->tm_year + 1900;		/* For Gregorian leap year */

		if((t->tm_mon=gN(p, &p, 2, NULL, NULL)) == -2) t->tm_mon = 12;
		if(t->tm_mon<1 || t->tm_mon>12) return(-1);
		cn = t->tm_mon;
		t->tm_mon -= 1;			/* tm_mon is 0..11 */

		if((t->tm_mday=gN(p, &p, 2, NULL, NULL)) == -2) t->tm_mday = mosize(y,cn);
		if(t->tm_mday<1 || t->tm_mday>mosize(y,cn)) return(-1);

		if((t->tm_hour=gN(p, &p, 2, NULL, NULL)) == -2) t->tm_hour = 23;
		if(t->tm_hour<0 || t->tm_hour>23) return(-1);

		if((t->tm_min=gN(p, &p, 2, NULL, NULL)) == -2) t->tm_min = 59;
		if(t->tm_min<0 || t->tm_min>59) return(-1);

		if((t->tm_sec=gN(p, &p, 2, NULL, NULL)) == -2) t->tm_sec = 59;
		if(t->tm_sec<0 || t->tm_sec>59) return(-1);
	}
	return(warn);
}
