/* @(#)getdtablesize.c	1.1 11/08/14 Copyright 2011 J. Schilling */
/*
 *	Copyright (c) 2011 J. Schilling
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
#include <schily/standard.h>
#include <schily/types.h>
#include <schily/limits.h>
#include <schily/resource.h>

#ifndef	HAVE_GETDTABLESIZE
EXPORT int
getdtablesize()
{
#ifdef	RLIMIT_NOFILE
	struct rlimit	rlim;

	getrlimit(RLIMIT_NOFILE, &rlim);
	return (rlim.rlim_cur);
#else	/* RLIMIT_NOFILE */
#ifdef	_MSC_VER
	return (2048);
#else
#ifdef	OPEN_MAX
	return (OPEN_MAX);
#else
	return (20);
#endif
#endif
#endif	/* RLIMIT_NOFILE */
}
#endif
