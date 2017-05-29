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
 * @(#)ssum.c	1.2 17/05/24 Copyright 2011-2017 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)ssum.c	1.2 17/05/24 Copyright 2011-2017 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)ssum.c"
#pragma ident	"@(#)sccs:lib/comobj/ssum.c"
#endif
#include	<defines.h>


#define	DO8(a)	a; a; a; a; a; a; a; a;

int
ssum(p, len)
	register char	*p;
	register int	len;
{
	register 	int	sum = 0;
	register signed	char	*cp = (signed char *)p;

	while (len >= 8) {
		DO8(sum += *cp++);
		len -= 8;
	}
	if (len <= 0)
		return (sum);

	switch (len) {

	case 7:	sum += *cp++;
	case 6:	sum += *cp++;
	case 5:	sum += *cp++;
	case 4:	sum += *cp++;
	case 3:	sum += *cp++;
	case 2:	sum += *cp++;
	case 1:	sum += *cp++;
	}
	return (sum);
}
