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
 *	Mkdir + check for errno != EEXIST
 *
 * @(#)xmkdir.c	1.2 15/02/25 Copyright 2015 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)xmkdir.c	1.2 15/02/25 Copyright 2015 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)xmkdir.c"
#pragma ident	"@(#)sccs:lib/mpwlib/xmkdir.c"
#endif
#include	<defines.h>

EXPORT int
#ifdef  PROTOTYPES
xmkdir(char *path, mode_t mode)
#else
xmkdir(path, mode)
	char	*path;
	mode_t	mode;
#endif
{
	int	ret;

	if ((ret = mkdir(path, mode)) < 0) {
		if (errno != EEXIST) {
			Ffile = path;
			efatal(gettext("cannot make directory (ut14)"));
		}
	}
	return (ret);
}
