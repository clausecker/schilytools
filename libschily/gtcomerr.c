/* @(#)gtcomerr.c	1.1 18/06/16 Copyright 1985-1989, 1995-2018 J. Schilling */
/*
 *	Routines for printing command errors
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

EXPORT	void	gtcomerr	__PR((const char *, ...));
EXPORT	void	gtxcomerr	__PR((int, const char *, ...));
EXPORT	void	gtcomerrno	__PR((int, const char *, ...));
EXPORT	void	gtxcomerrno	__PR((int, int, const char *, ...));
EXPORT	int	gterrmsg	__PR((const char *, ...));
EXPORT	int	gterrmsgno	__PR((int, const char *, ...));

/*
 * Fetch current errno, print a related message and exit(errno).
 */
/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT void
gtcomerr(const char *msg, ...)
#else
EXPORT void
gtcomerr(msg, va_alist)
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
	(void) _comerr(stderr, COMERR_EXIT, 0, geterrno(), _(msg), args);
	/* NOTREACHED */
	va_end(args);
}

/*
 * Fetch current errno, print a related message and exit(exc).
 */
/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT void
gtxcomerr(int exc, const char *msg, ...)
#else
EXPORT void
gtxcomerr(exc, msg, va_alist)
	int	exc;
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
	(void) _comerr(stderr, COMERR_EXCODE, exc, geterrno(), _(msg), args);
	/* NOTREACHED */
	va_end(args);
}

/*
 * Print a message related to err and exit(err).
 */
/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT void
gtcomerrno(int err, const char *msg, ...)
#else
EXPORT void
gtcomerrno(err, msg, va_alist)
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
	(void) _comerr(stderr, COMERR_EXIT, 0, err, _(msg), args);
	/* NOTREACHED */
	va_end(args);
}

/*
 * Print a message related to err and exit(exc).
 */
/* VARARGS3 */
#ifdef	PROTOTYPES
EXPORT void
gtxcomerrno(int exc, int err, const char *msg, ...)
#else
EXPORT void
gtxcomerrno(exc, err, msg, va_alist)
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
	(void) _comerr(stderr, COMERR_EXCODE, exc, err, _(msg), args);
	/* NOTREACHED */
	va_end(args);
}

/*
 * Fetch current errno, print a related message and return(errno).
 */
/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT int
gterrmsg(const char *msg, ...)
#else
EXPORT int
gterrmsg(msg, va_alist)
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
	ret = _comerr(stderr, COMERR_RETURN, 0, geterrno(), _(msg), args);
	va_end(args);
	return (ret);
}

/*
 * Print a message related to err and return(err).
 */
/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT int
gterrmsgno(int err, const char *msg, ...)
#else
EXPORT int
gterrmsgno(err, msg, va_alist)
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
	ret = _comerr(stderr, COMERR_RETURN, 0, err, _(msg), args);
	va_end(args);
	return (ret);
}
