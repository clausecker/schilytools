/* @(#)fileio.c	1.14 18/08/26 Copyright 1984-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)fileio.c	1.14 18/08/26 Copyright 1984-2018 J. Schilling";
#endif
/*
 *	Low level routines for Input/Output from/to files.
 *
 *	Copyright (c) 1984-2018 J. Schilling
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

#include "ved.h"
#include <schily/fcntl.h>
#include <schily/varargs.h>

EXPORT	int	stmpfmodes	__PR((Uchar *name));
EXPORT	FILE	*openerrmsg	__PR((Uchar *name, char *mode));
EXPORT	FILE	*opencomerr	__PR((ewin_t *wp, Uchar *name, char *mode));
EXPORT	FILE	*opensyserr	__PR((ewin_t *wp, Uchar *name, char *mode));
EXPORT	int	readsyserr	__PR((ewin_t *wp, FILE * f, void *buf, int len,
								Uchar *name));
EXPORT	int	writesyserr	__PR((ewin_t *wp, FILE * f, void *buf, int len,
								Uchar *name));
EXPORT	int	writebsyserr	__PR((ewin_t *wp, FILE * f, void *buf, int len,
								Uchar *name));
EXPORT	int	writeerrmsg	__PR((ewin_t *wp, FILE * f, void *buf, int len,
								Uchar *name));
EXPORT	void	exitcomerr	__PR((ewin_t *wp, char *fmt, ...));
EXPORT	void	excomerrno	__PR((ewin_t *wp, int err, char *fmt, ...));

/*
 * Make temporary files readable only by the user.
 */
EXPORT int
stmpfmodes(name)
	Uchar	*name;
{
	if (chmod(C name, 0600) < 0)
		return (-1);
	return (0);
}

/*
 * Open a file, print error message on failure.
 * Used only during startup of ved.
 */
EXPORT FILE *
openerrmsg(name, mode)
	Uchar	*name;
	char	*mode;
{
	FILE	*f;

	if ((f = fileopen(C name, mode)) == (FILE *) NULL)
		errmsg("Can not open '%s'.\r\n", name);
	return (f);
}

/*
 * Open a file, print error message and exit on failure.
 * Used only during startup and exit of ved.
 */
EXPORT FILE *
opencomerr(wp, name, mode)
	ewin_t	*wp;
	Uchar	*name;
	char	*mode;
{
	FILE	*f;

	if ((f = fileopen(C name, mode)) == (FILE *) NULL)
		exitcomerr(wp, "Can not open '%s'.\r\n", name);
	return (f);
}

/*
 * Open a file, print error message on the system line on failure.
 * Used only when the terminal is in edit mode.
 */
EXPORT FILE *
opensyserr(wp, name, mode)
	ewin_t	*wp;
	Uchar	*name;
	char	*mode;
{
	FILE	*f;

	if ((f = fileopen(C name, mode)) == (FILE *) NULL)
		write_errno(wp, "CAN'T OPEN %s", name);
	return (f);
}

/*
 * Read from file, print error message on the system line on failure.
 * Used only when the terminal is in edit mode.
 */
EXPORT int
readsyserr(wp, f, buf, len, name)
	ewin_t	*wp;
	FILE	*f;
	void	*buf;
	int	len;
	Uchar	*name;
{
	int	result;

	if ((result = ffileread(f, C buf, len)) < 0)	/* read unbuffered */
		write_errno(wp, "CAN'T READ %s", name);
	return (result);
}

/*
 * Write to file, print error message on the system line on failure.
 * Used only when the terminal is in edit mode.
 */
EXPORT int
writesyserr(wp, f, buf, len, name)
	ewin_t	*wp;
	FILE	*f;
	void	*buf;
	int	len;
	Uchar	*name;
{
	int	result;

	if ((result = ffilewrite(f, C buf, len)) != len)	/* write unbuffered */
		write_errno(wp, "CAN'T WRITE %s", name);
	return (result);
}

/*
 * Write to file, print error message on the system line on failure.
 * Used only when the terminal is in edit mode.
 * XXX Buffered version only used in edtmops.c
 */
EXPORT int
writebsyserr(wp, f, buf, len, name)
	ewin_t	*wp;
	FILE	*f;
	void	*buf;
	int	len;
	Uchar	*name;
{
	int	result;

	if ((result = filewrite(f, C buf, len)) != len)
		write_errno(wp, "CAN'T WRITE %s", name);
	return (result);
}

/*
 * Write to file, print error message on the system line on failure.
 * Used only when the terminal is in edit mode.
 * XXX Buffered version only used in edtmops.c
 */
/* ARGSUSED */
EXPORT int
writeerrmsg(wp, f, buf, len, name)
	ewin_t	*wp;
	FILE	*f;
	void	*buf;
	int	len;
	Uchar	*name;
{
	int	result;

	if ((result = filewrite(f, C buf, len)) != len)
		errmsg("Can't write %s", name);
	return (result);
}

/*
 * Reset terminal, do tmp file cleanup, print error message and exit.
 * Use system error number.
 */
/* PRINTFLIKE2 */
#ifdef	PROTOTYPES
EXPORT void
exitcomerr(ewin_t *wp, char *fmt, ...)
#else
EXPORT void
exitcomerr(wp, fmt, va_alist)
	ewin_t	*wp;
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;

	int err = geterrno();
	rsttmodes(wp);
	tmpcleanup(wp, FALSE);
#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	comerrno(err, "%r", fmt, args);
	va_end(args);
}

/*
 * Reset terminal, do tmp file cleanup, print error message and exit.
 * Use user privided error number.
 */
/* PRINTFLIKE3 */
#ifdef	PROTOTYPES
EXPORT void
excomerrno(ewin_t *wp, int err, char *fmt, ...)
#else
EXPORT void
excomerrno(wp, err, fmt, va_alist)
	ewin_t	*wp;
	int	err;
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;

	rsttmodes(wp);
	tmpcleanup(wp, FALSE);
#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	comerrno(err, "%r", fmt, args);
	va_end(args);
}
