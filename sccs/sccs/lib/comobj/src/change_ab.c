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
 * @(#)change_ab.c	1.1 15/02/28 Copyright 2015 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)change_ab.c 1.1 15/02/28 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)change_ab.c"
#pragma ident	"@(#)sccs:lib/comobj/change_ab.c"
#endif
# include	<defines.h>

static	void	change_syntax __PR((void));

EXPORT char *
change_ab(cbuf, pkt)
		char	*cbuf;
	register struct packet	*pkt;
{
	int		i;
	register char	*p;

	p = cbuf;
	p = urand_ab(p, &pkt->p_rand);
	if (*p++ != '|')
		change_syntax();

	p = sid_ab(p, &pkt->p_reqsid);
	if (*p++ != '|')
		change_syntax();

	p = satoi(p, &pkt->p_ghash);
	if (*p == ',')
		while(*p && *p != '|')
			;
	if (*p++ != '|')
		change_syntax();

	p = satoi(p, &i);
	if (*p++ != '|')
		change_syntax();

	pkt->p_init_path = fmalloc(i + 1);
	strlcpy(pkt->p_init_path, p, i);

	return (p+i);
}

static void
change_syntax()
{
	fatal(gettext("bad syntax in changeset line (co34)"));
}

