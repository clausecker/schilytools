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
/*
 * @(#)dtime.c	1.3 19/05/15 Copyright 2006-2019 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)dtime.c 1.3 19/05/15 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)dtime.c"
#pragma ident	"@(#)sccs:lib/comobj/dtime.c"
#endif
#include	<defines.h>

void
dtime(dt)
	dtime_t	*dt;
{
#ifdef	NO_NANOSECS
	time(&dt->dt_sec);
	dt->dt_nsec = 0;
	dt->dt_zone = gmtoff(dt->dt_sec);
#else
	struct timespec	ts;

	getnstimeofday(&ts);
	dt->dt_sec = ts.tv_sec;
	dt->dt_nsec = ts.tv_nsec;
	dt->dt_zone = gmtoff(dt->dt_sec);
#endif
}

void
time2dt(dt, secs, nsecs)
	dtime_t	*dt;
	time_t	secs;
	int	nsecs;
{
	dt->dt_sec  = secs;
	dt->dt_nsec = nsecs;
	dt->dt_zone = gmtoff(dt->dt_sec);
}

time_t
gmtoff(crtime)
	time_t	crtime;
{
	struct tm	local;
	struct tm	gmt;
	struct tm	*tp;

	tp    = localtime(&crtime);
	if (tp == NULL)
		fatal(gettext("time stamp conversion error (cm19)"));
	local = *tp;

	tp    = gmtime(&crtime);
	if (tp == NULL)
		fatal(gettext("time stamp conversion error (cm19)"));
	gmt   = *tp;

	local.tm_sec  -= gmt.tm_sec;
	local.tm_min  -= gmt.tm_min;
	local.tm_hour -= gmt.tm_hour;
	local.tm_yday -= gmt.tm_yday;
	local.tm_year -= gmt.tm_year;
	if (local.tm_year)		/* Hit new-year limit	*/
		local.tm_yday = local.tm_year;	/* yday = +-1	*/

	crtime = local.tm_sec + 60 *
		    (local.tm_min + 60 *
			(local.tm_hour + 24 * local.tm_yday));

	return (crtime);
}
