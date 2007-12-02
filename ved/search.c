/* @(#)search.c	1.27 06/09/13 Copyright 1984-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)search.c	1.27 06/09/13 Copyright 1984-2004 J. Schilling";
#endif
/*
 *	Search functions for VED
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

/*
 * This are the low level support routines to search forwards or backwards
 * through the buffer. High level interfaces are found in searchcmds.c and
 * movedot.c
 *
 * The package implements two levels of of optimazation:
 * If the pattern does not contain special characters of the pattern matcher
 * simplesearch() and simplereverse() are used. If the pattern in addition
 * is a one byte pattern, searchchar() and srevchar() are used.
 * All other search operations are handled directly inside search()
 * and reverse() by using the pattern matcher.
 *
 * All routines return a value > eof to indicate that the requested object
 * could not be found. Some routines decrement/incremet the value 'begin' so
 * we must return eof+2 to be different from all possible 'correct' values.
 * If the pattern is bad, eof+3 is returned to indicate that no 'not found'
 * message should be displayed.
 *
 *	Return values:
 *		eof+2	Pattern not found
 *		eof+3	Bad Pattern
 *		other	Pattern found
 */
#include "ved.h"
#include "buffer.h"
#include <schily/patmatch.h>

#define	SEARCHSIZE 4096

EXPORT	BOOL	issimple	__PR((Uchar* pattern, int length));
LOCAL	epos_t	searchchar	__PR((ewin_t *wp, epos_t begin, int ch));
EXPORT	epos_t	srevchar	__PR((ewin_t *wp, epos_t begin, int ch));
EXPORT	epos_t	search		__PR((ewin_t *wp, epos_t begin, Uchar* pattern, int patsize, BOOL domark));
EXPORT	epos_t	reverse		__PR((ewin_t *wp, epos_t begin, Uchar* pattern, int patsize, BOOL domark));
LOCAL	BOOL	simplesearch	__PR((Uchar* searchstr, int searchlen, Uchar* pattern, int patlen, Uchar** beginp, Uchar** endp));
LOCAL	BOOL	simplereverse	__PR((Uchar* searchstr, int searchlen, Uchar* pattern, int patlen, Uchar** beginp, Uchar** endp));
LOCAL	epos_t	searchdone	__PR((ewin_t *wp, epos_t start, epos_t ret, BOOL domark));

/*
 * Check if the pattern contains special characters of the pattern matcher.
 * Do not look for characters that may be used only inside of a character set.
 */
EXPORT BOOL
issimple(pattern, length)
	register Uchar	*pattern;
	register int	length;
{
	while (length-- > 0) {
		switch (*pattern++) {

		case ALT:	case REP:	case NIL:
		case STAR:	case LBRACK:	case RBRACK:
		case LCLASS:	case RCLASS:	case QUOTE:
		case ANY:	case START:	case END:
								return (FALSE);
		}
	}
	return (TRUE);
}

/*
 * Search forwards for a one character 'pattern'.
 */
LOCAL epos_t
searchchar(wp, begin, ch)
	ewin_t	*wp;
	epos_t	begin;
	Uchar	ch;
{
	register headr_t	*linkp;
	register Uchar		*stuff;
	register int		linksize;
		headr_t		*passlinkp;
		int		pos;

	findpos(wp, begin, &passlinkp, &pos);
	linkp = passlinkp;
	linksize = linkp->size - pos;
	stuff = linkp->cont + pos;

	for (;;) {
		while (linksize--) {
			if (*stuff++ == ch)
				return (begin);
			begin++;
		}
		if ((linkp = linkp->next) == NULL)
			return (wp->eof+2);
		readybuffer(wp, linkp);
		linksize = linkp->size;
		stuff = linkp->cont;
	}
}

/*
 * Search backwards for a one character 'pattern'.
 */
EXPORT epos_t
srevchar(wp, begin, ch)
	ewin_t	*wp;
	epos_t	begin;
	Uchar	ch;
{
	register headr_t	*linkp;
	register Uchar		*stuff;
	register int		linksize;
		headr_t		*passlinkp;
		int		pos;

	findpos(wp, begin, &passlinkp, &pos);
	linkp = passlinkp;
	linksize = pos + 1;
	stuff = linkp->cont + pos;

	for (;;) {
		while (linksize--) {
			if (*stuff-- == ch)
				return (begin);
			begin--;
		}
		if ((linkp = linkp->prev) == NULL)
			return (wp->eof+2);
		readybuffer(wp, linkp);
		linksize = linkp->size;
		stuff = linkp->cont + linkp->size - 1;
	}
}

/*
 * Search forwards for 'pattern' doing the appropriate optimizations.
 */
EXPORT epos_t
search(wp, begin, pattern, patsize, domark)
	ewin_t	*wp;
	epos_t	begin;
	Uchar	*pattern;
	int	patsize;
	BOOL	domark;
{
	Uchar	searchstr[SEARCHSIZE + 1];
	int	aux[SEARCHSIZE];	/* Nobody should be able to type such*/
	int	state[SEARCHSIZE+1];	/* a long pattern - avoid malloc()   */
	int	alt;
	int	start;
	int	readsize;
	int	firsttime;
	int	midline;
	int	nl;
	Uchar	*strstart;
	Uchar	*place;
	epos_t	pos;

	if (begin >= wp->eof)
		return (wp->eof+2);
	if (patsize == 0)
		return (wp->eof+2);

	if (wp->magic == FALSE || issimple(pattern, patsize)) {
		/*
		 * Search without the pattern matcher.
		 */
		if (patsize == 1) {		/* single character pattern */
			if ((pos = searchchar(wp, begin, *pattern)) < wp->eof)
				return (searchdone(wp, pos, pos+1, domark));
			else
				return (wp->eof+2);
		}

/*		while (readsize = extr_line(begin, searchstr, SEARCHSIZE)) {*/
		while ((readsize = extract(wp, begin, searchstr, SEARCHSIZE)) != 0) {
			if (simplesearch(searchstr, readsize,
					pattern, patsize, &strstart, &place)) {

				return (searchdone(wp, begin+
					(int)(strstart-searchstr),
					begin+(int)(place-searchstr),
					domark));
			}
			begin += readsize;
#ifdef	_use_extr_line_
			/*extr_line*/
			if (begin < wp->eof && searchstr[readsize-1] != '\n') {
#else
			if (begin < wp->eof) {
#endif
				begin -= patsize - 1;
			}
		}
	} else {
		/*
		 * First compile the pattern.
		 */
		if (patsize > sizeof (aux)/sizeof (aux[0])) {
			writeerr(wp, "PATTERN TOO LONG");
			return (wp->eof+3);
		}
		alt = patcompile(pattern, patsize, aux);
		if (alt == 0) {
			writeerr(wp, "BAD PATTERN");
			return (wp->eof+3);
		}
		/*
		 * Check if we start searching at the beginning of a line.
		 */
		midline = 0;
		nl = 1;
		if (begin != 0L &&
		    extract(wp, begin-1, searchstr, 1) && searchstr[0] != '\n') {
			nl = 0;  /* No newline found one left to the cursor */
		}

		/*
		 * Then start searching...
		 */
		for (firsttime = 1;
		    (readsize = extr_line(wp, begin, C searchstr, SEARCHSIZE)) != 0;
						firsttime = 0, midline = 0) {
			/*
			 * If the current line does not begin with a new-line
			 * we are in touble. In theory, we need to disable '^'
			 * (begin of line) but this is only a silly approach.
			 */
			if (!nl)
				midline++;
			nl = 0;
			if (searchstr[readsize-1] == '\n') {
				nl++;
				/* XXX Ist das richtig ??? */
/*				searchstr[readsize-1] = '\0';*/
			}
			for (start = 0; start < readsize; start++)
				if ((place = UC patmatch(pattern, aux,
						UC (searchstr - midline),
						start + midline,
						readsize + midline, alt, state)) != NULL)
					if (!firsttime || place > searchstr) {
						/*
						 * Only found if the cursor moved.
						 */
						return (searchdone(wp, begin+start,
							begin+(int)(place-searchstr),
									domark));
					}
			begin += readsize;
			if (begin < wp->eof && !nl)
				begin -= patsize - 1;
		}
	}
	return (wp->eof+2);
}

/*
 * Search backwards for 'pattern' doing the appropriate optimizations.
 * Decrement 'begin' first to make sure we will not find the same place twice.
 */
EXPORT epos_t
reverse(wp, begin, pattern, patsize, domark)
	ewin_t	*wp;
	epos_t	begin;
	Uchar	*pattern;
	int	patsize;
	BOOL	domark;
{
	Uchar	searchstr[SEARCHSIZE + 1];
	int	aux[SEARCHSIZE];	/* Nobody should be able to type such*/
	int	state[SEARCHSIZE+1];	/* a long pattern - avoid malloc()   */
	int	alt;
	int	start;
	int	readsize;
	int	firsttime;
	Uchar	*strstart;
	Uchar	*place;
	Uchar	*bestplace;
	epos_t	pos = wp->dot;

	begin--;
	if (begin <= 0)
		return (wp->eof+2);
	if (patsize == 0)
		return (wp->eof+2);
	if (begin > wp->eof)			/* OK JS */
		return (wp->eof+2);

	if (wp->magic == FALSE || issimple(pattern, patsize)) {
		/*
		 * Search without the pattern matcher.
		 */
		if (patsize == 1) {		/* single character pattern */
			if ((pos = srevchar(wp, begin-1, *pattern)) < wp->eof)
/*			if ((pos = srevchar(wp, begin, *pattern)) < wp->eof)*/
				return (searchdone(wp, pos, pos+1, domark));
			else
				return (wp->eof+2);
		}

/*		while (readsize = retractline(begin, searchstr, SEARCHSIZE)) {*/
		while ((readsize = extract(wp, max(0, begin-SEARCHSIZE), searchstr, (int)min(begin, SEARCHSIZE))) != 0) {
			begin -= readsize;
			if (begin < 0)
				begin = 0;
			if (simplereverse(searchstr, readsize,
					pattern, patsize, &strstart, &place))
				return (searchdone(wp, begin+(int)(strstart-searchstr),
					begin+(int)(place-searchstr), domark));
			if (begin > 0)
				begin += patsize - 1;
		}
	} else {
		/*
		 * First compile the pattern.
		 */
		if (patsize > sizeof (aux)/sizeof (aux[0])) {
			writeerr(wp, "PATTERN TOO LONG");
			return (wp->eof+3);
		}
		alt = patcompile(pattern, patsize, aux);
		if (alt == 0) {
			writeerr(wp, "BAD PATTERN");
			return (wp->eof+3);
		}

		/*
		 * Then start searching...
		 */
		++begin;
		for (firsttime = 1;
			(readsize = retractline(wp, begin, C searchstr, SEARCHSIZE)) != 0;
								firsttime = 0) {
			begin -= readsize;
			/* XXX Ist das richtig ??? */
/*			if (searchstr[readsize-1] == '\n')*/
/*				searchstr[readsize-1] = '\0';*/
			for (start = 0, bestplace = 0; start < readsize; ++start)
				if ((place = UC patmatch(pattern, aux, searchstr,
							start, readsize, alt, state)) != NULL)
					if (!firsttime ||
						place - searchstr < readsize)
						/*
						 * Only found if the cursor moved.
						 */
						if (place > bestplace) {
							/*
							 * Use the longest
							 * possible match.
							 */
							bestplace = place;
							pos = start;
						}
			if (bestplace)
				return (searchdone(wp, begin+pos,
					begin+(int)(bestplace-searchstr), domark));
		}
	}
	return (wp->eof+2);
}

/*
 * Search forwards for a simple pattern.
 * If the pattern could be found, *beginp amd *endp are set.
 * beginp points to the first character that matches,
 * endp points to character the after the last matching character.
 */
LOCAL BOOL
simplesearch(searchstr, searchlen, pattern, patlen, beginp, endp)
	Uchar	*searchstr;	/* buffer to be searched	   */
	int	searchlen;	/* length of buffer to be searched */
	Uchar	*pattern;	/* search pattern		   */
	int	patlen;		/* length of search pattern	   */
	Uchar	**beginp;	/* start 'return' value		   */
	Uchar	**endp;		/* end 'return' value		   */
{
	register Uchar	*str;
	register Uchar	*pat;
	register Uchar	*strstart;
	register Uchar	*patstart;
	register int	n;
	register int	len;
	register int	plen;

	len = searchlen;
	plen = patlen;
	strstart = searchstr;
	patstart = pattern;
	while (len-- >= plen) {
		for (str = strstart, pat = patstart, n = 0;
		    *str == *pat && n < plen;
		    str++, pat++, n++) {
			;
		}
		if (n == plen) {		/* Found */
			*beginp = strstart;
			*endp = str;
			return (TRUE);
		}
		strstart++;
	}
	return (FALSE);
}

/*
 * Search backwards for a simple pattern.
 * If the pattern could be found, *beginp amd *endp are set.
 * beginp points to the first character that matches,
 * endp points to character the after the last matching character.
 */
LOCAL BOOL
simplereverse(searchstr, searchlen, pattern, patlen, beginp, endp)
		Uchar	*searchstr;	/* buffer to be searched	   */
	register int	searchlen;	/* length of buffer to be searched */
		Uchar	*pattern;	/* search pattern		   */
		int	patlen;		/* length of search pattern	   */
		Uchar	**beginp;	/* start 'return' value		   */
		Uchar	**endp;		/* end 'return' value		   */
{
	register Uchar	*str;
	register Uchar	*pat;
	register Uchar	*searchpos;
	register Uchar	*strstart;
	register Uchar	*patstart;
	register int	n;
	register int	plen;

	plen = patlen;
	strstart = searchstr;
	searchpos = strstart + searchlen - plen + 1;
	patstart = pattern;
	while (--searchpos >= strstart) {
		for (str = searchpos, pat = patstart, n = 0;
		    *str == *pat && n < plen;
		    str++, pat++, n++)
			;
		if (n == plen) {		/* Found */
			*beginp = searchpos;
			*endp = str;
			return (TRUE);
		}
	}
	return (FALSE);
}

/*
 * Return the end of the found pattern and set a mark at the start of the
 * found pattern if wanted.
 */
LOCAL epos_t
searchdone(wp, start, ret, domark)
	ewin_t	*wp;
	epos_t	start;
	epos_t	ret;
	BOOL	domark;
{
	if (domark)
		setmark(wp, start);
	return (ret);
}
