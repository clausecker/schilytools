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
 * @(#)stell.c	1.1 11/09/11 Copyright 2011 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)stell.c	1.1 11/09/11 Copyright 2011 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)stell.c"
#pragma ident	"@(#)sccs:lib/comobj/stell.c"
#endif
#include	<defines.h>


off_t
stell(pkt)
	register struct packet *pkt;
{
#ifndef	HAVE_MMAP
	return (ftell(pkt->p_iop));
#else
	if (pkt->p_mmbase == NULL)
		return (ftell(pkt->p_iop));

	return ((off_t)(pkt->p_mmnext - pkt->p_mmbase));
#endif
}
