/* @(#)type_val.h	1.1 19/02/28 Copyright 2002-2019 J. Schilling */
/*
 *	Definitions to define the maximum and minimum values
 *	for all scalar types.
 *
 *	Copyright (c) 2002-2019 J. Schilling
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

#ifndef	_SCHILY_TYPE_VAL_H
#define	_SCHILY_TYPE_VAL_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

/*
 * Let us include system defined types too.
 */
#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif
/*
 * Include sys/param.h for NBBY - needed in case that CHAR_BIT is missing
 */
#ifndef	_SCHILY_PARAM_H
#include <schily/param.h>	/* Must be before limits.h */
#endif

/*
 * Include limits.h for CHAR_BIT - needed by TYPE_MINVAL(t) and  TYPE_MAXVAL(t)
 */
#ifndef	_SCHILY_LIMITS_H
#include <schily/limits.h>
#endif

#ifndef	CHAR_BIT
#ifdef	NBBY
#define	CHAR_BIT	NBBY
#endif
#endif

/*
 * Last resort: define CHAR_BIT by hand
 */
#ifndef	CHAR_BIT
#define	CHAR_BIT	8
#endif

/*
 * These macros may not work on all platforms but as we depend
 * on two's complement in many places, they do not reduce portability.
 * The macros below work with 2s complement and ones complement machines.
 * Verify with this table...
 *
 *	Bits	1's c.	2's complement.
 * 	100	-3	-4
 * 	101	-2	-3
 * 	110	-1	-2
 * 	111	-0	-1
 * 	000	+0	 0
 * 	001	+1	+1
 * 	010	+2	+2
 * 	011	+3	+3
 *
 * Computing -TYPE_MINVAL(type) will not work on 2's complement machines
 * if 'type' is int or more. Use:
 *		((unsigned type)(-1 * (TYPE_MINVAL(type)+1))) + 1;
 * it works for both 1's complement and 2's complement machines.
 */
#define	TYPE_ISSIGNED(t)	(((t)-1) < ((t)0))
#define	TYPE_ISUNSIGNED(t)	(!TYPE_ISSIGNED(t))
#if (-3 & 3) == 1		/* Two's complement */
#define TYPE_MSBVAL(t)		(2 * -(((t)1) << (sizeof (t)*CHAR_BIT - 2)))
#else
#define	TYPE_MSBVAL(t)		((t)(~((t)0) << (sizeof (t)*CHAR_BIT - 1)))
#endif
#define	TYPE_MINVAL(t)		(TYPE_ISSIGNED(t)			\
				    ? TYPE_MSBVAL(t)			\
				    : ((t)0))
#define	TYPE_MAXVAL(t)		((t)(~((t)0) - TYPE_MINVAL(t)))

#endif	/* _SCHILY_TYPE_VAL_H */
