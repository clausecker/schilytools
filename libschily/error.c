/* @(#)error.c	1.14 06/09/13 Copyright 1985, 1989, 1995-2003 J. Schilling */
/*
 *	fprintf() on standard error stdio stream
 *
 *	Copyright (c) 1985, 1989, 1995-2003 J. Schilling
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
#include <stdio.h>
#include <schily/standard.h>
#include <schily/varargs.h>
#include <schily/schily.h>

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
