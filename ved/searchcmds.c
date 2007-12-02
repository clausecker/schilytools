#ifndef lint
static	char sccsid[] =
	"@(#)searchcmds.c	1.21 04/03/12 Copyright 1984-2004 J. Schilling";
#endif
/*
 *	Commands that deal with searching patterns.
 *
 *	Copyright (c) 1984-2004 J. Schilling
 */
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

#include "ved.h"

/*
 * XXX EXPORT ???
 */
EXPORT	Uchar	sbuf[NAMESIZE];		/* buffer containing last search str */
EXPORT	int	sflg = 0;		/* size of search string	    */
					/* if >0 : search forward,	    */
					/* if <0 : search backwards	    */

EXPORT	void	vsearch		__PR((ewin_t *wp));
EXPORT	void	vssearch	__PR((ewin_t *wp));
LOCAL	void	xsearch		__PR((ewin_t *wp, BOOL domark));
EXPORT	void	vrsearch	__PR((ewin_t *wp));
EXPORT	void	vsrsearch	__PR((ewin_t *wp));
LOCAL	void	xrsearch	__PR((ewin_t *wp, BOOL domark));
EXPORT	void	vagainsrch	__PR((ewin_t *wp));
EXPORT	void	vsagainsrch	__PR((ewin_t *wp));
LOCAL	void	againsearch	__PR((ewin_t *wp, BOOL domark, BOOL rev));
EXPORT	void	vrevsrch	__PR((ewin_t *wp));
EXPORT	void	vsrevsrch	__PR((ewin_t *wp));
LOCAL	void	xreverse	__PR((ewin_t *wp, BOOL domark));
EXPORT	void	not_found	__PR((ewin_t *wp));

/*
 * Search forwards
 */
EXPORT void
vsearch(wp)
	ewin_t	*wp;
{
	xsearch(wp, FALSE);
}

/*
 * Search forwards and set mark at start of found pattern
 */
EXPORT void
vssearch(wp)
	ewin_t	*wp;
{
	xsearch(wp, TRUE);
}

/*
 * Search forwards for the 'curnum' th orrurence of a pattern,
 * set mark at start of found pattern if required.
 */
LOCAL void
xsearch(wp, domark)
	ewin_t	*wp;
	BOOL	domark;
{
	/* forward search */

	register int	i;
	register ecnt_t	n = wp->curnum;
		epos_t	newdot;
		Uchar	buf[NAMESIZE];

	i = getcmdline(wp, buf, sizeof (buf), "+Search: ");
	if (i == 0)
		return;
	movebytes(C buf, C sbuf, i + 1);
	sflg = i;
	while (--n >= 0) {
		if ((newdot = search(wp, wp->dot, sbuf, i, domark)) > wp->eof) {
			if (newdot == (wp->eof+2))
				not_found(wp);
			break;
		} else {
			wp->dot = newdot;
		}
	}
}

/*
 * Search backwards
 */
EXPORT void
vrsearch(wp)
	ewin_t	*wp;
{
	xrsearch(wp, FALSE);
}

/*
 * Search backwards and set mark at start of found pattern
 */
EXPORT void
vsrsearch(wp)
	ewin_t	*wp;
{
	xrsearch(wp, TRUE);
}

/*
 * Search backwards for the 'curnum' th orrurence of a pattern,
 * set mark at start of found pattern if required.
 */
LOCAL void
xrsearch(wp, domark)
	ewin_t	*wp;
	BOOL	domark;
{
	register int	i;
	register ecnt_t	n = wp->curnum;
		epos_t newdot;
		Uchar	buf[NAMESIZE];

	i = getcmdline(wp, buf, sizeof (buf), "-Search: ");
	if (i == 0)
		return;
	movebytes(C buf, C sbuf, i + 1);
	sflg = -i;
	while (--n >= 0) {
		if ((newdot = reverse(wp, wp->dot, sbuf, i, domark)) > wp->eof) {
			if (newdot == (wp->eof+2))
				not_found(wp);
			break;
		} else {
			wp->dot = newdot;
		}
	}
}

/*
 * Repeat the last search in same search direction
 */
EXPORT void
vagainsrch(wp)
	ewin_t	*wp;
{
	againsearch(wp, FALSE, FALSE);
}

/*
 * Repeat the last search in same search direction,
 * set mark at start of found pattern if required.
 */
EXPORT void
vsagainsrch(wp)
	ewin_t	*wp;
{
	againsearch(wp, TRUE, FALSE);
}

/*
 * Repeat the last search, reverse search direction if required,
 * set mark at start of found pattern if required.
 */
LOCAL void
againsearch(wp, domark, rev)
	ewin_t	*wp;
	BOOL	domark;
	BOOL	rev;
{
	register ecnt_t	n = wp->curnum;
		epos_t	newdot;

	if (rev)
		sflg = -sflg;
	writemsg(wp, "%sSearch: %s ...", (sflg >= 0 ? "+" : "-"), sbuf);
	while (--n >= 0) {
		if (sflg >= 0) {
			newdot = search(wp, wp->dot, sbuf, sflg, domark);
		} else {
			newdot = reverse(wp, wp->dot, sbuf, -sflg, domark);
		}
		if (newdot > wp->eof) {
			if (newdot == (wp->eof+2))
				not_found(wp);
			break;
		} else {
			wp->dot = newdot;
		}
	}
}

/*
 * Repeat the last search, reverse search direction
 */
EXPORT void
vrevsrch(wp)
	ewin_t	*wp;
{
	xreverse(wp, FALSE);
}

/*
 * Repeat the last search, reverse search direction
 * and set mark at start of found pattern
 */
EXPORT void
vsrevsrch(wp)
	ewin_t	*wp;
{
	xreverse(wp, TRUE);
}

/*
 * Repeat the last search, reverse search direction,
 * set mark at start of found pattern if required.
 */
LOCAL void
xreverse(wp, domark)
	ewin_t	*wp;
	BOOL	domark;
{
	againsearch(wp, domark, TRUE);
}

/*
 * write not found message
 */
EXPORT void
not_found(wp)
	ewin_t	*wp;
{
	writeerr(wp, "NOT FOUND");
}
