/* @(#)unsetenv.c	1.1 18/01/17 Copyright 2009-2018 J. Schilling */
/*
 *	Our unsetenv implementation for systems that don't have it.
 *	Note that this implementation may not work correctly on platforms
 *	that support multi threading.
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
#include <schily/unistd.h>
#include <schily/schily.h>

#ifndef	HAVE_UNSETENV

EXPORT	int	unsetenv		__PR((const char *name));

EXPORT int
unsetenv(name)
	const char	*name;
{
	register int		i = 0;
	register const char	*ep;
	register const char	*s2;

	if (name == NULL || name[0] == '\0')
		return (0);

	for (i = 0; environ[i] != NULL; i++) {
		/*
		 * Find string in environment entry.
		 */
		for (ep = environ[i], s2 = name; *ep++ == *s2++; )
			;
		if (*--ep == '=' && *--s2 == '\0')
			goto found;
	}
	return (0);
found:
	for (; environ[i] != NULL; i++)
		environ[i] = environ[i+1];
	return (0);
}
#endif	/* HAVE_UNSETENV */
