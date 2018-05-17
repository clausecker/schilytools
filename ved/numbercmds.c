/* @(#)numbercmds.c	1.19 09/07/09 Copyright 1984-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)numbercmds.c	1.19 09/07/09 Copyright 1984-2009 J. Schilling";
#endif
/*
 *	Routines that deal with number and mult.
 *
 *	Copyright (c) 1984-2009 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include "ved.h"

LOCAL	ecnt_t	mult = (ecnt_t)4;
LOCAL	ecnt_t	maxnum = (ecnt_t)0;

EXPORT	void	initnum		__PR((void));
EXPORT	void	vmult		__PR((ewin_t *wp));
EXPORT	void	vsmult		__PR((ewin_t *wp));
EXPORT	void	vnum		__PR((ewin_t *wp));
LOCAL	void	enumbad		__PR((ewin_t *wp));
LOCAL	void	enumoverflow	__PR((ewin_t *wp));
LOCAL	void	enumnegative	__PR((ewin_t *wp));

/*
 *  Multiply number with mult.
 */
EXPORT void
initnum()
{
	ecnt_t	n;
	ecnt_t	on;
	ecnt_t	a;
	int	i;

	if (maxnum == 0) {
		/*
		 * Overflow bei Berechnung von maxnum verhindern
		 */
		for (i = 0, on = 0, n = (ecnt_t)1; n > 0 && n > on && i < 256; i++) {
			on = n;
			n  = 2*n;
		}
		for (i = 0, a = n = on; n > (ecnt_t)0 && n >= on && i < 256; i++) {
			on = n;
			a /= 2;
			if (a < 1)
				break;
			n  += a;
		}
		maxnum = on;
	}
}

/*
 *  Multiply number with mult.
 */
EXPORT void
vmult(wp)
	ewin_t	*wp;
{
	if ((wp->eflags & KEEPDEL) != 0)
		wp->eflags |= DELDONE;
	wp->eflags &= ~COLUPDATE;

	if (wp->number >= maxnum/mult) {
		enumoverflow(wp);
		return;
	}
	wp->eflags |= SAVENUM;
	wp->number *= mult;
	writenum(wp, wp->number);
}

/*
 * Set mult.
 */
EXPORT void
vsmult(wp)
	ewin_t	*wp;
{
	Llong l;
	ecnt_t	n;
	Uchar	numbuf[NAMESIZE];

	if (! getcmdline(wp, numbuf, sizeof (numbuf), "Mult = "))
		return;
	if (*astoll(C numbuf, &l)) {
		enumbad(wp);
	} else if (l <= 0) {
		enumnegative(wp);
	} else {
		n = l;
		if (n != l) {
			enumoverflow(wp);
			return;
		}
		mult = l;
	}
	wp->eflags &= ~COLUPDATE;
}

/*
 * Set number from the commandline.
 * We already have the first character.
 * XXX If we bind this to something else than ESC[0-9], we have to rethink.
 */
EXPORT void
vnum(wp)
	ewin_t	*wp;
{
	Llong	l;
	ecnt_t	n;
	Uchar	numbuf[NAMESIZE];

	if (! getccmdline(wp, wp->lastch, numbuf, sizeof (numbuf), "# = "))
		return;
	if (*astoll(C numbuf, &l)) {
		enumbad(wp);
	} else if (l <= 0) {
		enumnegative(wp);
	} else {
		n = l;
		if (n != l) {
			enumoverflow(wp);
			return;
		}
		wp->number = l;
		writenum(wp, wp->number);
	}
	wp->eflags |= SAVENUM;
	if ((wp->eflags & KEEPDEL) != 0)
		wp->eflags |= DELDONE;
}

LOCAL void
enumbad(wp)
	ewin_t	*wp;
{
	writeerr(wp, "Bad Number");
}

LOCAL void
enumoverflow(wp)
	ewin_t	*wp;
{
	writeerr(wp, "Number Overflow!");
}

LOCAL void
enumnegative(wp)
	ewin_t	*wp;
{
	writeerr(wp, "Must be Positive");
}
