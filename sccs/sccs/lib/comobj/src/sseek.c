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
 * @(#)sseek.c	1.1 11/09/11 Copyright 2011 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)sseek.c	1.1 11/09/11 Copyright 2011 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)sseek.c"
#pragma ident	"@(#)sccs:lib/comobj/sseek.c"
#endif
#include	<defines.h>


int
sseek(pkt, off, whence)
	register struct packet	*pkt;
		off_t		off;
		int		whence;
{
#ifndef	HAVE_MMAP
	return (fseek(pkt->p_iop, off, whence));
#else
	if (pkt->p_mmbase == NULL)
		return (fseek(pkt->p_iop, off, whence));

	if (pkt->p_savec) {
		*pkt->p_mmnext = pkt->p_savec;
		pkt->p_savec = '\0';
	}
	switch (whence) {

	case SEEK_SET:
			pkt->p_mmnext = pkt->p_mmbase + off;
			break;
	case SEEK_END:
			pkt->p_mmnext = (pkt->p_mmbase + pkt->p_mmsize) + off;
			break;
	case SEEK_CUR:
			pkt->p_mmnext = pkt->p_mmnext + off;
			break;

	default:	seterrno(EINVAL);
			return (-1);
	}

	if (pkt->p_mmnext > (pkt->p_mmbase + pkt->p_mmsize)) {
		pkt->p_mmnext = (pkt->p_mmbase + pkt->p_mmsize);
		seterrno(EINVAL);
		return (-1);
	}
	if (pkt->p_mmnext < pkt->p_mmbase) {
		pkt->p_mmnext = pkt->p_mmbase;
		seterrno(EINVAL);
		return (-1);
	}

	return (0);
#endif
}

int
srewind(pkt)
	register struct packet	*pkt;
{
	return (sseek(pkt, (off_t)0, SEEK_SET));
}
