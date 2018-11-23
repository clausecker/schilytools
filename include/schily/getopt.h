/* @(#)getopt.h	1.1 18/11/08 Copyright 2018 J. Schilling */
/*
 *	Definitions for the enhanced getopt() from libgetopt
 *
 *	Copyright (c) 2018 J. Schilling
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
 * Additional external variables defined by the AT&T getopt().
 */
extern	int	opt_sp;

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_GETOPT_H */
