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
 * @(#)urand_ba.c	1.2 18/11/29 Copyright 2011-2018 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)urand_ba.c	1.2 18/11/29 Copyright 2011-2018 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)urand_ba.c"
#pragma ident	"@(#)sccs:lib/comobj/urand_ba.c"
#endif
#include	<defines.h>

char *
urand_ba(urandp, aurand, aurandsize)
	register urand_t	*urandp;
		char		*aurand;
		size_t		aurandsize;
{
	aurand[0] = '\0';
	/*
	 * With 13 hexadecimal digits, we are able to keep the same urand
	 * string length up to Mar 31 10:55:07 2155 GMT. If we did not
	 * enforce 13 digits there would be a length change at
	 * Mon Jun 14 06:30:56 2021 GMT.
	 */
#ifdef	HAVE_LONG_LONG
	if (*urandp != 0) {
		snprintf(aurand, aurandsize, "%13.13llx",
			*urandp);
	}
#else
	if (urandp->high != 0 || urandp->low != 0) {
		snprintf(aurand, aurandsize, "%5.5x%8.8x",
			urandp->high, urandp->low);
	}
#endif
	return (aurand);
}

int
urand_valid(urandp)
	register urand_t	*urandp;
{
	int	valid = 0;

#ifdef	HAVE_LONG_LONG
	if (*urandp != 0)
		valid++;
#else
	if (urandp->high != 0 || urandp->low != 0)
		valid++;
#endif
	return (valid);
}
