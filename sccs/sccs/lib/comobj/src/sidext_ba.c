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
 * @(#)sidext_ba.c	1.3 19/11/11 Copyright 2011-2019 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)sidext_ba.c	1.3 19/11/11 Copyright 2011-2019 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)sidext_ba.c"
#pragma ident	"@(#)sccs:lib/comobj/sidext_ba.c"
#endif
#include	<defines.h>

void
sidext_ba(pkt, dt)
	register struct packet	*pkt;
		struct deltab	*dt;
{
	char	line[MAXLINE];

	if ((pkt->p_flags & (PF_V6 | PF_V6TAGS)) == 0)
		return;

	/*
	 * This entry is not really a SID extendion but an extended delta entry
	 * in SCCS v4 coding. We create the entry here as it must be the first
	 * degenerated comment and as it must be directly followed by "^AS s".
	 */
	if ((pkt->p_flags & PF_V6) == 0) {
		del_ba(dt, &line[2], PF_V6);
		line[0] = CTLCHAR;
		line[1] = COMMENTS;
		line[2] = '_';
		putline(pkt, line);
	}
	if (pkt->p_flags & PF_V6)
		snprintf(line, sizeof (line), "%c%c s %5.5d\n",
			CTLCHAR, SIDEXTENS, pkt->p_ghash);
	else
		snprintf(line, sizeof (line), "%c%c_%c s %5.5d\n",
			CTLCHAR, COMMENTS, SIDEXTENS, pkt->p_ghash);
	putline(pkt, line);
	if (pkt->p_mail) {
		if (pkt->p_flags & PF_V6)
			snprintf(line, sizeof (line), "%c%c m %s\n",
				CTLCHAR, SIDEXTENS, pkt->p_mail);
		else
			snprintf(line, sizeof (line), "%c%c_%c m %s\n",
				CTLCHAR, COMMENTS, SIDEXTENS, pkt->p_mail);
		putline(pkt, line);
	}
}
