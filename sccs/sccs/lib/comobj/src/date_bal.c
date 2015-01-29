/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
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
 * This file contains modifications Copyright 2008-2015 J. Schilling
 *
 * @(#)date_bal.c	1.11 15/01/26 J. Schilling
 *
 * From Sun: @(#)sccs:lib/comobj/date_ba.c @(#)date_ba.c 1.5 06/12/12
 */
#if defined(sun)
#pragma ident "@(#)date_bal.c 1.11 15/01/26 J. Schilling"
#endif
# include	<defines.h>

# define DO2(p,n,c)	*p++ = ((char) ((n)/10) + '0'); *p++ = ( (char) ((n)%10) + '0'); *p++ = c;
# define DO2_(p,n)	*p++ = ((char) ((n)/10) + '0'); *p++ = ( (char) ((n)%10) + '0');
# define DO4(p,n,c)	*p++ = ((char) ((n)/1000) + '0'); *p++ = ( (char) ((n)%1000/100) + '0'); \
			*p++ = ((char) ((n)%100/10) + '0'); *p++ = ( (char) ((n)%10) + '0'); *p++ = c;


char *
date_bal(bdt, adt, flags)
time_t	*bdt;
char	*adt;
int	flags;
{
	dtime_t	dt;

	dt.dt_sec = *bdt;
	dt.dt_nsec = 0;
	dt.dt_zone = DT_NO_ZONE;
	return (date_bazl(&dt, adt, flags));
}

char *
date_bazl(bdt, adt, flags)
dtime_t	*bdt;
char	*adt;
int	flags;
{
	register struct tm *lcltm;
	register char	*p;
		int	zone = bdt->dt_zone;
		int	nsec = bdt->dt_nsec;

#if defined(BUG_1205145) || defined(GMT_TIME)
	lcltm = gmtime(&bdt->dt_sec);
#else
	if (zone != DT_NO_ZONE && (flags & PF_V6)) {
		time_t	sec = bdt->dt_sec + zone;

		lcltm = gmtime(&sec);
	} else if (flags & PF_GMT) {
		lcltm = gmtime(&bdt->dt_sec);
		zone = DT_NO_ZONE;
	} else {
		lcltm = localtime(&bdt->dt_sec);
		zone = DT_NO_ZONE;
	}
#endif
	p = adt;
	lcltm->tm_year += 1900;

	DO4(p,lcltm->tm_year,'/');
	DO2(p,lcltm->tm_mon + 1,'/');
	DO2(p,lcltm->tm_mday,' ');
	DO2(p,lcltm->tm_hour,':');
	DO2(p,lcltm->tm_min,':');
	DO2(p,lcltm->tm_sec,0);
	if (nsec > 0 && nsec < 1000000000) {
		char	*psave;
		int	n = 10;

		if (nsec % 1000 == 0) {
			nsec /= 1000;
			n = 7;
		}
		*--p = '.';
		psave = p += n;
		*p = '\0';
		while (--n > 0) {
			*--p = '0' + (nsec % 10);
			nsec /= 10;
		}
		p = ++psave;
	}
	if (zone != DT_NO_ZONE) {
		register int	z = zone / 60;	/* seconds -> minutes */
		register int	n;

		--p;
		if (z < 0) {
			*p++ = '-';
			z = -z;
		} else {
			*p++ = '+';
		}
		n = z / 60;
		DO2_(p, n);
		n = z % 60;
		DO2(p, n, 0);
	}
	return(adt);
}
