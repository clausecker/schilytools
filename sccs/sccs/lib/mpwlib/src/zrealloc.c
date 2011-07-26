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
 *
 * @(#)zrealloc.c	1.1 11/06/26 Copyright 2011 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)zrealloc.c	1.1 11/06/26 Copyright 2011 J. Schilling";
#endif

#if defined(sun)
#pragma ident	"@(#)zrealloc.c"
#pragma ident	"@(#)sccs:lib/mpwlib/zrealloc.c"
#endif
# include	<defines.h>

#undef	realloc

void *
zrealloc(ptr, amt)
	void	*ptr;
	size_t	amt;
{
	if (ptr)
		return (realloc(ptr, amt));
	return (malloc(amt));
}
