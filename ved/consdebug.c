/* @(#)consdebug.c	1.18 06/09/13 Copyright 1986-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)consdebug.c	1.18 06/09/13 Copyright 1986-2004 J. Schilling";
#endif
/*
 *	Print debugging messages to the console or to "VED_DBGTERM" environment
 *
 *	Copyright (c) 1986-2004 J. Schilling
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
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/fcntl.h>
#include <schily/string.h>
#include <schily/varargs.h>
#include <schily/schily.h>

EXPORT	void	cdbg		__PR((char *fmt, ...));
EXPORT	void	writecons	__PR((char *s));
EXPORT	long	getcaller	__PR((void));

#ifndef	DEBUG
#define	DEBUG
#endif
/*#define	DEBUG_CALLER*/
#if !defined(sun)
#undef	DEBUG_CALLER
#endif

#ifdef	DEBUG

#ifdef	DEBUG_CALLER
#ifdef	FOPEN_MAX
#if	defined(__sun) && defined(__i386)
#include <sys/reg.h>
#endif
#include <sys/frame.h>		/* Bug in SYSV frame.h ist Prozessorabhängig*/
#else
#include <machine/frame.h>
#endif
#endif

/*
 * Do formatted debugging output to the console
 */
/* PRINTFLIKE1 */
#ifdef	PROTOTYPES
EXPORT void
cdbg(char *fmt, ...)
#else
EXPORT void
cdbg(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	char	buf[200];
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	snprintf(buf, sizeof (buf), "%r", fmt, args);
	va_end(args);
	writecons(buf);
}

/*
 * Write to the console or to "VED_DBGTERM" environment.
 * Open and close the device for every access.
 */
EXPORT void
writecons(s)
	char	*s;
{
	static	int	f;
		char	*cname;

	if (f == 0) {
		if ((cname = getenv("VED_DBGTERM")) == NULL)
			cname = "/dev/console";

		f = open(cname, 1);
		if (f == 0)
			return;
	}

	write(f, s, strlen(s));
	write(f, "\n", 1);
}

/*
 * Try to get calling function's address
 */
#ifdef	DEBUG_CALLER
EXPORT long
getcaller()
{
	/*
	 * Return saved pc of previous frame to getcaller()
	 * As the SCO OpenServer C-Compiler has a bug that may cause
	 * the first function call to getfp() been done before the
	 * new stack frame is created, we call getfp() twice.
	 */
	(void) getfp();
/*	return (((struct frame *)getfp())->fr_savfp->fr_savpc);*/
	return (((struct frame *)(((struct frame *)getfp())->fr_savfp))->fr_savpc);
}
#else
EXPORT long
getcaller()
{
	return (0);
}
#endif

#else

/* ARGSUSED */
#ifdef	PROTOTYPES
EXPORT void
cdbg(char *fmt, ...)
#else
EXPORT void
cdbg(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
}

/* ARGSUSED */
EXPORT void
writecons(s)
	char	*s;
{
}

EXPORT long
getcaller()
{
	return (0);
}

#endif
