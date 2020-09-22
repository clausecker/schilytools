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
 * @(#)namedflags.c	1.3 20/09/16 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)namedflags.c 1.3 20/09/16 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)namedflags.c"
#pragma ident	"@(#)sccs:lib/comobj/namedflags.c"
#endif
# include	<defines.h>

	void	donamedflags	__PR((struct packet *pkt));

/*
 * WARNING: This function must be called after doflags() or you need to make
 * otherwise sure that pkt->p_line points to the first line after the
 * optional SCCS v4 flags section in the history file.
 */
void
donamedflags(pkt)
register struct packet *pkt;
{
	register char *p;

	/*
	 * If we start to support named flags, we need to clear/free old
	 * flags here.
	 */

	/*
	 * We are called after doflags() returned and for this reason, the
	 * first line past SCCS v4 flags has already been read. As SCCS v6
	 * flags are optional, we need to exit in case that we are not on
	 * a line with named flags.
	 */
	p = pkt->p_line;
	if (p == NULL || *p++ != CTLCHAR || *p++ != NAMEDFLAG)
		return;

	do {
		NONBLANK(p);

		/*
		 * Skip dummy flag entries (only spaces).
		 */
		if (*p == '\n') {
			pkt->p_wrttn++;			/* only if pkt->p_upd */
			continue;
		}
		pkt->p_line[pkt->p_line_length-1] = 0;	/* Kill newline */

		if (Ffile) {
			fprintf(stderr,
				gettext(
				"WARNING [%s]: unsupported named flag '%s' at line %d\n"),
				Ffile, p, pkt->p_slnno);
		} else {
			fprintf(stderr,
				gettext(
				"WARNING: unsupported named flag '%s' at line %d\n"),
				p, pkt->p_slnno);
		}
		pkt->p_line[pkt->p_line_length-1] = '\n'; /* Restore newline */
	} while ((p = getline(pkt)) != NULL &&
				*p++ == CTLCHAR && *p++ == NAMEDFLAG);
}
