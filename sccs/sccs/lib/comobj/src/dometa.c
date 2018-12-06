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
 * @(#)dometa.c	1.2 18/11/25 Copyright 2011-2018 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)dometa.c	1.2 18/11/25 Copyright 2011-2018 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)dometa.c"
#pragma ident	"@(#)sccs:lib/comobj/dometa.c"
#endif
#include	<defines.h>

#define	BLANK(l)	while (!(*l == '\0' || *l == ' ' || *l == '\t' || *l == '\n')) l++;


void
dometa(pkt)
	register struct packet	*pkt;
{
	register char *p;

	/*
	 * We are called after donamedflags() returned and for this reason,
	 * the first line past SCCS v6 flags has already been read. As SCCS v6
	 * global meta data my be missing in old SCCS v6 history files, we need
	 * to exit in case that we are not on a line with global meta data.
	 */
	p = pkt->p_line;
	if (p == NULL || *p++ != CTLCHAR || *p++ != GLOBALEXTENS)
		return;

	do {
		char	*p2;

		NONBLANK(p);
		p2 = p;
		BLANK(p2);

		if ((p2 - p) == 1 && *p == 'r') {
			p++;
			NONBLANK(p);
			urand_ab(p, &pkt->p_rand);
			continue;
		}

		if ((p2 - p) == 1 && *p == 'p') {
			char	*ip;

			p++;
			NONBLANK(p);
			p2 = p;
			BLANK(p2);

			ip = fmalloc(p2 - p + 1);
			strlcpy(ip, p, p2 - p + 1);
			pkt->p_init_path = ip;
			continue;
		}

		pkt->p_line[pkt->p_line_length-1] = 0;	/* Kill newline */
		if (Ffile) {
			fprintf(stderr,
				gettext(
				"WARNING [%s]: unsupported global meta data '%s' at line %d\n"),
				Ffile, p, pkt->p_slnno);
		} else {
			fprintf(stderr,
				gettext(
				"WARNING: unsupported global meta data '%s' at line %d\n"),
				p, pkt->p_slnno);
		}
	} while ((p = getline(pkt)) != NULL &&
				*p++ == CTLCHAR && *p++ == GLOBALEXTENS);
}
