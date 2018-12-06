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
 * Copyright 2018 J. Schilling
 *
 * @(#)sfree.c	1.1 18/11/25 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)sfree.c 1.1 18/11/25 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)sfree.c"
#pragma ident	"@(#)sccs:lib/comobj/sfree.c"
#endif
# include	<defines.h>

	void	sfree		__PR((struct packet *pkt));

void
sfree(pkt)
register struct packet *pkt;
{
	if (pkt->p_linebase)
		free(pkt->p_linebase);
	pkt->p_linebase = pkt->p_line = NULL;
}
