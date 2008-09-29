/* @(#)gettimeofday.c	1.1 08/06/14 Copyright 2007 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)gettimeofday.c	1.1 08/06/14 Copyright 2007 J. Schilling";
#endif
/*
 *	Emulate gettimeofday where it does not exist
 *
 *	Copyright (c) 2007 J. Schilling
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

#if	!defined(HAVE_GETTIMEOFDAY) && defined(_MSC_VER)
#include <windows.h>
#include <schily/time.h>

const	__int64 MS_FTIME_ADD	= 0x2b6109100i64; 
const	__int64 MS_FTIME_SECS	= 10000000i64; 

EXPORT int
gettimeofday(tp, dummy)
	struct timeval	*tp;
	void		*dummy;
{
	FILETIME	ft;
	__int64		T;

	if (tp == 0)
		return (0);

	GetSystemTimeAsFileTime(&ft);	/* 100ns time since 1601 */
	T   = ft.dwHighDateTime;
	T <<= 32;
	T  += ft.dwLowDateTime;

	tp->tv_sec  = T / MS_FTIME_SECS - MS_FTIME_ADD;
	tp->tv_usec = (T % MS_FTIME_SECS) / 10);

	return (0);
}
#endif
