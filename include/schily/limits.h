/* @(#)limits.h	1.8 19/02/28 Copyright 2011-2019 J. Schilling */
/*
 *	Copyright (c) 2011-2019 J. Schilling
 */
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

#ifndef	_SCHILY_LIMITS_H
#define	_SCHILY_LIMITS_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_LIMITS_H
#ifndef	_INCL_LIMITS_H
#include <limits.h>
#define	_INCL_LIMITS_H
#endif
#endif

#ifndef	COLL_WEIGHTS_MAX
#define	COLL_WEIGHTS_MAX	2
#endif

#ifndef	_POSIX2_LINE_MAX
#define	_POSIX2_LINE_MAX	2048
#endif

/*
 * Include sys/param.h for PIPE_BUF
 */
#ifndef	_SCHILY_PARAM_H
#include <schily/param.h>
#endif

#ifndef	PIPE_BUF
#if	defined(__MINGW32__) || defined(_MSC_VER)
#define	PIPE_BUF		5120
#else
#define	PIPE_BUF		512
#endif
#endif	/* PIPE_BUF */

/*
 * For definitions of TYPE_MINVAL() ...
 */
#ifndef	_SCHILY_TYPE_VAL_H
#include <schily/type_val.h>
#endif

#ifndef	INT_MIN
#define	INT_MIN		TYPE_MINVAL(int)
#endif
#ifndef	INT_MAX
#define	INT_MAX		TYPE_MAXVAL(int)
#endif
#ifndef	UINT_MIN
#define	UINT_MIN	TYPE_MINVAL(unsigned int)
#endif
#ifndef	UINT_MAX
#define	UINT_MAX	TYPE_MAXVAL(unsigned int)
#endif

#ifndef	LONG_MIN
#define	LONG_MIN	TYPE_MINVAL(long)
#endif
#ifndef	LONG_MAX
#define	LONG_MAX	TYPE_MAXVAL(long)
#endif
#ifndef	ULONG_MIN
#define	ULONG_MIN	TYPE_MINVAL(unsigned long)
#endif
#ifndef	ULONG_MAX
#define	ULONG_MAX	TYPE_MAXVAL(unsigned long)
#endif

#ifndef	LLONG_MIN
#define	LLONG_MIN	TYPE_MINVAL(long)
#endif
#ifndef	LLONG_MAX
#define	LLONG_MAX	TYPE_MAXVAL(long long)
#endif
#ifndef	ULLONG_MIN
#define	ULLONG_MIN	TYPE_MINVAL(unsigned long long)
#endif
#ifndef	ULLONG_MAX
#define	ULLONG_MAX	TYPE_MAXVAL(unsigned long long)
#endif

#endif	/* _SCHILY_LIMITS_H */
