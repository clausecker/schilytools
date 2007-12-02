/* @(#)stdlib.h	1.7 06/09/13 Copyright 1996 J. Schilling */
/*
 *	Definitions for stdlib
 *
 *	Copyright (c) 1996 J. Schilling
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

#ifndef _SCHILY_STDLIB_H
#define	_SCHILY_STDLIB_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_STDLIB_H
#ifndef	_INCL_STDLIB_H
#include <stdlib.h>
#define	_INCL_STDLIB_H
#endif
#else

extern	char	*malloc();
extern	char	*realloc();

extern	double	atof();

#endif	/* HAVE_STDLIB_H */

#ifndef	EXIT_FAILURE
#define	EXIT_FAILURE	1
#endif
#ifndef	EXIT_SUCCESS
#define	EXIT_SUCCESS	0
#endif
#ifndef	RAND_MAX
#define	RAND_MAX	32767
#endif

#endif	/* _SCHILY_STDLIB_H */
