/* @(#)movedot.c	1.22 09/01/04 Copyright 1984-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)movedot.c	1.22 09/01/04 Copyright 1984-2009 J. Schilling";
#endif
/*
 *	Functions that are used to move the dot around.
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
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

/*
 * The buffer structure of ved has no idea of a line, so everything that
 * moves the 'dot' more than a single character needs to use the
 * appropriate search functions.
 */

#include "ved.h"
#include "movedot.h"

LOCAL	Uchar	whitespace[] = "^![ \t]!$";
EXPORT	Uchar	notwhitespace[] = "[^ \t]";		/* Used by cmds.c/screen.c */

#ifdef	__needed__
LOCAL	Uchar	inword[] = "[0-9_A-Za-z]";
#endif
EXPORT	Uchar	notinword[] = "[^0-9_A-Za-z]!$";	/* Used by tags.c */

LOCAL	Uchar	newline[] = "\n";
LOCAL	Uchar	notnewline[] = "[^\n]";
LOCAL	Uchar	dosnotnewline[] = "[^\r\n]";

EXPORT	epos_t	forwword	__PR((ewin_t *wp, epos_t begin, ecnt_t n));
EXPORT	epos_t	revword		__PR((ewin_t *wp, epos_t begin, ecnt_t n));
EXPORT	epos_t	forwline	__PR((ewin_t *wp, epos_t begin, ecnt_t n));
EXPORT	epos_t	revline		__PR((ewin_t *wp, epos_t begin, ecnt_t n));
EXPORT	epos_t	forwpara	__PR((ewin_t *wp, epos_t begin, ecnt_t n));
EXPORT	epos_t	revpara		__PR((ewin_t *wp, epos_t begin, ecnt_t n));
LOCAL	epos_t	move_forward	__PR((ewin_t *wp, epos_t begin, ecnt_t n, epos_t (*func)(ewin_t *wp, epos_t begin)));
LOCAL	epos_t	move_reverse	__PR((ewin_t *wp, epos_t begin, ecnt_t n, epos_t (*func)(ewin_t *wp, epos_t begin)));
LOCAL	epos_t	searchword	__PR((ewin_t *wp, epos_t begin));
LOCAL	epos_t	srevword	__PR((ewin_t *wp, epos_t begin));
LOCAL	epos_t	searchline	__PR((ewin_t *wp, epos_t begin));
LOCAL	epos_t	srevline	__PR((ewin_t *wp, epos_t begin));
LOCAL	epos_t	searchpara	__PR((ewin_t *wp, epos_t begin));
LOCAL	epos_t	srevpara	__PR((ewin_t *wp, epos_t begin));

/*
 * Move forwards 'n' words from 'begin'. Stop at the beginning of the word.
 */
EXPORT epos_t
forwword(wp, begin, n)
	ewin_t	*wp;
	epos_t	begin;
	ecnt_t	n;
{
	if (n <= 0)
		return (begin);

	return (move_forward(wp, begin, n, searchword));
}

/*
 * Move backwards 'n' words from 'begin'. Stop at the beginning of the word.
 * If we are inside a word, the first search loop will take us to the
 * beginning of this word.
 */
EXPORT epos_t
revword(wp, begin, n)
	ewin_t	*wp;
	epos_t	begin;
	ecnt_t	n;
{
	if (n <= 0)
		return (begin);
	if (begin > wp->eof)
		begin = wp->eof;
	begin++;		/* Compensate 'compensation' in 'reverse' */

	return (move_reverse(wp, begin, n, srevword));
}

/*
 * Move forwards 'n' lines from 'begin'. Stop at the beginning of the line.
 */
EXPORT epos_t
forwline(wp, begin, n)
	ewin_t	*wp;
	epos_t	begin;
	ecnt_t	n;
{
	if (n <= 0)
		return (begin);

	return (move_forward(wp, begin, n, searchline));
}

/*
 * Move backwards 'n' lines from 'begin'. Stop at the beginning of the line.
 * If we are inside a line, the first search loop will take us to the
 * beginning of this line.
 */
EXPORT epos_t
revline(wp, begin, n)
	ewin_t	*wp;
	epos_t	begin;
	ecnt_t	n;
{
	if (n <= 0)
		return (begin);
	if (begin > wp->eof)
		begin = wp->eof;
	begin++;		/* Compensate 'compensation' in 'reverse' */

	return (move_reverse(wp, begin, n, srevline));
}

/*
 * Move forwards 'n' paragraphs from 'begin'.
 * Stop at the beginning of the parapragph.
 */
EXPORT epos_t
forwpara(wp, begin, n)
	ewin_t	*wp;
	epos_t	begin;
	ecnt_t	n;
{
	if (n <= 0)
		return (begin);

	return (move_forward(wp, begin, n, searchpara));
}


/*
 * Move backwards 'n' paragraphs from 'begin'.
 * Stop at the beginning of the paragraph.
 * If we are inside a paragraph, the first search loop will take us to the
 * beginning of this paragraph.
 */
EXPORT epos_t
revpara(wp, begin, n)
	ewin_t	*wp;
	epos_t	begin;
	ecnt_t	n;
{
	if (n <= 0)
		return (begin);
#ifdef	_old_revpara_			/* See cursorcmds.c */
	if (begin >= wp->eof)
		begin = wp->eof-1;
	begin += 2;		/* Compensate 'compensation' in 'reverse' */
#else
	if (begin > wp->eof)
		begin = wp->eof;
	begin++;		/* Compensate 'compensation' in 'reverse' */
#endif

	return (move_reverse(wp, begin, n, srevpara));
}

/*
 * Do actual forward movement with repeat counter.
 * Need to do search in 'magic' mode.
 */
LOCAL epos_t
move_forward(wp, begin, n, func)
	ewin_t	*wp;
	epos_t	begin;
	ecnt_t	n;
	register epos_t (*func) __PR((ewin_t *wp, epos_t begin));
{
	register epos_t	newdot = begin;
		BOOL	omagic = wp->magic;

	wp->magic = TRUE;

	while (--n >= 0) {
		if ((newdot = (*func)(wp, newdot)) > wp->eof) {
			newdot = wp->eof;
			break;
		}
	}
	wp->magic = omagic;
	return (newdot);
}

/*
 * Do actual backward movement with repeat counter.
 * Need to do search in 'magic' mode.
 */
LOCAL epos_t
move_reverse(wp, begin, n, func)
	ewin_t	*wp;
	epos_t	begin;
	ecnt_t	n;
	register epos_t (*func) __PR((ewin_t *wp, epos_t begin));
{
	register epos_t	newdot = begin;
		BOOL	omagic = wp->magic;

	wp->magic = TRUE;

	while (--n >= 0) {
		if ((newdot = (*func)(wp, newdot)) > wp->eof) {
			newdot = 0;
			break;
		}
	}
	wp->magic = omagic;
	return (newdot);
}

/*
 * Search forwards one word (to the beginning of the next word).
 * A word is defined as a group of characters delimited by whitespace.
 * This only works in 'magic' mode.
 */
LOCAL epos_t
searchword(wp, begin)
	ewin_t	*wp;
	epos_t	begin;
{
	begin = search(wp, begin, whitespace, sizeof (whitespace)-1, 0);
	begin = search(wp, begin, notwhitespace, sizeof (notwhitespace)-1, 0);
	if (begin <= wp->eof)
		return (begin-1);
	return (begin);
}

/*
 * Search backwards one word (to the beginning of the previous word if we
 * are at the beginning of a word else to the beginning of the current word).
 * A word is defined as a group of characters delimited by whitespace.
 * This only works in 'magic' mode.
 */
LOCAL epos_t
srevword(wp, begin)
	ewin_t	*wp;
	epos_t	begin;
{
	begin = reverse(wp, begin, notwhitespace, sizeof (notwhitespace)-1, 0);
	return (reverse(wp, begin, whitespace, sizeof (whitespace)-1, 0));
}

/*
 * Search forwards one line (to the beginning of the next line).
 * May only work in 'magic' mode in future.
 */
LOCAL epos_t
searchline(wp, begin)
	ewin_t	*wp;
	epos_t	begin;
{
	return (search(wp, begin, newline, sizeof (newline)-1, 0));
}

/*
 * Search backwards one line (to the beginning of the previous line if we
 * are at the beginning of a line else to the beginning of the current line).
 * May only work in 'magic' mode in future.
 */
LOCAL epos_t
srevline(wp, begin)
	ewin_t	*wp;
	epos_t	begin;
{
	return (reverse(wp, begin, newline, sizeof (newline)-1, 0));
}

/*
 * Search forwards one paragraph (to the beginning of the next paragraph).
 * A paragraph is defined as a group of characters delimited by two newlines.
 * This only works in 'magic' mode.
 */
LOCAL epos_t
searchpara(wp, begin)
	ewin_t	*wp;
	epos_t	begin;
{
	epos_t	prev;
	BOOL	wasdos;

	begin = search(wp, begin, newline, sizeof (newline)-1, 0);
	if (begin > wp->eof)
		return (begin);
	/*
	 *  Find two newlines that immediately follow each other
	 */
	do {
		prev = begin;
		if (wp->dosmode)
			wasdos = dosnl(wp, begin);
		else
			wasdos = FALSE;
		begin = search(wp, prev, newline, sizeof (newline)-1, 0);
		if (begin > wp->eof)
			return (begin);
	} while (begin != prev+1+(wasdos?1:0));

	if (wp->dosmode)
		begin = search(wp, begin, dosnotnewline, sizeof (dosnotnewline)-1, 0);
	else
		begin = search(wp, begin, notnewline, sizeof (notnewline)-1, 0);

	if (begin <= wp->eof)
		return (begin-1);
	else
		return (begin);
}

/*
 * Search backwards one paragraph (to the beginning of the previous paragraph
 * if we are at the beginning of a paragraph else to the beginning of the
 * current paragraph).
 * A paragraph is defined as a group of characters delimited by two newlines.
 * First search for a non-empty line to be sure to be inside a paragraph.
 * This only works in 'magic' mode.
 */
LOCAL epos_t
srevpara(wp, begin)
	ewin_t	*wp;
	epos_t	begin;
{
	epos_t	prev;
	BOOL	wasdos;

	if (wp->dosmode)
		begin = reverse(wp, begin, dosnotnewline, sizeof (dosnotnewline)-1, 0);
	else
		begin = reverse(wp, begin, notnewline, sizeof (notnewline)-1, 0);
	begin = reverse(wp, begin, newline, sizeof (newline)-1, 0);
	if (begin > wp->eof)
		return (begin);
	/*
	 *  Find two newlines that immediately follow each other
	 */
	do {
		prev = begin;
		begin = reverse(wp, prev, newline, sizeof (newline)-1, 0);
		if (begin > wp->eof)
			return (begin);

		if (wp->dosmode)
			wasdos = dosnl(wp, begin);
		else
			wasdos = FALSE;
	} while (begin != prev-1-(wasdos?1:0));

	if (begin < wp->eof)
		return (begin+1+(wasdos?1:0));
	else
		return (begin);
}
