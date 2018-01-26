/* @(#)cursorcmds.c	1.34 09/07/09 Copyright 1984-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)cursorcmds.c	1.34 09/07/09 Copyright 1984-2009 J. Schilling";
#endif
/*
 *	Commands that deal with cursor movement
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
#include "terminal.h"

EXPORT	BOOL	dosnl		__PR((ewin_t *wp, epos_t pos));
EXPORT	void	vforw		__PR((ewin_t *wp));
EXPORT	void	vsforw		__PR((ewin_t *wp));
EXPORT	void	vwforw		__PR((ewin_t *wp));
EXPORT	void	vswforw		__PR((ewin_t *wp));
EXPORT	void	vrev		__PR((ewin_t *wp));
EXPORT	void	vsrev		__PR((ewin_t *wp));
EXPORT	void	vwrev		__PR((ewin_t *wp));
EXPORT	void	vswrev		__PR((ewin_t *wp));
EXPORT	void	vup		__PR((ewin_t *wp));
EXPORT	void	vsup		__PR((ewin_t *wp));
EXPORT	void	vpup		__PR((ewin_t *wp));
EXPORT	void	vspup		__PR((ewin_t *wp));
EXPORT	void	vdown		__PR((ewin_t *wp));
EXPORT	void	vsdown		__PR((ewin_t *wp));
EXPORT	void	vpdown		__PR((ewin_t *wp));
EXPORT	void	vspdwn		__PR((ewin_t *wp));
EXPORT	void	vpageup		__PR((ewin_t *wp));
EXPORT	void	vspageup	__PR((ewin_t *wp));
EXPORT	void	vpagedwn	__PR((ewin_t *wp));
EXPORT	void	vspagedwn	__PR((ewin_t *wp));
EXPORT	void	vend		__PR((ewin_t *wp));
EXPORT	void	vsend		__PR((ewin_t *wp));
EXPORT	void	vpend		__PR((ewin_t *wp));
EXPORT	void	vspend		__PR((ewin_t *wp));
EXPORT	void	vbegin		__PR((ewin_t *wp));
EXPORT	void	vsbegin		__PR((ewin_t *wp));
EXPORT	void	vpbegin		__PR((ewin_t *wp));
EXPORT	void	vspbegin	__PR((ewin_t *wp));
EXPORT	void	vtop		__PR((ewin_t *wp));
EXPORT	void	vstop		__PR((ewin_t *wp));
EXPORT	void	vbottom		__PR((ewin_t *wp));
EXPORT	void	vsbottom	__PR((ewin_t *wp));
EXPORT	void	vadjwin		__PR((ewin_t *wp));
EXPORT	void	vredisp		__PR((ewin_t *wp));
EXPORT	void	vltopwin	__PR((ewin_t *wp));
LOCAL	epos_t	srchbrack	__PR((ewin_t *wp, epos_t begin, int ch, epos_t (*)(ewin_t *wp, epos_t, Uchar *, int, int)));
LOCAL	epos_t	searchbrack	__PR((ewin_t *wp, epos_t begin, int ch));
EXPORT	void	vbrack		__PR((ewin_t *wp));

/*
 * Check if dot points to a DOS CR-LF combination.
 * Dot in this case points to the LF character.
 */
EXPORT BOOL
dosnl(wp, pos)
	ewin_t	*wp;
	epos_t	pos;
{
	Uchar	b[2+1];	/* \r\n\0 */

	if (pos <= 0)
		return (FALSE);
	if (extract(wp, pos, b, 2) == 2) {
		if (b[0] == '\r' && b[1] == '\n') {
			return (TRUE);
		}
	}
	return (FALSE);
}

/*
 * Move cursor forwards 'curnum' characters
 */
EXPORT void
vforw(wp)
	ewin_t	*wp;
{
	if (wp->dot == wp->eof) {
		ringbell();
	} else {
		if (wp->eof-wp->dot < wp->curnum)
			wp->dot = wp->eof;
		else
			wp->dot += wp->curnum;

		if (wp->dosmode && (wp->eof - wp->dot) >= 1 && dosnl(wp, wp->dot -1))
			wp->dot += 1;
	}
}

/*
 * Set mark, then move cursor forwards 'curnum' characters
 */
EXPORT void
vsforw(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
	vforw(wp);
}

/*
 * Move cursor forwards 'curnum' words
 */
EXPORT void
vwforw(wp)
	ewin_t	*wp;
{
	if (wp->dot == wp->eof) {
		ringbell();
	} else {
		wp->dot = forwword(wp, wp->dot, wp->curnum);
	}
}

/*
 * Set mark, then move cursor forwards 'curnum' words
 */
EXPORT void
vswforw(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
	vwforw(wp);
}

/*
 * Move cursor backwards 'curnum' characters
 */
EXPORT void
vrev(wp)
	ewin_t	*wp;
{
	if (wp->dot == 0) {
		ringbell();
	} else {
		if (wp->dot < wp->curnum)
			wp->dot = 0;
		else
			wp->dot -= wp->curnum;

		if (wp->dosmode && wp->dot >= 1 && dosnl(wp, wp->dot -1))
			wp->dot -= 1;
	}
}

/*
 * Set mark, then move cursor backwards 'curnum' characters
 */
EXPORT void
vsrev(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
	vrev(wp);
}

/*
 * Move cursor backwards 'curnum' words
 */
EXPORT void
vwrev(wp)
	ewin_t	*wp;
{
	if (wp->dot == 0) {
		ringbell();
	} else {
		/*
		 * Count == 1 moves to the begining of current word.
		 * The rest moves back for 'curnum' words.
		 */
		wp->dot = revword(wp, wp->dot, wp->curnum);
	}
}

/*
 * Set mark, then move cursor backwards 'curnum' words
 */
EXPORT void
vswrev(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
	vwrev(wp);
}

/*
 * Move cursor up 'curnum' lines
 */
EXPORT void
vup(wp)
	ewin_t	*wp;
{
	epos_t	savedot = wp->dot;

	/*
	 * Count == 1 moves to the begining of current line.
	 * The rest moves back for 'curnum' lines.
	 */
	wp->dot = revline(wp, wp->dot, wp->curnum+1);
	wp->dot = findcol(wp, wp->column, wp->dot);

	if (wp->dosmode && wp->dot >= 1 && dosnl(wp, wp->dot -1))
		wp->dot -= 1;

	wp->eflags &= ~COLUPDATE;
	if (savedot == wp->dot)
		ringbell();
}

/*
 * Set mark, then move cursor up 'curnum' lines
 */
EXPORT void
vsup(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
	vup(wp);
}

/*
 * Move cursor up 'curnum' paragraphs
 */
EXPORT void
vpup(wp)
	ewin_t	*wp;
{
	if (wp->dot == 0) {
		ringbell();
	} else {
#ifdef	_old_revpara_
		/*
		 * Count == 1 moves to the begining of current paragraph.
		 * The rest moves back for 'curnum' paragraphs.
		 */
		wp->dot = revpara(wp, wp->dot, wp->curnum+1);
#else
		wp->dot = revpara(wp, wp->dot, wp->curnum);
#endif
	}
}

/*
 * Set mark, then move cursor up 'curnum' paragraphs
 */
EXPORT void
vspup(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
	vpup(wp);
}

/*
 * Move cursor down 'curnum' lines
 */
EXPORT void
vdown(wp)
	ewin_t	*wp;
{
	if (wp->dot == wp->eof) {
		ringbell();
	} else {
		wp->dot = forwline(wp, wp->dot, wp->curnum);
		wp->dot = findcol(wp, wp->column, wp->dot);

		if (wp->dosmode && (wp->eof - wp->dot) >= 1 && dosnl(wp, wp->dot -1))
			wp->dot -= 1;
	}
	wp->eflags &= ~COLUPDATE;
}

/*
 * Set mark, then move cursor down 'curnum' lines
 */
EXPORT void
vsdown(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
	vdown(wp);
}

/*
 * Move cursor down 'curnum' paragraphs
 */
EXPORT void
vpdown(wp)
	ewin_t	*wp;
{
	if (wp->dot == wp->eof) {
		ringbell();
	} else {
		wp->dot = forwpara(wp, wp->dot, wp->curnum);
	}
}

/*
 * Set mark, then move cursor down 'curnum' paragraphs
 */
EXPORT void
vspdwn(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
	vpdown(wp);
}

/*
 * Move cursor up 'curnum' pages
 */
EXPORT void
vpageup(wp)
	ewin_t	*wp;
{
	if (wp->dot == 0) {
		ringbell();
	} else {
/*cdbg("up: %d", wp->curnum * (wp->psize + 1) - wp->optline);*/
		wp->dot = revline(wp, wp->window, wp->curnum * (wp->psize + 1) - wp->optline);
#ifdef	__comment__
/* XXX	??? */
		wp->dot = revline(wp, wp->dot, wp->curnum * (wp->psize + 1));
		wp->window = revline(wp, wp->window, wp->curnum * (wp->psize + 1));
#endif
	}
}

/*
 * Set mark, then move cursor up 'curnum' pages
 */
EXPORT void
vspageup(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
	vpageup(wp);
}

/*
 * Move cursor down 'curnum' pages
 */
EXPORT void
vpagedwn(wp)
	ewin_t	*wp;
{
	if (wp->dot >= wp->eof) {
		ringbell();
	} else {
/*cdbg("down: %d", wp->curnum * (wp->psize - 2) + wp->optline);*/
		wp->dot = forwline(wp, wp->window, wp->curnum * (wp->psize - 2) + wp->optline);
#ifdef	__comment__
/* XXX	??? */
		wp->dot = forwline(wp, wp->dot, wp->curnum * (wp->psize - 1));
		wp->window = forwline(wp, wp->window, wp->curnum * (wp->psize - 1));
#endif
	}
}

/*
 * Set mark, then move cursor down 'curnum' pages
 */
EXPORT void
vspagedwn(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
	vpagedwn(wp);
}

/*
 * Move cursor to the end of current line,
 * then go back 'curnum' characters
 *
 * As the search operation returns the position of the '\n',
 * we must go back one character even if 'curnum' is '1'.
 */
EXPORT void
vend(wp)
	ewin_t	*wp;
{
	wp->dot = search(wp, wp->dot, UC "\n", 1, 0);
	if (wp->dot > wp->eof)
		wp->dot = wp->eof + 1;

	vrev(wp);
}

/*
 * Set mark, then move cursor to the end of current line,
 * then go back 'curnum' characters
 */
EXPORT void
vsend(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
	vend(wp);
}

/*
 * Move cursor to the end of current paragraph,
 * then go back 'curnum-1' words
 */
EXPORT void
vpend(wp)
	ewin_t	*wp;
{
	wp->dot = forwpara(wp, wp->dot, (ecnt_t)1);
	/*
	 * Go back one for the overshoot and one for the space before
	 */
	if (wp->dot < wp->eof)
		wp->dot -= 2;
	wp->curnum--;
	if (wp->curnum > 0)
		vwrev(wp);
}

/*
 * Set mark, then move cursor to the end of current paragraph,
 * then go back 'curnum-1' words
 */
EXPORT void
vspend(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
	vpend(wp);
}

/*
 * Move cursor to the beginning of current line,
 * then go forwards 'curnum' characters
 */
EXPORT void
vbegin(wp)
	ewin_t	*wp;
{
	wp->dot = revline(wp, wp->dot, (ecnt_t)1);
	wp->curnum--;
	vforw(wp);
}

/*
 * Set mark, then move cursor to the beginning of current line,
 * then go forwards 'curnum' characters
 */
EXPORT void
vsbegin(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
	vbegin(wp);
}

/*
 * Move cursor to the beginning of current paragraph,
 * then go forwards 'curnum-1' words
 */
EXPORT void
vpbegin(wp)
	ewin_t	*wp;
{
	wp->dot = revpara(wp, wp->dot, (ecnt_t)1);
	wp->curnum--;
	vwforw(wp);
}

/*
 * Set mark, then move cursor to the beginning of current paragraph,
 * then go forwards 'curnum-1' words
 */
EXPORT void
vspbegin(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
	vpbegin(wp);
}

/*
 * Move cursor to the beginning of the file
 *
 * If 'curnum' if > 0, move cursor to line 'curnum' from top of file
 */
EXPORT void
vtop(wp)
	ewin_t	*wp;
{
	if (wp->dot == 0 && wp->curnum == 1)
		ringbell();
	wp->dot = forwline(wp, (epos_t)0, wp->curnum-1);
}

/*
 * Set mark, then move cursor to the beginning of the file
 *
 * If 'curnum' if > 0, move cursor to line 'curnum' from top of file
 */
EXPORT void
vstop(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
	vtop(wp);
}

/*
 * Move cursor to the end of the file
 *
 * If 'curnum' if > 0, move cursor backwards 'curnum' lines from end of file
 */
EXPORT void
vbottom(wp)
	ewin_t	*wp;
{
	if (wp->curnum <= 1) {
		if (wp->dot == wp->eof)
			ringbell();
		else
			wp->dot = wp->eof;
	} else {
		wp->dot = revline(wp, wp->eof, wp->curnum);
		wp->curnum = 1;
		vend(wp);
	}
}

/*
 * Move cursor to the end of the file
 *
 * If 'curnum' if > 0, move cursor backwards 'curnum' lines from end of file
 */
EXPORT void
vsbottom(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
	vbottom(wp);
}

/*
 * Adjust window
 */
EXPORT void
vadjwin(wp)
	ewin_t	*wp;
{
	setwindow(wp);
}

/*
 * Redisplay window content
 */
EXPORT void
vredisp(wp)
	ewin_t	*wp;
{
	CLEAR_SCREEN(wp);
	refreshmsg(wp);
	MOVE_CURSOR(wp, 1, 0);
	typescreen(wp, wp->window, 0, wp->eof);
}

/*
 * Make current line be the top line in window
 */
EXPORT void
vltopwin(wp)
	ewin_t	*wp;
{
	wp->dot = forwline(wp, wp->dot, (ecnt_t)(wp->optline -1));
	update(wp);
	setwindow(wp);
}

char	brack[] = "([{<)]}>";
char	*brclose = &brack[4];
struct br {
	Uchar	*br_type;
	Uchar	*br_pat;
} br[] = {
	{UC "()", UC "[()]"},
	{UC "[]", UC "[[\\]]"},
	{UC "{}", UC "[{}]"},
	{UC "<>", UC "[<>]"},
};

/*
 * Find matching bracket
 */
LOCAL epos_t
srchbrack(wp, begin, ch, sfunc)
	ewin_t	*wp;
	epos_t	begin;
	Uchar	ch;
	epos_t	(*sfunc) __PR((ewin_t *wp, epos_t, Uchar *, int, int));
{
	Uchar	b[2];
	Uchar	*spat;
	int	 plen;
	int	 n = 1;
	struct br *brp = br;

	while (!strchr(C brp->br_type, (char)ch))
		brp++;
	spat = brp->br_pat;
	plen = strlen(C spat);

	begin++;
	while (n > 0) {
		begin = (*sfunc)(wp, begin, spat, plen, 0);
		if (begin > wp->eof)
			return (begin);
		if (extract(wp, begin-1, b, 1) != 1)
			return (wp->eof+2);
/*		writeerr("C: '%c' p %d", b[0], begin);sleep(1);*/
		if (b[0] == ch)
			n++;
		else
			n--;
	}
	return (--begin);
}

/*
 * Search for matching backet, check if we need to search forwards or backwards
 */
LOCAL epos_t
searchbrack(wp, begin, ch)
	ewin_t	*wp;
	epos_t	begin;
	Uchar	ch;
{
	BOOL	omagic;

	omagic = wp->magic;
	wp->magic = TRUE;
	if (strchr(brclose, (char)ch)) {
		begin = srchbrack(wp, begin, ch, reverse);
	} else {
		begin = srchbrack(wp, begin, ch, search);
	}
	wp->magic = omagic;
	return (begin);
}

/*
 * Move cursor to matching bracket
 */
EXPORT void
vbrack(wp)
	ewin_t	*wp;
{
	Uchar	b[2];
	epos_t	newdot;

	if (extract(wp, wp->dot, b, 1) != 1 || !strchr(brack, (char)b[0])) {
		ringbell();
	} else {
		newdot = searchbrack(wp, wp->dot, b[0]);
		if (newdot > wp->eof)
			not_found(wp);
		else
			wp->dot = newdot;
	}
}
