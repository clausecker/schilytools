/* @(#)misc.c	1.5 06/09/13 Copyright 1998, 2001-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)misc.c	1.5 06/09/13 Copyright 1998, 2001-2004 J. Schilling";
#endif
/*
 *	Misc support functions
 *
 *	Copyright (c) 1998, 2001-2004 J. Schilling
 */
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

#include <schily/mconfig.h>
#include <schily/time.h>
#include <stdio.h>
#include <schily/standard.h>
#include <schily/schily.h>

EXPORT	void	timevaldiff	__PR((struct timeval *start, struct timeval *stop));
EXPORT	void	prtimediff	__PR((const char *fmt,
					struct timeval *start,
					struct timeval *stop));

EXPORT void
timevaldiff(start, stop)
	struct timeval	*start;
	struct timeval	*stop;
{
	struct timeval tv;

	tv.tv_sec = stop->tv_sec - start->tv_sec;
	tv.tv_usec = stop->tv_usec - start->tv_usec;
	while (tv.tv_usec > 1000000) {
		tv.tv_usec -= 1000000;
		tv.tv_sec += 1;
	}
	while (tv.tv_usec < 0) {
		tv.tv_usec += 1000000;
		tv.tv_sec -= 1;
	}
	*stop = tv;
}

EXPORT void
prtimediff(fmt, start, stop)
	const	char	*fmt;
	struct timeval	*start;
	struct timeval	*stop;
{
	struct timeval tv;

	tv.tv_sec = stop->tv_sec - start->tv_sec;
	tv.tv_usec = stop->tv_usec - start->tv_usec;
	while (tv.tv_usec > 1000000) {
		tv.tv_usec -= 1000000;
		tv.tv_sec += 1;
	}
	while (tv.tv_usec < 0) {
		tv.tv_usec += 1000000;
		tv.tv_sec -= 1;
	}
	/*
	 * We need to cast timeval->* to long because
	 * of the broken sys/time.h in Linux.
	 */
	printf("%s%4ld.%03lds\n", fmt, (long)tv.tv_sec, (long)tv.tv_usec/1000);
	flush();
}
