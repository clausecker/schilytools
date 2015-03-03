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
 * @(#)change_ba.c	1.2 15/03/02 Copyright 2015 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)change_ba.c 1.2 15/03/02 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)change_ba.c"
#pragma ident	"@(#)sccs:lib/comobj/change_ba.c"
#endif
# define	NEED_PRINTF_Z		/* Need defines for js_snprintf()? */
# include	<defines.h>

EXPORT void
change_ba(pkt, cbuf, cbuflen)
	register struct packet	*pkt;
		char	*cbuf;
		size_t	cbuflen;
{
	char	str[SID_STRSIZE];
	char	urbuf[20];

	if (!pkt->p_init_path)
		fatal(gettext("no initial path in data structures (co35)"));

	urand_ba(&pkt->p_rand, urbuf, sizeof (urbuf));
	sid_ba(&pkt->p_reqsid, str);
	snprintf(cbuf, cbuflen, "%s|%s|%5.5d|%zd|%s",
			urbuf, str, pkt->p_ghash,
			strlen(pkt->p_init_path), pkt->p_init_path);
}
