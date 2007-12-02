/* @(#)fcomerr.c	1.1 07/02/26 Copyright 1985-1989, 1995-2007 J. Schilling */
/*
 *	Routines for printing command errors on a specified FILE *
 *
 *	Copyright (c) 1985-1989, 1995-2007 J. Schilling
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
#include <schily/unistd.h>	/* include <sys/types.h> try to get size_t */
#include <stdio.h>		/* Try again for size_t	*/
#include <schily/stdlib.h>	/* Try again for size_t	*/
#include <schily/standard.h>
#include <schily/varargs.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/errno.h>

EXPORT	void	fcomerr		__PR((FILE *, const char *, ...));
EXPORT	void	fcomerrno	__PR((FILE *, int, const char *, ...));
EXPORT	int	ferrmsg		__PR((FILE *, const char *, ...));
EXPORT	int	ferrmsgno	__PR((FILE *, int, const char *, ...));
/*extern	int	_comerr		__PR((FILE *, int, int, const char *, va_list));*/

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT void
fcomerr(FILE *f, const char *msg, ...)
#else
EXPORT void
fcomerr(f, msg, va_alist)
	FILE	*f;
	char	*msg;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	(void) _comerr(f, TRUE, geterrno(), msg, args);
	/* NOTREACHED */
	va_end(args);
}

/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT void
fcomerrno(FILE *f, int err, const char *msg, ...)
#else
EXPORT void
fcomerrno(f, err, msg, va_alist)
	FILE	*f;
	int	err;
	char	*msg;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	(void) _comerr(f, TRUE, err, msg, args);
	/* NOTREACHED */
	va_end(args);
}

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT int
ferrmsg(FILE *f, const char *msg, ...)
#else
EXPORT int
ferrmsg(f, msg, va_alist)
	FILE	*f;
	char	*msg;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	ret = _comerr(f, FALSE, geterrno(), msg, args);
	va_end(args);
	return (ret);
}

/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT int
ferrmsgno(FILE *f, int err, const char *msg, ...)
#else
EXPORT int
ferrmsgno(f, err, msg, va_alist)
	FILE	*f;
	int	err;
	char	*msg;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	ret = _comerr(f, FALSE, err, msg, args);
	va_end(args);
	return (ret);
}
