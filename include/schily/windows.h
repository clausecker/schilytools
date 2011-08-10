/* @(#)windows.h	1.1 11/08/02 Copyright 2011 J. Schilling */
/*
 *	Definitions for windows.h
 *
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

#ifndef _SCHILY_WINDOWS_H
#define	_SCHILY_WINDOWS_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_WINDOWS_H
#ifndef	_INCL_WINDOWS_H

#ifdef	_MSC_VER	/* configure believes they are missing */
#undef	u_char
#undef	u_short
#undef	u_int
#undef	u_long
#endif

#define	BOOL	WBOOL		/* This is the Win BOOL		*/
#define	format	__ms_format	/* Avoid format parameter hides global ... */

#ifdef	timerclear		/* struct timeval has already been declared */
#define	timeval	__ms_timeval
#endif

#include <windows.h>

#undef	BOOL			/* MS Code uses WBOOL or #define BOOL WBOOL */
#undef	format			/* Return to previous definition */
#undef	timeval

#define	_INCL_WINDOWS_H
#endif

#endif	/* HAVE_WINDOWS_H */

#endif	/* _SCHILY_WINDOWS_H */
