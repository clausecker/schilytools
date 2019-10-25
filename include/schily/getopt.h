/* @(#)getopt.h	1.2 19/10/23 Copyright 2018-2019 J. Schilling */
/*
 *	Definitions for the enhanced getopt() from libgetopt
 *
 *	Copyright (c) 2018-2019 J. Schilling
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

#ifndef	_SCHILY_GETOPT_H
#define	_SCHILY_GETOPT_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_UNISTD_H
#include <schily/unistd.h>	/* This is where the standard puts getopt() */
#endif

#ifdef	HAVE_GETOPT_H
#ifndef	_INCL_GETOPT_H
#define	_INCL_GETOPT_H
#include <getopt.h>		/* This may deliver getopt_long() */
#endif
#endif	/* HAVE_GETOPT_H */

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * The POSIX variables for getopt():
 *
 * Init value	name	explanation
 * 1		optind	Next argv[] index to scan, argv index of failing option
 * 1		opterr	If 0, do not print error messages from within getopt()
 * 0		optopt	The option character that caused the error.
 * NULL		optarg	Pointer to the option argument.
 */
extern	int	optind;
extern	int	opterr;
extern	int	optopt;
extern	char	*optarg;

/*
 * Additional external variables defined by the AT&T getopt().
 *
 * The original from AT&T used _sp, but we changed that to opt_sp to make sure
 * that nobody is able to link an updated Solaris program with getopt() from
 * the old Solaris libc variants.
 *
 * The initial value is 1, since that is the first option character after '-'.
 */
extern	int	opt_sp;		/* Current index for combined option strings */

/*
 * Additional external variables defined by Schily getopt() extensions.
 *
 * The initial value is 0.
 */
extern	int	optflags;

/*
 * Definitions for optflags...
 */
#define	OPT_PLUS	1	/* The current option is of type +o, not -o */

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_GETOPT_H */
