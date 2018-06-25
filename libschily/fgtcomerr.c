/* @(#)fgtcomerr.c	1.1 18/06/16 Copyright 1985-1989, 1995-2018 J. Schilling */
/*
 *	Routines for printing command errors on a specified FILE *
 *	These routines call gettext() on the message first.
 *
 *	Copyright (c) 1985-1989, 1995-2018 J. Schilling
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
#include <schily/unistd.h>	/* include <sys/types.h> try to get size_t */
#include <schily/stdio.h>	/* Try again for size_t	*/
#include <schily/stdlib.h>	/* Try again for size_t	*/
#include <schily/standard.h>
#include <schily/varargs.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/errno.h>
#include <schily/nlsdefs.h>

EXPORT	void	fgtcomerr	__PR((FILE *, const char *, ...));
EXPORT	void	fgtxcomerr	__PR((FILE *, int, const char *, ...));
EXPORT	void	fgtcomerrno	__PR((FILE *, int, const char *, ...));
EXPORT	void	fgtxcomerrno	__PR((FILE *, int, int, const char *, ...));
EXPORT	int	fgterrmsg	__PR((FILE *, const char *, ...));
EXPORT	int	fgterrmsgno	__PR((FILE *, int, const char *, ...));

/*
 * Fetch current errno, print a related message and exit(errno).
 */
/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT void
fgtcomerr(FILE *f, const char *msg, ...)
#else
EXPORT void
fgtcomerr(f, msg, va_alist)
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
	(void) _comerr(f, COMERR_EXIT, 0, geterrno(), _(msg), args);
	/* NOTREACHED */
	va_end(args);
}

/*
 * Fetch current errno, print a related message and exit(exc).
 */
/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT void
fgtxcomerr(FILE *f, int exc, const char *msg, ...)
#else
EXPORT void
fgtxcomerr(f, exc, msg, va_alist)
	int	exc;
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
	(void) _comerr(f, COMERR_EXCODE, exc, geterrno(), _(msg), args);
	/* NOTREACHED */
	va_end(args);
}

/*
 * Print a message related to err and exit(err).
 */
/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT void
fgtcomerrno(FILE *f, int err, const char *msg, ...)
#else
EXPORT void
fgtcomerrno(f, err, msg, va_alist)
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
	(void) _comerr(f, COMERR_EXIT, 0, err, _(msg), args);
	/* NOTREACHED */
	va_end(args);
}

/*
 * Print a message related to err and exit(exc).
 */
/* VARARGS3 */
#ifdef	PROTOTYPES
EXPORT void
fgtxcomerrno(FILE *f, int exc, int err, const char *msg, ...)
#else
EXPORT void
fgtxcomerrno(f, exc, err, msg, va_alist)
	FILE	*f;
	int	exc;
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
	(void) _comerr(f, COMERR_EXCODE, exc, err, _(msg), args);
	/* NOTREACHED */
	va_end(args);
}

/*
 * Fetch current errno, print a related message and return(errno).
 */
/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT int
fgterrmsg(FILE *f, const char *msg, ...)
#else
EXPORT int
fgterrmsg(f, msg, va_alist)
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
	ret = _comerr(f, COMERR_RETURN, 0, geterrno(), _(msg), args);
	va_end(args);
	return (ret);
}

/*
 * Print a message related to err and return(err).
 */
/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT int
fgterrmsgno(FILE *f, int err, const char *msg, ...)
#else
EXPORT int
fgterrmsgno(f, err, msg, va_alist)
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
	ret = _comerr(f, COMERR_RETURN, 0, err, _(msg), args);
	va_end(args);
	return (ret);
}
