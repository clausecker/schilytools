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
 * @(#)dogtime.c	1.1 18/12/16 Copyright 2015-2018 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)dogtime.c	1.1 18/12/16 Copyright 2015-2018 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)doget.c"
#pragma ident	"@(#)sccs:lib/comobj/doget.c"
#endif
#include	<defines.h>

void
dogtime(pkt, gfile, mtime)
	struct packet	*pkt;
	char		*gfile;
	struct timespec	*mtime;
{
	struct timespec	ts[2];
	extern dtime_t	Timenow;

	ts[0].tv_sec = Timenow.dt_sec;
	ts[0].tv_nsec = Timenow.dt_nsec;
	ts[1].tv_sec = mtime->tv_sec;
	ts[1].tv_nsec = mtime->tv_nsec;

	/*
	 * As SunPro make and gmake call sccs
	 * get when the time if s.file equals
	 * the time stamp of the g-file, make
	 * sure the g-file is a bit younger.
	 */
	if (!(pkt->p_flags & PF_V6)) {
		struct timespec	tn;

		getnstimeofday(&tn);
		ts[1].tv_nsec = tn.tv_nsec;
	}
	if (ts[1].tv_nsec <= 500000000)
		ts[1].tv_nsec += 499999999;

	utimensat(AT_FDCWD, gfile, ts, 0);
}
