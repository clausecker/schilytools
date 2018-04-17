/* @(#)fpoff.c	1.20 18/04/09 Copyright 1988-2018 J. Schilling */
/*
 *	Get frame pointer offset
 *
 *	Copyright (c) 1988-2018 J. Schilling
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
#include <schily/standard.h>
#include <schily/schily.h>

#if defined(sparc) && defined(__GNUC__)
#	define	FP_OFF		0x10	/* some strange things on sparc gcc */
#else
#	define	FP_OFF		0
#endif

EXPORT	void	**___fpoff	__PR((char *cp));

/*
 * Don't make it static to avoid inline optimization.
 *
 * We need this function to fool GCCs check for returning addresses
 * from outside the functions local address space.
 */
EXPORT void **
___fpoff(cp)
	char	*cp;
{
	long ***lp;

	lp = (long ***)(cp + FP_OFF);
	lp++;
	return ((void **)lp);
}
