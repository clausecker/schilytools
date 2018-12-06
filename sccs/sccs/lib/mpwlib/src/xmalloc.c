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
 * @(#)xmalloc.c	1.1 18/12/04 Copyright 2018 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)xmalloc.c	1.1 18/12/04 Copyright 2018 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)xmalloc.c"
#pragma ident	"@(#)sccs:lib/mpwlib/xmalloc.c"
#endif
#include	<defines.h>

EXPORT void *
xmalloc(size)
	size_t	size;
{
	void	*ret;

	if ((ret = malloc(size)) == NULL) {
		fatal(gettext("OUT OF SPACE (ut9)"));
	}
	return (ret);
}
