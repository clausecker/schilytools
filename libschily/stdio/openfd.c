/* @(#)openfd.c	1.10 06/11/28 Copyright 1986, 1995 J. Schilling */
/*
 *	Copyright (c) 1986, 1995 J. Schilling
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

#include "schilyio.h"

#if	defined(_openfd) && !defined(USE_LARGEFILES)
#	undef	_openfd
#endif

EXPORT int
_openfd(name, omode)
	const char	*name;
	int		omode;
{
	return (open(name, omode, (mode_t)0666));
}
