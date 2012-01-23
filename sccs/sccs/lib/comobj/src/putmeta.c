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
 * @(#)putmeta.c	1.2 11/11/28 Copyright 2011 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)putmeta.c	1.2 11/11/28 Copyright 2011 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)putmeta.c"
#pragma ident	"@(#)sccs:lib/comobj/putmeta.c"
#endif
#include	<defines.h>

void
putmeta(pkt)
	register struct packet	*pkt;
{
	char	line[max(8192, PATH_MAX+1)];

	if (pkt->p_init_path) {
		snprintf(line, sizeof (line), "%c%c %s %s\n",
			CTLCHAR, GLOBALEXTENS, "p",
			pkt->p_init_path);
		putline(pkt, line);
	}
	/*
	 * With 13 hexadecimal digits, we are able to keep the same urand
	 * string length up to 2146 Sep 27 13:30:51 GMT. If we did not
	 * enforce 13 digits there would be a length change at
	 * 2012 Dec 11 09:06:41 GMT.
	 */
#ifdef	HAVE_LONG_LONG
	if (pkt->p_rand != 0) {
		snprintf(line, sizeof (line), "%c%c %s %13.13llx\n",
			CTLCHAR, GLOBALEXTENS, "r",
			pkt->p_rand);
		putline(pkt, line);
	}
#else
	if (pkt->p_rand.high != 0 || pkt->p_rand.low != 0) {
		snprintf(line, sizeof (line), "%c%c %s %5.5x%8.8x\n",
			CTLCHAR, GLOBALEXTENS, "r",
			pkt->p_rand.high, pkt->p_rand.low);
		putline(pkt, line);
	}
#endif
}
