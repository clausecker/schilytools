/* @(#)values.h	1.1 18/12/30 Copyright 2018 J. Schilling */
/*
 *	Abstraction code for values.h
 *
 *	Copyright (c) 2018 J. Schilling
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

#ifndef	_SCHILY_VALUES_H
#define	_SCHILY_VALUES_H

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_FLOAT_H
#ifndef	_INCL_FLOAT_H
#define	_INCL_FLOAT_H
#include <float.h>
#endif
#endif

#ifdef	HAVE_VALUES_H
#ifndef	_INCL_VALUES_H
#define	_INCL_VALUES_H
#include <values.h>
#endif
#endif

#ifndef	BITSPERBYTE
#define	BITSPERBYTE	8
#endif

#ifndef	BITS
#define	BITS(type)	(BITSPERBYTE * (long)sizeof (type))
#endif

#ifndef	HIBITS
#define	HIBITS		((short)(1 << (BITS(short) - 1)))
#endif

#if defined(__STDC__)
#ifndef	HIBITI
#define	HIBITI		(1U << (BITS(int) - 1))
#endif
#ifndef	HIBITL
#define	HIBITL		(1UL << (BITS(long) - 1))
#endif
#else
#ifndef	HIBITI
#define	HIBITI		((unsigned)1 << (BITS(int) - 1))
#endif
#ifndef	HIBITL
#define	HIBITL		(1L << (BITS(long) - 1))
#endif
#endif

#ifndef	MAXSHORT
#define	MAXSHORT	((short)~HIBITS)
#endif
#ifndef	MAXINT
#define	MAXINT		((int)(~HIBITI))
#endif
#ifndef	MAXLONG
#define	MAXLONG		((long)(~HIBITL))
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_VALUES_H */
