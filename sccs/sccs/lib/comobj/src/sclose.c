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
 * Copyright 2018-2020 J. Schilling
 *
 * @(#)sclose.c	1.2 20/07/27 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)sclose.c 1.2 20/07/27 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)sclose.c"
#pragma ident	"@(#)sccs:lib/comobj/sclose.c"
#endif
#include	<defines.h>

	void	sclose		__PR((struct packet *pkt));

void
sclose(pkt)
register struct packet *pkt;
{
	if (pkt->p_iop)
		(void) fclose(pkt->p_iop);
	pkt->p_iop = NULL;
#ifdef	HAVE_MMAP
	if (pkt->p_mmbase) {
		munmap(pkt->p_mmbase, pkt->p_mmsize);
		pkt->p_mmbase = NULL;
		pkt->p_mmnext = NULL;
		pkt->p_mmend = NULL;
		pkt->p_mmsize = 0;
	}
#endif
}
