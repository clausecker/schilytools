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
 * @(#)putmeta.c	1.6 18/12/04 Copyright 2011-2018 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)putmeta.c	1.6 18/12/04 Copyright 2011-2018 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)putmeta.c"
#pragma ident	"@(#)sccs:lib/comobj/putmeta.c"
#endif
#include	<defines.h>

void
putmeta(pkt, flags)
	register struct packet	*pkt;
		unsigned int	flags;
{
	char	line[max(8192, PATH_MAX+1)];
	char	urbuf[20];

	if (flags & M_URAND)
#ifdef	HAVE_LONG_LONG
	if (pkt->p_rand != 0) {
#else
	if (pkt->p_rand.high != 0 || pkt->p_rand.low != 0) {
#endif
		urand_ba(&pkt->p_rand, urbuf, sizeof (urbuf));
		snprintf(line, sizeof (line), "%c%c %s %s\n",
			CTLCHAR, GLOBALEXTENS, "r", urbuf);
		putline(pkt, line);
	}

	/*
	 * We place this after urand because init_path is most likely to be
	 * added later and thus will appear past urand in such a case anyway.
	 * This way, we can better avoid disordered meta data.
	 */
	if (pkt->p_init_path && (flags & M_INIT_PATH)) {
		snprintf(line, sizeof (line), "%c%c %s %s\n",
			CTLCHAR, GLOBALEXTENS, "p",
			pkt->p_init_path);
		putline(pkt, line);
	}
}
