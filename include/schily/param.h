/* @(#)param.h	1.5 09/07/14 Copyright 2006-2007 J. Schilling */
/*
 *	Abstraction from sys/param.h
 *
 *	Copyright (c) 2006-2007 J. Schilling
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

#ifndef _SCHILY_PARAM_H
#define	_SCHILY_PARAM_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

/*
 * Let us include system defined types first.
 */
#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif
#ifndef	_SCHILY_LIMITS_H
#include <schily/limits.h>	/* For _SC_CLK_TCK */
#endif
#ifndef	_SCHILY_UNISTD_H
#include <schily/unistd.h>	/* For _SC_CLK_TCK */
#endif

#ifdef	HAVE_SYS_PARAM_H
#ifndef	_INCL_SYS_PARAM_H
#include <sys/param.h>
#define	_INCL_SYS_PARAM_H
#endif
#endif

#ifndef	NBBY
#define	NBBY		8	/* Number of bits per byte */
#endif

#ifndef	DEV_BSIZE
#define	DEV_BSIZE	512	/* UNIX Device Block size */
#endif

/*
 * NODEV may be in sys/param.h keep this definition past the include.
 */
#ifndef	NODEV
#define	NODEV	((dev_t)-1L)
#endif

#ifndef	HZ
#if	defined(_SC_CLK_TCK)
#define	HZ	((clock_t)sysconf(_SC_CLK_TCK))
#else
#define	HZ	100
#endif
#endif

#endif	/* _SCHILY_PARAM_H */
