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
 * @(#)urand_ab.c	1.2 15/02/28 Copyright 2015 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)urand_ab.c	1.2 15/02/28 Copyright 2015 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)urand_ab.c"
#pragma ident	"@(#)sccs:lib/comobj/urand_ab.c"
#endif
#include	<defines.h>

char *
urand_ab(aurand, urandp)
		char		*aurand;
	register urand_t	*urandp;
{
	char	*p;

#ifdef	HAVE_LONG_LONG
	unsigned long long	ll = 0;
	int			c;

	for (p = aurand; (c = *p) != '\0'; p++) {
		if (c >= '0' && c <= '9')
			c = c - '0';
		else if (c >= 'a' && c <= 'f')
			c = c - 'a' + 10;
		else
			break;			/* scan until !hex char seen */
		ll *= 16;
		ll += c;
	}
	*urandp = ll;
	aurand = p;
#else
	int		ret;
	unsigned long	lhi;
	unsigned long	llo;

	ret = sscanf(aurand, "%5lx%8lx", &lhi, &llo); /* handles only 52 bits */
	if (ret == 2) {
		urandp->high = lhi;
		urandp->low = llo;
		p = aurand + 13;
		aurand = p;
	}
#endif
	return (aurand);
}
