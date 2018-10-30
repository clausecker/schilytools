/* @(#)ovstrcpy.c	1.1 18/10/14 Copyright 2009-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)ovstrcpy.c	1.1 18/10/14 Copyright 2009-2018 J. Schilling";
#endif
/*
 *	A strcpy() that works with overlapping buffers
 *
 *	Copyright (c) 2009-2018 J. Schilling
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

#include <schily/standard.h>
#include <schily/schily.h>

EXPORT	char	*ovstrcpy	__PR((char *s1, const char *s2));

EXPORT char *
ovstrcpy(s1, s2)
	register char		*s1;
	register const char	*s2;
{
	char	 *ret	= s1;

	while ((*s1++ = *s2++) != '\0')
		;

	return (ret);
}
