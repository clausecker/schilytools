/* @(#)getcwd.h	1.4 06/09/13 Copyright 1998 J. Schilling */
/*
 *	Definitions for getcwd()
 *
 *	Copyright (c) 1998 J. Schilling
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

#ifndef	_SCHILY_GETCWD_H
#define	_SCHILY_GETCWD_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef JOS
#	ifndef	_INCL_SYS_STYPES_H
#	include <sys/stypes.h>
#	define	_INCL_SYS_STYPES_H
#	endif
	extern char	*gwd();
#	define		getcwd(b, len)	gwd(b)
#else
#	ifndef	HAVE_GETCWD
#		define	getcwd(b, len)	getwd(b)
#	endif
#endif

#endif	/* _SCHILY_GETCWD_H */
