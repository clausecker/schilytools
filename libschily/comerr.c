/* @(#)comerr.c	1.32 07/02/26 Copyright 1985-1989, 1995-2007 J. Schilling */
/*
 *	Routines for printing command errors
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

EXPORT	int	on_comerr	__PR((void (*fun)(int, void *), void *arg));
EXPORT	void	comerr		__PR((const char *, ...));
EXPORT	void	comerrno	__PR((int, const char *, ...));
EXPORT	int	errmsg		__PR((const char *, ...));
EXPORT	int	errmsgno	__PR((int, const char *, ...));
EXPORT	int	_comerr		__PR((FILE *, int, int, const char *, va_list));
EXPORT	void	comexit		__PR((int));
EXPORT	char	*errmsgstr	__PR((int));

typedef	struct ex {
	struct ex *next;
	void	(*func) __PR((int, void *));
	void	*arg;
} ex_t;

LOCAL	ex_t	*exfuncs;

EXPORT	int
on_comerr(func, arg)
	void	(*func) __PR((int, void *));
	void	*arg;
{
	ex_t	*fp;

	fp = malloc(sizeof (*fp));
	if (fp == NULL)
		return (-1);

	fp->func = func;
	fp->arg  = arg;
	fp->next = exfuncs;
	exfuncs = fp;
	return (0);
}

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT void
comerr(const char *msg, ...)
#else
EXPORT void
comerr(msg, va_alist)
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
	(void) _comerr(stderr, TRUE, geterrno(), msg, args);
	/* NOTREACHED */
	va_end(args);
}

/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT void
comerrno(int err, const char *msg, ...)
#else
EXPORT void
comerrno(err, msg, va_alist)
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
	(void) _comerr(stderr, TRUE, err, msg, args);
	/* NOTREACHED */
	va_end(args);
}

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT int
errmsg(const char *msg, ...)
#else
EXPORT int
errmsg(msg, va_alist)
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
	ret = _comerr(stderr, FALSE, geterrno(), msg, args);
	va_end(args);
	return (ret);
}

/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT int
errmsgno(int err, const char *msg, ...)
#else
EXPORT int
errmsgno(err, msg, va_alist)
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
	ret = _comerr(stderr, FALSE, err, msg, args);
	va_end(args);
	return (ret);
}

#ifdef	__BEOS__
	/*
	 * On BeOS errno is a big negative number (0x80000000 + small number).
	 * We assume that small negative numbers are safe to be used as special
	 * values that prevent printing the errno text.
	 *
	 * We tried to use #if EIO < 0 but this does not work because EIO is
	 * defined to a enum. ENODEV may work as ENODEV is defined to a number
	 * directly.
	 */
#define	silent_error(e)		((e) < 0 && (e) >= -1024)
#else
	/*
	 * On UNIX errno is a small non-negative number, so we assume that
	 * negative values cannot be a valid errno and don't print the error
	 * string in this case. However the value may still be used as exit()
	 * code if 'exflg' is set.
	 */
#define	silent_error(e)		((e) < 0)
#endif
EXPORT int
_comerr(f, exflg, err, msg, args)
	FILE		*f;
	int		exflg;
	int		err;
	const char	*msg;
	va_list		args;
{
	char	errbuf[20];
	char	*errnam;
	char	*prognam = get_progname();

	if (silent_error(err)) {
		js_fprintf(f, "%s: %r", prognam, msg, args);
	} else {
		errnam = errmsgstr(err);
		if (errnam == NULL) {
			(void) js_snprintf(errbuf, sizeof (errbuf),
						"Error %d", err);
			errnam = errbuf;
		}
		js_fprintf(f, "%s: %s. %r", prognam, errnam, msg, args);
	}
	if (exflg) {
		comexit(err);
		/* NOTREACHED */
	}
	return (err);
}

EXPORT void
comexit(err)
	int	err;
{
	while (exfuncs) {
		(*exfuncs->func)(err, exfuncs->arg);
		exfuncs = exfuncs->next;
	}
	exit(err);
	/* NOTREACHED */
}

EXPORT char *
errmsgstr(err)
	int	err;
{
#ifdef	HAVE_STRERROR
	/*
	 * POSIX compliance may look strange...
	 */
	int	errsav = geterrno();
	char	*ret;

	seterrno(0);
	ret = strerror(err);
	err = geterrno();
	seterrno(errsav);

	if (ret == NULL || err)
		return (NULL);
	return (ret);
#else
	if (err < 0 || err >= sys_nerr) {
		return (NULL);
	} else {
		return (sys_errlist[err]);
	}
#endif
}
