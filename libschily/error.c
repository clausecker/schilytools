/* @(#)error.c	1.16 15/03/29 Copyright 1985, 1989, 1995-2015 J. Schilling */
/*
 *	fprintf() on standard error stdio stream
 *
 *	Copyright (c) 1985, 1989, 1995-2015 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/varargs.h>
#include <schily/schily.h>

#undef	error
#ifdef	HAVE_PRAGMA_WEAK
#pragma	weak error =	js_error
#else
/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT int
error(const char *fmt, ...)
#else
EXPORT int
error(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	ret = js_fprintf(stderr, "%r", fmt, args);
	va_end(args);
	return (ret);
}
#endif

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT int
js_error(const char *fmt, ...)
#else
EXPORT int
js_error(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	ret = js_fprintf(stderr, "%r", fmt, args);
	va_end(args);
	return (ret);
}
