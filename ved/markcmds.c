/* @(#)markcmds.c	1.17 00/12/14 Copyright 1984 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)markcmds.c	1.17 00/12/14 Copyright 1984 J. Schilling";
#endif
/*
 *	Commands that deal with the mark defining the selection
 *	which is bewteen cursor and mark.
 *
 *	Copyright (c) 1984 J. Schilling
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

EXPORT	void	setmark		__PR((ewin_t *wp, epos_t newmark));
EXPORT	void	resetmark	__PR((ewin_t *wp));
LOCAL	void	displaymark	__PR((ewin_t *wp));
EXPORT	void	vjumpmark	__PR((ewin_t *wp));
EXPORT	void	vexchmarkdot	__PR((ewin_t *wp));
EXPORT	void	vclearmark	__PR((ewin_t *wp));
EXPORT	void	vsetmark	__PR((ewin_t *wp));

/*
 * Set the mark to a position and display it
 */
EXPORT void
setmark(wp, newmark)
	ewin_t	*wp;
	epos_t	newmark;
{
	if (wp->markvalid)
		resetmark(wp);
	if (newmark > wp->eof)
		newmark = wp->eof;
	wp->mark = newmark;
	wp->markvalid = 1;
	displaymark(wp);
}

/*
 * Clear (unvalidate) the mark and display the change
 */
EXPORT void
resetmark(wp)
	ewin_t	*wp;
{
	wp->markvalid = 0;
	displaymark(wp);
}

/*
 * Go to the mark and make it visible or not - depending on the current
 * status of the mark.
 */
LOCAL void
displaymark(wp)
	ewin_t	*wp;
{
	epos_t	savedot = wp->dot;

	if (wp->mark > wp->eof)
		wp->mark = wp->eof;
	if (wp->mark < wp->window)
		return;
	update(wp);		/* Needed if the command moved 'dot' */
	wp->dot = wp->mark;

	setpos(wp);
	if (setcursor(wp))
		typescreen(wp, wp->dot, cursor.hp, wp->dot + 1);
	wp->dot = savedot;
	setpos(wp);
	setcursor(wp);
}

/*
 * Set the cursor to the position of the mark
 */
EXPORT void
vjumpmark(wp)
	ewin_t	*wp;
{
	if (! wp->markvalid) {
		nomarkmsg(wp);
	} else {
		wp->dot = wp->mark;
	}
}

/*
 * Exchange the positions of the cursor and the mark
 */
EXPORT void
vexchmarkdot(wp)
	ewin_t	*wp;
{
	epos_t	savedot = wp->dot;

	if (! wp->markvalid) {
		nomarkmsg(wp);
	} else {
		wp->dot = wp->mark;
	}
	setmark(wp, savedot);
}

/*
 * Clear (unvalidate) the mark
 */
EXPORT void
vclearmark(wp)
	ewin_t	*wp;
{
	if (wp->markvalid)
		resetmark(wp);
	writemsg(wp, "Mark cleared.");
}

/*
 * Set the mark to the position of the cursor
 */
EXPORT void
vsetmark(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
}
