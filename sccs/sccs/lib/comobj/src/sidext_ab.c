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
 * @(#)sidext_ab.c	1.1 11/09/02 Copyright 2011 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)sidext_ab.c	1.1 11/09/02 Copyright 2011 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)sidext_ab.c"
#pragma ident	"@(#)sccs:lib/comobj/sidext_ab.c"
#endif
#include	<defines.h>

void
sidext_ab(pkt, dt, asx)
	register struct packet	*pkt;
		struct deltab	*dt;
	register char		*asx;
{
	int	n;

	NONBLANK(asx);
	if (asx[0] == 's' && asx[1] == ' ') {
		if (pkt->p_hash == 0) {
			pkt->p_hash = (uint16_t *)
				fmalloc((unsigned) (n=((dt->d_serial+1)*
				sizeof(uint16_t))));
			zero((char *) pkt->p_hash, n);
		}
		asx++;
		NONBLANK(asx);
		asx = satoi(asx, &n);
		if (*asx != '\n')
			fmterr(pkt);
		pkt->p_hash[dt->d_serial] = n;	
	}
}

void
sidext_v4compat_ab(pkt, dt)
	register struct packet	*pkt;
		struct deltab	*dt;
{
	if (pkt->p_line[3] == BDELTAB) {
		if (pkt->p_idel->i_pred == dt->d_serial)
			pkt->p_flags |= PF_V6TAGS;
		/*
		 * Ignore v6 timezone extensions for now when in compat mode.
		 */
	} else if (pkt->p_line[3] == SIDEXTENS) {
		if (pkt->p_flags & PF_V6TAGS)
			sidext_ab(pkt, dt, &pkt->p_line[4]);
	}
}
