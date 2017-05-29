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
 * @(#)usum.c	1.3 17/05/24 Copyright 2011-2017 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)usum.c	1.3 17/05/24 Copyright 2011-2017 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)usum.c"
#pragma ident	"@(#)sccs:lib/comobj/usum.c"
#endif
#include	<defines.h>


#define	DO8(a)	a; a; a; a; a; a; a; a;

unsigned int
usum(p, len)
	register char	*p;
	register int	len;
{
	register unsigned int	sum = 0;
	register unsigned char	*cp = (unsigned char *)p;

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
