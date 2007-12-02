/* @(#)time.h	1.18 07/04/25 Copyright 1996-2007 J. Schilling */
/*
 *	Generic header for users of time(), gettimeofday() ...
 *
 *	It includes definitions for time_t, struct timeval, ...
 *
 *	Copyright (c) 1996-2007 J. Schilling
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

#ifndef	_SCHILY_TIME_H
#define	_SCHILY_TIME_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>	/* Needed for time_t		*/
#endif

#ifdef	TIME_WITH_SYS_TIME
#	ifndef	_INCL_SYS_TIME_H
#	include <sys/time.h>
#	define	_INCL_SYS_TIME_H
#	endif
#	ifndef	_INCL_TIME_H
#	include <time.h>
#	define	_INCL_TIME_H
#	endif
#else
#ifdef	HAVE_SYS_TIME_H
#	ifndef	_INCL_SYS_TIME_H
#	include <sys/time.h>
#	define	_INCL_SYS_TIME_H
#	endif
#else
#	ifndef	_INCL_TIME_H
#	include <time.h>
#	define	_INCL_TIME_H
#	endif
#endif
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef	timerclear
/*
 * With MSVC timerclear / struct timeval present in case that
 * winsock2.h has been included before.
 */
#undef	HAVE_STRUCT_TIMEVAL
#define	HAVE_STRUCT_TIMEVAL	1
#endif

#ifndef	HAVE_STRUCT_TIMEVAL

struct timeval {
	long	tv_sec;
	long	tv_usec;
};
#endif

#ifndef	HAVE_STRUCT_TIMEZONE

struct timezone {
	int	tz_minuteswest;
	int	tz_dsttime;
};
#endif

#undef	timerclear
#define	timerclear(tvp)		(tvp)->tv_sec = (tvp)->tv_usec = 0

#undef	timerfix
#define	timerfix1(tvp)		while ((tvp)->tv_usec < 0) {		\
					(tvp)->tv_sec--;		\
					(tvp)->tv_usec += 1000000;	\
				}

#define	timerfix2(tvp)		while ((tvp)->tv_usec > 1000000) {	\
					(tvp)->tv_sec++;		\
					(tvp)->tv_usec -= 1000000;	\
				}

#define	timerfix(tvp)		do { timerfix1(tvp); timerfix2(tvp); } while (0)

/*
 * timersub() and timeradd() are defined on FreeBSD with a different
 * interface (3 parameters).
 */
#undef	timersub
#define	timersub(tvp1, tvp2)	do {					\
					(tvp1)->tv_sec -= (tvp2)->tv_sec; \
					(tvp1)->tv_usec -= (tvp2)->tv_usec; \
					timerfix1(tvp1); timerfix2(tvp1); \
				} while (0)

#undef	timeradd
#define	timeradd(tvp1, tvp2)	do {					\
					(tvp1)->tv_sec += (tvp2)->tv_sec; \
					(tvp1)->tv_usec += (tvp2)->tv_usec; \
					timerfix1(tvp1); timerfix2(tvp1); \
				} while (0)

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_TIME_H */
