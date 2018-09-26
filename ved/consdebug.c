/* @(#)consdebug.c	1.23 18/09/20 Copyright 1986-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)consdebug.c	1.23 18/09/20 Copyright 1986-2009 J. Schilling";
#endif
/*
 *	Print debugging messages to the console or to "VED_DBGTERM" environment
 *
 *	Copyright (c) 1986-2009 J. Schilling
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

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/fcntl.h>
#include <schily/string.h>
#include <schily/varargs.h>
#include <schily/schily.h>

EXPORT	void	cdbg		__PR((char *fmt, ...));
EXPORT	int	writecons	__PR((char *s));
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
	(void) writecons(buf);
}

/*
 * Write to the console or to "VED_DBGTERM" environment.
 * Open and close the device for every access.
 * The return code is to fool GCC as there are broken GCC
 * configurations on Linux where (void) write() results in a warning.
 */
EXPORT int
writecons(s)
	char	*s;
{
	static	int	f;
		char	*cname;
		int	r;

	if (f == 0) {
		if ((cname = getenv("VED_DBGTERM")) == NULL)
			cname = "/dev/console";

		f = open(cname, 1);
		if (f == 0)
			return (0);
	}

	r =  write(f, s, strlen(s));
	r += write(f, "\n", 1);
	return (r);
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
EXPORT int
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
