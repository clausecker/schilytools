/* @(#)delcmds.c	1.27 09/07/09 Copyright 1984-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)delcmds.c	1.27 09/07/09 Copyright 1984-2009 J. Schilling";
#endif
/*
 *	Commands that deal with deletions
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
#include "movedot.h"

EXPORT	void	delchars	__PR((ewin_t *wp, epos_t numchars));
EXPORT	void	rubchars	__PR((ewin_t *wp, epos_t numchars));
EXPORT	void	vdel		__PR((ewin_t *wp));
EXPORT	void	vrub		__PR((ewin_t *wp));
EXPORT	void	vwdel		__PR((ewin_t *wp));
EXPORT	void	vwrub		__PR((ewin_t *wp));
EXPORT	void	vkill		__PR((ewin_t *wp));
EXPORT	void	vpkill		__PR((ewin_t *wp));
EXPORT	void	vskill		__PR((ewin_t *wp));
EXPORT	void	vundel		__PR((ewin_t *wp));

/*
 * Delete characters forwards starting at the current cursor position.
 * First save them in the delete buffer if needed.
 */
EXPORT void
delchars(wp, nchars)
	ewin_t	*wp;
	epos_t nchars;
{
	if (nchars <= 0)
		return;

	if (nchars > wp->eof-wp->dot)
		nchars = wp->eof-wp->dot;

	if ((wp->eflags & SAVEDEL) != 0) {
		if ((wp->eflags & KEEPDEL) == 0)
			rubsize = delsize = 0;
		lseek(fdown(delfile), (off_t)delsize, SEEK_SET);
		savefile(wp, wp->dot, wp->dot+nchars, delfile, "DEL BUF");
		delsize += nchars;
		wp->eflags |= DELDONE;
	}
	dispup(wp, wp->dot, wp->dot+nchars);

	delete(wp, nchars);
	modified(wp);
}

/*
 * Delete characters backwards starting at the current cursor position.
 * First save them in the rub-out buffer if needed. We have to save them
 * backwards to be able to restore backward deletions correctly.
 */
EXPORT void
rubchars(wp, nchars)
	ewin_t	*wp;
	epos_t nchars;
{
	epos_t	savedot;
	epos_t	newdot;

	if (nchars <= 0)
		return;

	if (nchars > wp->dot)
		nchars = wp->dot;

	if ((wp->eflags & SAVEDEL) != 0) {
		if ((wp->eflags & KEEPDEL) == 0)
			rubsize = delsize = 0;
		lseek(fdown(rubfile), (off_t)rubsize, SEEK_SET);
		backsavefile(wp, wp->dot-nchars, wp->dot, rubfile, "DEL BUF");
		rubsize += nchars;
		wp->eflags |= DELDONE;
	}

	savedot = wp->dot;
	newdot = wp->dot -= nchars;
	if (newdot < wp->window)
		wp->dot = wp->window;
	setpos(wp);			/* update part on screen */
	setcursor(wp);
	dispup(wp, wp->dot, savedot);
	if (newdot < wp->window)
		findwpos(wp, wp->window = newdot);
	wp->dot = savedot;

	rubout(wp, nchars);
	modified(wp);
}

/*
 * Delete 'curnum' characters atfer the cursor
 */
EXPORT void
vdel(wp)
	ewin_t	*wp;
{
	if (wp->dot == wp->eof) {
		ringbell();
	} else {
		if (wp->dot+wp->curnum > wp->eof)	/* Make sure we don't go bejond eof. */
			wp->curnum = wp->eof-wp->dot;
		delchars(wp, (epos_t)wp->curnum);
	}
}

/*
 * Delete 'curnum' characters before the cursor
 */
EXPORT void
vrub(wp)
	ewin_t	*wp;
{
	if (wp->dot == 0) {
		ringbell();
	} else {
		if (wp->curnum > wp->dot)
			wp->curnum = wp->dot;
		rubchars(wp, (epos_t)wp->curnum);
	}
}

/*
 * Delete 'curnum' words after the cursor
 */
EXPORT void
vwdel(wp)
	ewin_t	*wp;
{
	wp->curnum = forwword(wp, wp->dot, wp->curnum) - wp->dot;
	vdel(wp);
}

/*
 * Delete 'curnum' words before the cursor
 */
EXPORT void
vwrub(wp)
	ewin_t	*wp;
{
	epos_t	pos;

	pos = revword(wp, wp->dot, wp->curnum);
	wp->curnum = wp->dot - pos;
	vrub(wp);
}

/*
 * Delete from cursor to the end of the 'curnum-1' th line
 */
EXPORT void
vkill(wp)
	ewin_t	*wp;
{
	wp->curnum = forwline(wp, wp->dot, wp->curnum) - wp->dot;
	vdel(wp);
}

/*
 * Delete from cursor to the end of the 'curnum-1' th paragraph
 */
EXPORT void
vpkill(wp)
	ewin_t	*wp;
{
	wp->curnum = forwpara(wp, wp->dot, wp->curnum) - wp->dot;
	vdel(wp);
}

/*
 * Delete characters in selection (between cursor and mark)
 */
EXPORT void
vskill(wp)
	ewin_t	*wp;
{
	if (wp->markvalid) {
		if (wp->mark > wp->dot) {
			delchars(wp, wp->mark-wp->dot);
		} else {
			rubchars(wp, wp->dot-wp->mark);
		}
	} else {
		nomarkmsg(wp);
	}
}

/*
 * Undo previous deletions
 *
 * First insert the content of the rub-out (back delete) buffer,
 * then insert the content of the forward delete buffer.
 * After doing that, the cursor is between both insertions.
 */
EXPORT void
vundel(wp)
	ewin_t	*wp;
{
	wp->curnum = 1;
	backgetfile(wp, rubfile, rubsize, "RUB BUF");
	update(wp);
	getfile(wp, delfile, delsize, "DEL BUF");
	rubsize = delsize = 0;
}
