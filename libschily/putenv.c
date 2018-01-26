/* @(#)putenv.c	1.1 18/01/17 Copyright 1999-2018 J. Schilling */
/*
 *	Our putenv implementation for systems that don't have it.
 *	Note that this implementation may not work correctly on platforms
 *	that support multi threading.
 *
 *	Copyright (c) 1999-2018 J. Schilling
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
#include <schily/stdlib.h>
#include <schily/schily.h>

#ifndef	HAVE_PUTENV

EXPORT	int	putenv		__PR((char *new));
LOCAL	int	ev_find		__PR((const char *s));

LOCAL	BOOL	ealloc = FALSE;	/* TRUE if environ is already allocated */

EXPORT int
putenv(new)
	char		*new;
{
	char 		**newenv;
	register int	idx;

	if ((idx = ev_find(new)) >= 0) {
		/*
		 * An old entry with the same name exists, replace it.
		 */
		environ[idx] = (char *)new;
	} else {
		/*
		 * If idx is < 0, we need to expand environ for the new entry.
		 * In this case -idx is the inverted size of the old environ.
		 */
		idx = -idx + 1;		/* Add space for new entry */
		if (ealloc) {
			/*
			 * environ is allocated, expand with realloc
			 */
			newenv = (char **)realloc(environ, idx*sizeof (char *));
			if (newenv == NULL)
				return (-1);
		} else {
			/*
			 * environ is orig space, copy to malloc'ed space
			 */
			ealloc = TRUE;
			newenv = (char **)malloc(idx*sizeof (char *));
			if (newenv == NULL)
				return (-1);
			(void) movebytes((char *)environ, (char *)newenv,
						(int)(idx*sizeof (char *)));
		}
		environ = newenv;
		environ[idx-2] = (char *)new;
		environ[idx-1] = NULL;
	}
	return (0);
}

/*
 * Check if arg of form name=value is part of environ.
 * Return index on success and -environ_size if not.
 */
LOCAL int
ev_find(s)
	register const char	*s;
{
	register int		i = 0;
	register const char	*ep;
	register const char	*s2;

	for (i = 0; environ[i] != NULL; i++) {
		/*
		 * Find string in environment entry.
		 */
		for (ep = environ[i], s2 = s; *ep == *s2++; ep++) {
			if (*ep == '=')
				return (i);
		}
	}
	return (-(++i));
}
#endif	/* HAVE_PUTENV */
