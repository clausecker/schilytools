/* @(#)cmds.c	1.45 09/07/13 Copyright 1984-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)cmds.c	1.45 09/07/13 Copyright 1984-2009 J. Schilling";
#endif
/*
 *	Commands that deal with various things that do not apply to other
 *	systematic categories.
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

#include "ved.h"
#include "terminal.h"
#include <schily/signal.h>

LOCAL	BOOL	rawinput;

EXPORT 	void	vnorm		__PR((ewin_t *wp));
EXPORT	void	vsnorm		__PR((ewin_t *wp));
EXPORT	void	vnl		__PR((ewin_t *wp));
EXPORT	void	vsnl		__PR((ewin_t *wp));
EXPORT	void	verror		__PR((ewin_t *wp));
EXPORT	void	modified	__PR((ewin_t *wp));
EXPORT	void	vmode		__PR((ewin_t *wp));
LOCAL	void	modemsg		__PR((ewin_t *wp, char *msg, BOOL flag));
LOCAL	void	modeprint	__PR((ewin_t *wp, char *msg, BOOL flag));
EXPORT	void	vwhere		__PR((ewin_t *wp));
EXPORT	void	vewhere		__PR((ewin_t *wp));
EXPORT	void	vswhere		__PR((ewin_t *wp));
EXPORT	void	vsewhere	__PR((ewin_t *wp));
LOCAL	epos_t	where		__PR((ewin_t *wp, epos_t loc));
EXPORT	void	vquote		__PR((ewin_t *wp));
EXPORT	void	v8cntlq		__PR((ewin_t *wp));
EXPORT	void	v8quote		__PR((ewin_t *wp));
LOCAL	int	tocntrl		__PR((int c));
EXPORT	void	vhex		__PR((ewin_t *wp));
EXPORT	void	vopen		__PR((ewin_t *wp));
EXPORT	void	vsopen		__PR((ewin_t *wp));
EXPORT	void	vhelp		__PR((ewin_t *wp));

/*
 * Insert a regular character 'curnum' times, handle overstrike and autowrap
 */
EXPORT void
vnorm(wp)
	ewin_t	*wp;
{
	register epos_t save = wp->dot;
	register epos_t dels;
	register ecnt_t	n = wp->curnum;
		epos_t	diff = (epos_t)0;

	if (wp->overstrikemode) {
		if ((dels = min(n, wp->eof-wp->dot)) > 0) {
			dispup(wp, wp->dot, wp->dot+dels);
			delete(wp, dels);
			setpos(wp);
			setcursor(wp);
		}
	}
	if (wp->lastch == '\n' && wp->dosmode && !rawinput) {
		while (--n >= 0)
			insert(wp, UC "\r\n", 2L);	/* insert the chars into the file*/
	} else {
		while (--n >= 0)
			insert(wp, &wp->lastch, 1L);	/* insert the char into the file*/
	}
	dispup(wp, wp->dot, save);		/* update display with inserted chars*/

	if (wp->lastch != '\n' &&
	    ((wp->wrapmargin && cursor.hp > (wp->llen - wp->wrapmargin)) ||
	    (wp->maxlinelen && cursor.hp > wp->maxlinelen))) {
		save = wp->dot;
		if ((wp->dot = revword(wp, save, (ecnt_t)1)) > revline(wp, save, (ecnt_t)1)) {
			update(wp);
			diff += wp->dot;
			n = wp->curnum;
			wp->curnum = 1;
			vnl(wp);
			wp->curnum = n;
			diff -= wp->dot;
		}
		wp->dot = save - diff;
	}
	modified(wp);
}

/*
 * Set mark, then insert a regular character 'curnum' times
 */
EXPORT void
vsnorm(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
	vnorm(wp);
}

/*
 * Insert a linefeed character 'curnum' times, handle autoindent if necessary
 */
EXPORT void
vnl(wp)
	ewin_t	*wp;
{
	cpos_t		c;
	BOOL		save;
	epos_t		lbeg;
	epos_t		textbeg;
	long		indent;
	extern Uchar	notwhitespace[];

	if (wp->autoindent) {
		c.vp = 1;
		c.hp = 0;
		lbeg = revline(wp, wp->dot, (ecnt_t)1);	/* an den Anfang der Zeile */
		save = wp->magic;
		wp->magic = TRUE;
		textbeg =
		    search(wp, lbeg, notwhitespace, strlen(C notwhitespace), 0) - 1;
		wp->magic = save;
		if (textbeg >= wp->dot) {
			textbeg = wp->dot;
			wp->dot = lbeg;
			update(wp);
			wp->lastch = '\n';
			vnorm(wp);
			wp->dot = textbeg + wp->curnum;
			return;
		}
/*		writeerr(wp, "%d %d %d", lbeg, textbeg, wp->dot);*/
		countpos(wp, lbeg, textbeg, &c);
		indent = c.hp;
		wp->lastch = '\n';
		vnorm(wp);
		update(wp);
		wp->lastch = '\t';
		wp->curnum = indent / wp->tabstop;
		vnorm(wp);
		update(wp);
		wp->lastch = ' ';
		wp->curnum = indent % wp->tabstop;
		vnorm(wp);
	} else {
		wp->lastch = '\n';
		vnorm(wp);
	}
}

/*
 * Set mark, then insert a linefeed character 'curnum times',
 * handle autoindent if necessary
 */
EXPORT void
vsnl(wp)
	ewin_t	*wp;
{
	setmark(wp, wp->dot);
	vnl(wp);
}

/*
 * Non-existant function or error condition
 */
/* ARGSUSED */
EXPORT void
verror(wp)
	ewin_t	*wp;
{
	ringbell();
}

/*
 * Increment modified flag, set defaultinfo on first modification
 */
EXPORT void
modified(wp)
	ewin_t	*wp;
{
	if (wp->modflg++ == 0)
		defaultinfo(wp, UC "*");
}

/*
 * Change the current modes of ved.
 *
 * Known modes are:
 *	v:	Visible mode
 *	d:	DOS mode
 *	8:	raw8 mode
 *	o:	Overstrike mode
 *	r:	Reset to default mode
 */
EXPORT void
vmode(wp)
	ewin_t	*wp;
{
	Uchar ch;

	writemsg(wp, "Mode?(DORV8)"); MOVE_CURSOR(wp, 0, 12); flush();
	ch = nigchar(wp);

	switch (ch) {

	case 'v':
	case 'V':
		wp->visible = !wp->visible;
		vredisp(wp);
		modemsg(wp, "Visible", wp->visible);
		break;
	case 'd':
	case 'D':
		wp->dosmode = !wp->dosmode;
		vredisp(wp);
		modemsg(wp, "DOS", wp->dosmode);
		break;
	case '8':
		wp->raw8 = !wp->raw8;
		vredisp(wp);
		modemsg(wp, "Raw8", wp->raw8);
		break;
	case 'o':
	case 'O':
		wp->overstrikemode = !wp->overstrikemode;
		modemsg(wp, "Overstrike", wp->overstrikemode);
		break;
	case 'r':
	case 'R':
		if (wp->visible) {
			wp->visible = FALSE;
			vredisp(wp);
		}
		wp->overstrikemode = FALSE;
		writemsg(wp, "Modes Reset");
		break;
	case '\r':
	case '\n':
		MOVE_CURSOR_ABS(wp, 1, 0);
		modeprint(wp, "Visible", wp->visible);
		modeprint(wp, "DOS", wp->dosmode);
		modeprint(wp, "Raw8", wp->raw8);
		modeprint(wp, "Overstrike", wp->overstrikemode);
		wait_for_confirm(wp);
		vredisp(wp);
		break;
	default:
		abortmsg(wp);
		break;
	}
}

LOCAL void
modemsg(wp, msg, flag)
	ewin_t	*wp;
	char	*msg;
	BOOL	flag;
{
	writemsg(wp, "%s mode %s", msg, flag?"on":"off");
}

LOCAL void
modeprint(wp, msg, flag)
	ewin_t	*wp;
	char	*msg;
	BOOL	flag;
{
	printscreen(wp, "%s mode %s\n", msg, flag?"on":"off");
}

/*
 * Count and print the number of lines from top of file to cursor
 */
EXPORT void
vwhere(wp)
	ewin_t	*wp;
{
	writemsg(wp, "+Line: %lld", (Llong)where(wp, wp->dot));
}

/*
 * Count and print the number of lines from end of file to cursor
 */
EXPORT void
vewhere(wp)
	ewin_t	*wp;
{
	writemsg(wp, "-Line: %lld", (Llong)(where(wp, wp->eof) - where(wp, wp->dot) + 1));
}

/*
 * Count and print the number of lines from top of file to mark
 */
EXPORT void
vswhere(wp)
	ewin_t	*wp;
{
	if (wp->markvalid)
		writemsg(wp, "+Line: %lld", (Llong)where(wp, wp->mark));
	else
		nomarkmsg(wp);
}

/*
 * Count and print the number of lines from end of file to mark
 */
EXPORT void
vsewhere(wp)
	ewin_t	*wp;
{
	if (wp->markvalid)
		writemsg(wp, "-Line: %lld", (Llong)(where(wp, wp->eof) - where(wp, wp->mark) + 1));
	else
		nomarkmsg(wp);
}

/*
 * Count the number of lines from top of file to location
 */
LOCAL epos_t
where(wp, loc)
		ewin_t	*wp;
	register epos_t loc;
{
	register epos_t newdot = (epos_t)0;
	register epos_t	line = 0;

	while (newdot <= loc) {
		line++;
		if ((newdot = search(wp, newdot, UC "\n", 1, 0)) > wp->eof) {
			newdot = wp->eof;
			break;
		}
	}
	return (line);
}

/*
 * Quote next character to control character and insert it 'curnum' times
 */
EXPORT void
vquote(wp)
	ewin_t	*wp;
{
	writemsg(wp, "^ Quote:");
	flush();
	wp->lastch = nigchar(wp);
	wp->lastch = tocntrl(wp->lastch);
	rawinput = TRUE;
	vnorm(wp);
	rawinput = FALSE;
	writemsg(wp, C NULL);
}

/*
 * Quote next character to 8 bit control character and insert it 'curnum' times
 */
EXPORT void
v8cntlq(wp)
	ewin_t	*wp;
{
	writemsg(wp, "~^ Quote:");
	flush();
	wp->lastch = nigchar(wp);
	wp->lastch = tocntrl(wp->lastch);
	wp->lastch ^= 0200;
	vnorm(wp);
	writemsg(wp, C NULL);
}

/*
 * Quote next character to 8 bit character and insert it 'curnum' times
 */
EXPORT void
v8quote(wp)
	ewin_t	*wp;
{
	writemsg(wp, "~ Quote:");
	flush();
	wp->lastch = nigchar(wp);
	wp->lastch ^= 0200;
	vnorm(wp);
	writemsg(wp, C NULL);
}

/*
 * Change a character into a control character
 * XXX with the curent bindings, ved would only work in an ASCII world
 * XXX or on a world where the low 7 bits are ASCII compatible.
 */
LOCAL int
tocntrl(c)
	register Uchar c;
{
	if (c == '?')
		return (DEL);

	/*
	 * Convert into capital letters
	 */
	if (c >= 'a' && c <= 'z')
		c += 'A' - 'a';

	/*
	 * Change only the valid range into a control character.
	 */
	if (c >= (unsigned)'@' && c <= (unsigned)('@' + 0x1F))
		c &= 0x1F;

	return (c);
}

/*
 * Quote next character from hex input and insert it 'curnum' times
 */
EXPORT void
vhex(wp)
	ewin_t	*wp;
{
	int	i;
	char	hbuf[5];

	hbuf[0] = '0';
	hbuf[1] = 'x';
	if (getcmdline(wp, UC &hbuf[2], sizeof (hbuf) - 2, "Quote: 0x")) {
		if (*astoi(hbuf, &i) != '\0')
			writeerr(wp, "BAD ARG");
		else {
			wp->lastch = (Uchar)i;
			vnorm(wp);
		}
	}
}

/*
 * Insert 'curnum' newlines after the cursor, leave cursor at current position
 * XXX is this the right way ? It is in the manual ?!?
 */
EXPORT void
vopen(wp)
	ewin_t	*wp;
{
	wp->lastch = '\n';
	vnorm(wp);
	wp->dot -= wp->curnum;
}

/*
 * Insert 'curnum' newlines after the cursor, leave cursor at current position,
 * then set mark at old corsor position.
 * XXX is this the right way ? It is in the manual ?!?
 */
EXPORT void
vsopen(wp)
	ewin_t	*wp;
{
	vopen(wp);
	setmark(wp, wp->dot+wp->curnum);
}

/*
 * Edit vedhelp file
 */
EXPORT void
vhelp(wp)
	ewin_t	*wp;
{
	if (! getcmdchar(wp, "yY", "HELP?(Y/N) "))
		return;
	if (spawncmd(wp, "ved", "-vhelp") != 0) {
		wait_for_confirm(wp);
		vredisp(wp);
	} else {
		vredisp(wp);
		writemsg(wp, "BACK AGAIN");
	}
}
