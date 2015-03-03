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
 *	Mkdirs (mkdir -p) + check for errno != EEXIST
 *
 *	path must be modifyable as we insert null bytes when traversing the tree.
 *
 * @(#)xmkdirs.c	1.1 15/02/28 Copyright 2015 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)xmkdirs.c	1.1 15/02/28 Copyright 2015 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)xmkdirs.c"
#pragma ident	"@(#)sccs:lib/mpwlib/xmkdirs.c"
#endif
#include	<defines.h>

EXPORT int
#ifdef  PROTOTYPES
xmkdirs(char *path, mode_t mode)
#else
xmkdirs(path, mode)
	char	*path;
	mode_t	mode;
#endif
{
	int	ret;

	if ((ret = mkdirs(path, mode)) < 0) {
		if (errno != EEXIST) {
			Ffile = path;
			efatal(gettext("cannot make directories (ut15)"));
		}
	}
	return (ret);
}
