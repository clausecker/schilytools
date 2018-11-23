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
/*
 * @(#)intree.c	1.1 18/11/07 Copyright 2018 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)intree.c 1.1 18/11/07 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)intree.c"
#pragma ident	"@(#)sccs:lib/comobj/intree.c"
#endif
#include	<defines.h>

EXPORT	BOOL	has_dotdot	__PR((const char *name));
EXPORT	BOOL	in_tree		__PR((const char *name));
EXPORT	void	make_relative	__PR((char *name));

/*
 * Check wheter a /../ sequence is in the file name.
 */
EXPORT BOOL
has_dotdot(name)
	const char	*name;
{
	register const char	*p = name;

	while (*p) {
		if ((p[0] == '.' && p[1] == '.') &&
		    (p[2] == '/' ||
		    p[2] == '\0')) {
			return (TRUE);
		}
		do {
			if (*p++ == '\0')
				return (FALSE);
		} while (*p != '/');
		p++;
		while (*p == '/')	/* Skip multiple slashes */
			p++;
	}
	return (FALSE);
}

/*
 * Check whether the name argument is inside the change set home tree.
 */
EXPORT BOOL
in_tree(name)
	const char	*name;
{
	if (name[0] == '/') {
		return (FALSE);
	}
	if (has_dotdot(name)) {
		return (FALSE);
	}
	return (TRUE);
}

/*
 * Try to make the path name a relative path name, relative to the
 * change set home directory.
 *
 * If this cannot be done, leave name as it is.
 */
EXPORT void
make_relative(name)
	char	*name;
{
	if (name[0] == '/' && setahome) {
		if (strncmp(name, setahome, setahomelen) == 0 &&
		    name[setahomelen] == '/') {
			ovstrcpy(name, &name[setahomelen+1]);
		}
	}
}
