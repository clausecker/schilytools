/* @(#)takecmds.c	1.30 08/12/22 Copyright 1984-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)takecmds.c	1.30 08/12/22 Copyright 1984-2008 J. Schilling";
#endif
/*
 *	Commands that deal with take buffers.
 *
 *	Copyright (c) 1984-2008 J. Schilling
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
#include "movedot.h"
extern	char TAKEBUF[];

EXPORT	void	vcleartake	__PR((ewin_t *wp));
LOCAL	void	wsave		__PR((ewin_t *wp, epos_t start, epos_t size));
EXPORT	void	vlsave		__PR((ewin_t *wp));
EXPORT	void	vpsave		__PR((ewin_t *wp));
EXPORT	void	vcsave		__PR((ewin_t *wp));
EXPORT	void	vwsave		__PR((ewin_t *wp));
EXPORT	void	vssave		__PR((ewin_t *wp));
EXPORT	void	vgetclr		__PR((ewin_t *wp));
EXPORT	void	vget		__PR((ewin_t *wp));
EXPORT	void	vsgetclr	__PR((ewin_t *wp));
EXPORT	void	vsget		__PR((ewin_t *wp));
EXPORT	void	vtname		__PR((ewin_t *wp));
EXPORT	void	vwrtake		__PR((ewin_t *wp));

/*
 * Clear the curent take buffer
 */
EXPORT void
vcleartake(wp)
	ewin_t	*wp;
{
	takesize = 0L;
	writemsg(wp, "Buffer cleared");
}

/*
 * Save 'size' characters from 'start' position into current take buffer
 */
LOCAL void
wsave(wp, start, size)
	ewin_t	*wp;
	epos_t	start;
	epos_t	size;
{
	lseek(fdown(takefile), (off_t)takesize, SEEK_SET);
	savefile(wp, start, start+size, takefile, TAKEBUF);
	takesize += size;
}

/*
 * Put from cursor to the end of the 'curnum-1' th line
 * into the current take buffer
 */
EXPORT void
vlsave(wp)
	ewin_t	*wp;
{
	epos_t	new;

	if (wp->dot == wp->eof) {
		ringbell();
	} else {
		new = forwline(wp, wp->dot, wp->curnum);
		wsave(wp, wp->dot, (epos_t)(new-wp->dot));
		wp->eflags &= ~SAVEDEL;
		vkill(wp);
	}
}

/*
 * Put from cursor to the end of the 'curnum-1' th paragraph
 * into the current take buffer
 */
EXPORT void
vpsave(wp)
	ewin_t	*wp;
{
	epos_t	new;

	if (wp->dot == wp->eof) {
		ringbell();
	} else {
		new = forwpara(wp, wp->dot, wp->curnum);
		wsave(wp, wp->dot, (epos_t)(new-wp->dot));
		wp->eflags &= ~SAVEDEL;
		vpkill(wp);
	}
}

/*
 * Put 'curnum' characters after the cursor into the current take buffer
 */
EXPORT void
vcsave(wp)
	ewin_t	*wp;
{
	if (wp->dot == wp->eof) {
		ringbell();
	} else {
		if (wp->curnum + wp->dot > wp->eof)
			wp->curnum = wp->eof - wp->dot;
		wsave(wp, wp->dot, (epos_t)wp->curnum);
		wp->eflags &= ~SAVEDEL;
		vdel(wp);
	}
}

/*
 * Put 'curnum' words after the cursor into the current take buffer
 */
EXPORT void
vwsave(wp)
	ewin_t	*wp;
{
	epos_t	new;

	if (wp->dot == wp->eof) {
		ringbell();
	} else {
		new = forwword(wp, wp->dot, wp->curnum);
		wsave(wp, wp->dot, (epos_t)(new-wp->dot));
		wp->eflags &= ~SAVEDEL;
		vwdel(wp);
	}
}

/*
 * Put characters in selection (between cursor and mark)
 * into the current take buffer
 */
EXPORT void
vssave(wp)
	ewin_t	*wp;
{
	if (wp->dot > wp->mark)
		wsave(wp, wp->mark, (epos_t)(wp->dot-wp->mark));
	else
		wsave(wp, wp->dot, (epos_t)(wp->mark-wp->dot));
	wp->eflags &= ~SAVEDEL;
	vskill(wp);
}

/*
 * Insert the content of the current take buffer at cursor position,
 * then clear the take buffer
 */
EXPORT void
vgetclr(wp)
	ewin_t	*wp;
{
	vget(wp);
	takesize = 0L;
}

/*
 * Insert the content of the current take buffer at cursor position
 */
EXPORT void
vget(wp)
	ewin_t	*wp;
{
	getfile(wp, takefile, takesize, TAKEBUF);
}

/*
 * Delete all characters between cursor and mark,
 * then insert the content of the current take buffer at cursor position,
 * then clear the take buffer
 */
EXPORT void
vsgetclr(wp)
	ewin_t	*wp;
{
	if (!wp->markvalid) {
		nomarkmsg(wp);
	} else {
		vsget(wp);
		takesize = 0L;
	}
}

/*
 * Delete all characters between cursor and mark,
 * then insert the content of the current take buffer at cursor position
 */
EXPORT void
vsget(wp)
	ewin_t	*wp;
{
	BOOL	markfirst;
	epos_t	savesize = takesize;

	if (!wp->markvalid) {
		nomarkmsg(wp);
		return;
	}
	markfirst = wp->dot >= wp->mark;

	wp->eflags &= ~SAVEDEL;
	vskill(wp);
	update(wp);
	vget(wp);

	/*
	 * Now restore old position of cursor and mark
	 */
	if (markfirst) {
		setmark(wp, wp->dot);
		wp->dot += savesize;
	} else {
		setmark(wp, wp->dot+savesize);
	}
}

/*
 * Set name of current take buffer
 */
EXPORT void
vtname(wp)
	ewin_t	*wp;
{
	Uchar	name[NAMESIZE];

	if (! getcmdline(wp, name, sizeof (name), "Take Buffer: "))
		return;
	settakename(wp, name);
}

/*
 * Write the content of the current take buffer into a file
 */
EXPORT void
vwrtake(wp)
	ewin_t	*wp;
{
	FILE	*file;
	Uchar	name[NAMESIZE];

	if (! getcmdline(wp, name, sizeof (name), "\\ to file: "))
		return;
	if ((file = opensyserr(wp, name, "ctwub")) == NULL)
		return;
	fcopy(wp, takefile, file, takesize, TAKEBUF, "FILE");
	fclose(file);
}
