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
 * @(#)sclose.c	1.1 18/11/13 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)sclose.c 1.1 18/11/13 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)sclose.c"
#pragma ident	"@(#)sccs:lib/comobj/sclose.c"
#endif
# include	<defines.h>

	void	sclose		__PR((struct packet *pkt));

void
sclose(pkt)
register struct packet *pkt;
{
	if (pkt->p_iop)
		(void) fclose(pkt->p_iop);
	pkt->p_iop = NULL;
}
