/* @(#)screen.c	1.42 18/08/27 Copyright 1984-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)screen.c	1.42 18/08/27 Copyright 1984-2018 J. Schilling";
#endif
/*
 *	Screen update functions for VED
 *
 *	Copyright (c) 1984-2018 J. Schilling
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

/*
 * The functions below are responsible for the fast screen update of VED.
 * Main implementation goal was to make the screen uptate fast, ergonomic
 * and free of jerks. The screen update of vi is not aceptable since
 * inserting line by line makes the screen hard to read while scrolling.
 * The screen update of emacs shows you jerking status lines and for that
 * reason is hard to read too.
 *
 * If the article from James Gosling (ACM Sig-Plan Notices 1981 vol 16 p123-129
 * still applies to EMACS, then its screen update method differs from the
 * update method used by VED. Emacs tries to compute the updates for more
 * than one change while VED only looks for one changed spot. However, the
 * visible behaviour on screen looks very similar.
 *
 * Newer versions of 'emacs' try to imitate the basic screen update behaviour
 * of VED by doing block insert lines, but are slower (emacs spends 4 times
 * the CPU time in its equivalent screen update package than VED needs).
 * Take extrame care when doing any modifications on this module.
 * The basic functionality has not been changed since 1986. Although
 * many minor changes (markwrap and some cursor positioning problems)
 * have been done to make it behave more correctly.
 *
 * The basic idea is to use no retained memory to compare the current and
 * the new screen but to try to compute the visual efects 'on the fly'.
 * This allows us to resize the screen to any size without any problems.
 * To be able to compute sizes, a character string table and a corresponding
 * character width table is used. VED may deal with lines that are even longer
 * than the screen if folded and handles printing strings of arbitrary
 * length for each character. To be able to do this, the routine typerest()
 * will type any un-typed rest of a character string if there is the need to
 * add charecters to the end of the screen because of a scroll up or
 * delete operation.
 * Typescn() is the interface function that may be used to do partial
 * screen updates, typescreen() will handle to retype the whole screen
 * unconditionally. Typescn() is only an internal interface for either
 * dispup() or typescreen(). Dispup() is the external high level interface
 * that takes care of doing optimizations like insert char, delete char
 * and similar.
 *
 * Exported functions:
 *
 *	update		- Update the display for the result of a cursor
 *			  movement. This may result in the need for an update
 *			  of the screen content too.
 *	setwindow	- Adjust the current window for optimal cursor position
 *	newwindow	- Adjust the current window for optimal cursor position
 *			  and retype the screen.
 *	getindent	- Return visible indentation of current line,
 *			  used by autoindent functionality.
 *	setpos		- Re-compute the actual cursor position.
 *	setcursor	- Set HW-cursor to 'cursor' position value.
 *	countpos	- Count visible position difference for two byte
 *			  offset positions in buffer.
 *	findcol		- Find the place on current line that is at a specific
 *			  visible column.
 *	dispup		- Update display as a result of an insert or delete
 *			  operation.
 *	realvp		- Return 'real' vertial cursor position.
 *	realhp		- Return 'real' horizontal cursor position.
 *	typescreen	- unconditionally re-type the whole screen or
 *			  parts of it
 *
 * Edit() calls update() after each action. Only functions that insert or
 * delete parts of the visible screen need to call dispup() and - or
 * a setpos() setcursor() sequence.
 *
 * To find out how much of the screen needs to be updated, findmatch() is used.
 */

/*
 * Ein Problem sind Terminals, bei denen b_auto_right_margin true ist.
 * Sie setzen am Ende einer Zeile Den Cursor automatisch auf die naechste Zeile
 * oder scrollen gar, wenn es sich um die letzte Bildschirmposition handelt.
 *
 * Da wird es schwierig, die letzte Bildschirmposition zu beschreiben.
 * Ich sehe z.Zt. nur die Möglichkeit durch Verwendung von insert char oder
 * insert line.
 *
 * Will man mit insert char arbeiten, dann wird zuerst das Letze Zeichen auf der
 * vorletzten Position geschrieben und dann davor das vorletzte Zeichen
 * eingefügt.
 * Vorteil: ruhiges Bild.
 *
 * Wenn man mit insert Line arbeitet, denn geht es z.B. mit folgendem
 * Codefragment, welches aber den Nachteil hat, das das Bild springt und das es
 * nicht funktioniert, wenn das verwendete Terminal wider Erwarten doch micht
 * hochscrollt.
 *
 *	if (b_auto_right_margin && hpos >= wp->llen) {
 *		MOVE_CURSOR(0, 0);
 *		INSERT_LINE(wp, 1);
 *		refreshmsg();
 *	}
 *
 * Da der ved versucht die Terminalabhängigen Dinge möglichst vorsichtig zu
 * verwenden, scheidet daher die letzte Methode aus.
 *
 * N.B.: Der vi ist auf maximale Performance bei 300 Baud optimiert, der
 *	 ved auf maximale Performance bei hohen Datenraten.
 */

#include "ved.h"
#include "movedot.h"
#include "buffer.h"
#include "terminal.h"

/*#define	DEBUG*/
#ifdef	DEBUG
#define	DBG(a)		(cdbg a)
#define	FDBG(a)		(flush(), cdbg a)
#define	SDBG(a)		(cdbg a, sleep(2))
#define	FSDBG(a)	(flush(), cdbg a, sleep(2))
#else
#define	DBG(a)
#define	FDBG(a)
#define	SDBG(a)
#define	FSDBG(a)
#endif

EXPORT	cpos_t	cursor;		/* position of cursor in window hp not mapped*/
EXPORT	cpos_t	cpos;		/* real pos of cursor on screen hp mapped    */

#define	MARKWRAP
/* XXX markwrap ist noch nicht implementiert */

LOCAL	BOOL	nl_ostop;	/* stop output past wrap marker '\\'	    */

extern	Uchar	csize[];	/* The character sizes table		    */
extern	Uchar	*ctab[];	/* The character string table		    */

/*
 * XXX static beseitigen! -> automatic.
 */
LOCAL	headr_t	*passlink = 0;		/* Used by getnext(). Both should be */
LOCAL	int	passoffset = 0;		/* set with findpos() and left alone */

/*
 * Get the next character in buffer.
 *
 * Getnext() needs acess to two variables: rpasslink and rpassoffset.
 * Both should be in registers as a result of a previous call to
 * findpos(pos, &passlink, &passoffset); and then left alone.
 * Getnext() then will increment rpasslink and rpassoffset as needed and
 * return the next character in the buffer on each call.
 */

#define	getnext(wp)	rpasslink->cont[rpassoffset++];		\
									\
	while (rpassoffset >= rpasslink->size) {			\
		rpassoffset -= rpasslink->size;				\
		rpasslink = rpasslink->next;				\
		readybuffer(wp, rpasslink);				\
	}

#define	peeknext()	rpasslink->cont[rpassoffset]

/*
 * On a Motorola 68000 there is no divsll and even on an 68020
 * divu if much faster than divsll.
 * Try to use divu as long as llen * psize is < 65535 (256 * 256).
 * Otherwise define LONG_MOD.
 */

#ifndef	JOS
#if	defined(mc68000) && !defined(LONG_MOD)
#define	modu(a, b)	((unsigned short) \
				(((unsigned short)(a)) % ((unsigned short)(b))))
#else
#define	modu(a, b)	((a) % (b))
#endif	/* mc68000 */
#endif	/* JOS */

#ifndef	modu
#define	modu(a, b)	((a) % (b))
#endif

/*
 * Check if we are done with matching the screen update.
 * This is true if there was a linewrap and we are at the start of the line or
 * if folding the lines would take us to the same virtual horizontal position.
 */
#define	nomatch(hp1, hp2) (linewrap ? (hp1) || (hp2) : \
				modu(hp1, wp->llen) != modu(hp2, wp->llen))

LOCAL	epos_t	lastdot = -1L;

EXPORT	void	update		__PR((ewin_t *wp));
EXPORT	void	setwindow	__PR((ewin_t *wp));
EXPORT	void	newwindow	__PR((ewin_t *wp));
LOCAL	epos_t	getnewwindow	__PR((ewin_t *wp));
LOCAL	BOOL	setcurpos	__PR((ewin_t *wp, epos_t ldot));
LOCAL	BOOL	onscreen	__PR((ewin_t *wp, cpos_t * cp));
EXPORT	int	getindent	__PR((ewin_t *wp));
EXPORT	void	setpos		__PR((ewin_t *wp));
EXPORT	BOOL	setcursor	__PR((ewin_t *wp));
LOCAL	void	mappos		__PR((ewin_t *wp, cpos_t * cp));
EXPORT	epos_t	countpos	__PR((ewin_t *wp, epos_t old, epos_t new,
					cpos_t * cp));
LOCAL	epos_t	getcol		__PR((ewin_t *wp, epos_t begin, int maxcol,
					cpos_t * cp));
EXPORT	epos_t	findcol		__PR((ewin_t *wp, int col, epos_t begin));
LOCAL	epos_t	findmatch	__PR((ewin_t *wp, epos_t begin, cpos_t * a,
					cpos_t * b));
EXPORT	void	dispup		__PR((ewin_t *wp, epos_t old, epos_t new));
LOCAL	void	dispdel		__PR((ewin_t *wp, epos_t old, epos_t new));
LOCAL	void	dispins		__PR((ewin_t *wp, epos_t old, epos_t new));
EXPORT	int	realvp		__PR((ewin_t *wp, cpos_t * cp));
EXPORT	int	realhp		__PR((ewin_t *wp, cpos_t * cp));
LOCAL	void	typescn		__PR((ewin_t *wp, epos_t begin, int col,
					epos_t end));
LOCAL	epos_t	typescx		__PR((ewin_t *wp, epos_t begin, int col,
					epos_t end));
LOCAL	void	typerest	__PR((ewin_t *wp, epos_t begin, cpos_t * cp));
EXPORT	void	typescreen	__PR((ewin_t *wp, epos_t begin, int col,
					epos_t end));
LOCAL	BOOL	outch		__PR((ewin_t *wp, int c));
LOCAL	BOOL	outnl		__PR((ewin_t *wp, BOOL  nlflg));
LOCAL	BOOL	outtab		__PR((ewin_t *wp));
LOCAL	BOOL	siaddchar	__PR((ewin_t *wp, int c));
LOCAL	void	insert_pad	__PR((ewin_t *wp, int size));


/*---------------------------------------------------------------------------
|
| Update window/cursor position to reflect the new position of dot
|
+---------------------------------------------------------------------------*/

EXPORT void
update(wp)
	ewin_t	*wp;
{
	if (wp->dot >= wp->window && setcurpos(wp, lastdot))
		setcursor(wp);
	else
		setwindow(wp);
	lastdot = wp->dot;
}


/*---------------------------------------------------------------------------
|
| Adjust window so that the cursor will be in an optimal position on screen
|
+---------------------------------------------------------------------------*/

EXPORT void
setwindow(wp)
	ewin_t	*wp;
{
	epos_t	save = wp->window;

	DBG(("setwindow"));

	if (save > wp->eof)
		save = wp->eof;
	wp->window = getnewwindow(wp);
	MOVE_CURSOR(wp, 1, 0);
	cursor.vp = 1;
	cursor.hp = 0;
	dispup(wp, save, wp->window);
	setpos(wp);
	setcursor(wp);
}


/*---------------------------------------------------------------------------
|
| Retype the whole screen after adjusting the window with an optimal curpos
|
+---------------------------------------------------------------------------*/

EXPORT void
newwindow(wp)
	ewin_t	*wp;
{
	DBG(("newwindow"));

	wp->window = getnewwindow(wp);
	MOVE_CURSOR_ABS(wp, 1, 0);
	cursor.vp = 1;
	cursor.hp = 0;
	typescreen(wp, wp->window, 0, wp->eof);
	setpos(wp);
	setcursor(wp);
	lastdot = wp->dot;
}

#ifdef OLD

LOCAL epos_t
getnewwindow()
{
	return (revline(dot, optline));
}

#else


/*---------------------------------------------------------------------------
|
| Recompute the start of the window to have the cursor in a optimal position
|
+---------------------------------------------------------------------------*/

LOCAL epos_t
getnewwindow(wp)
	ewin_t	*wp;
{
		cpos_t	c;
	register epos_t	newwin;
	register epos_t	win = wp->dot;
	register int	vpos = 1;
	register int	vopt = wp->optline;
	register int	col;

	if (win > wp->eof) {
		writeerr(wp, "BAD DOT POS (>EOF)");
		wp->dot = wp->eof;
		return (getnewwindow(wp));
	}
	do {
		c.vp = 1;
		c.hp = 0;
		newwin = revline(wp, win, (ecnt_t)2); /* eine Zeile zurueck */
		countpos(wp, newwin, win, &c);
		win = newwin;
		vpos += realvp(wp, &c) - 1;
	} while (vpos < vopt && newwin > 0);
	if (vpos >= wp->psize) {
		/*
		 * Should not happen .... but paranoia.
		 */
		wp->pmargin = 0;
		col = wp->llen/2;
		newwin = wp->dot - col - 1;
		while (findcol(wp, col, ++newwin) < wp->dot);
	}
	findwpos(wp, newwin);
	return (newwin);
}
#endif


/*---------------------------------------------------------------------------
|
| Compute the position where the cursor should be depending on 'dot' pos.
|
+---------------------------------------------------------------------------*/

LOCAL BOOL
setcurpos(wp, ldot)
	ewin_t	*wp;
	epos_t	ldot;
{
	DBG(("setcurpos dot: %lld ldot: %lld", (Llong)wp->dot, (Llong)ldot));

	if (ldot >= 0 && wp->dot >= ldot) {

		countpos(wp, ldot, wp->dot, &cursor);
	} else {
		setpos(wp);
	}
	return (onscreen(wp, &cursor));
}


/*---------------------------------------------------------------------------
|
| Check if 'cp' will be visible on the current screen
|
+---------------------------------------------------------------------------*/

LOCAL BOOL
onscreen(wp, cp)
	ewin_t	*wp;
	cpos_t	*cp;
{
	register int	v = realvp(wp, cp);

	return (v <= wp->psize-wp->pmargin &&
		(v != wp->psize-wp->pmargin || realhp(wp, cp) < wp->llen) &&
					(v > wp->pmargin || wp->window == 0));
}


/*---------------------------------------------------------------------------
|
| Return the (visible) indentation of the current line
|
+---------------------------------------------------------------------------*/

EXPORT int
getindent(wp)
	ewin_t	*wp;
{
	cpos_t	c;
	BOOL		save;
	epos_t		lbeg;
	epos_t		textbeg;
	extern Uchar	notwhitespace[];

	c.vp = 1;
	c.hp = 0;
	lbeg = revline(wp, wp->dot, (ecnt_t)1);	/* an den Anfang der Zeile */
	save = wp->magic;
	wp->magic = TRUE;
	textbeg = search(wp, lbeg, notwhitespace, strlen(C notwhitespace), 0)
			- 1;
	wp->magic = save;
	if (textbeg > wp->dot)
		textbeg = wp->dot;
/*	writeerr(wp, "%d %d %d", lbeg, textbeg, wp->dot);*/
	countpos(wp, lbeg, textbeg, &c);
	return (c.hp);
}


/*---------------------------------------------------------------------------
|
| Compute the position where the cursor should be, start at top of 'window'
|
+---------------------------------------------------------------------------*/

EXPORT void
setpos(wp)
	ewin_t	*wp;
{
	cursor.vp = 1;
	cursor.hp = 0;
	countpos(wp, wp->window, wp->dot, &cursor);
}


/*---------------------------------------------------------------------------
|
| Set the cursor to the position previously set up in struct curpos by setpos()
|
+---------------------------------------------------------------------------*/

EXPORT BOOL
setcursor(wp)
	ewin_t	*wp;
{
	int	v;

	DBG(("setcursor caller: 0x%lX P: %d.%d C: %d.%d",
		getcaller(), cpos.vp, cpos.hp, cursor.vp, cursor.hp));

	if ((v = realvp(wp, &cursor)) <= wp->psize) {
		MOVE_CURSOR(wp, v, realhp(wp, &cursor));

		DBG(("setcursor P: %d.%d C: %d.%d",
			cpos.vp, cpos.hp, cursor.vp, cursor.hp));

		return (TRUE);
	} else {
		return (FALSE);
	}
}


/*---------------------------------------------------------------------------
|
| Map cp->vp/cp->hp position description into description of the form vp/0
|
+---------------------------------------------------------------------------*/

LOCAL void
mappos(wp, cp)
		ewin_t	*wp;
	register cpos_t	*cp;
{
	cp->vp = realvp(wp, cp) + 1;
	cp->hp = 0;
}


/*---------------------------------------------------------------------------
|
| Update 'cp' to the cursor position when moving from old to new.
| Return position in file that is actually taken.
|
+---------------------------------------------------------------------------*/

EXPORT epos_t
countpos(wp, old, new, cp)
		ewin_t	*wp;
		epos_t	old;
		epos_t	new;
	register cpos_t	*cp;
{
	register headr_t *rpasslink;	/* state for getnext - these should */
	register int	rpassoffset;	/* set by any caller and left alone */
	register Uchar	c;
	register Uchar	*rcsize;
	register int	rllen;
	register int	rpsize;
	register epos_t	cnt;

	if (old >= new)
		return (old);
	rcsize = csize;
	rllen = wp->llen;
	rpsize = wp->psize;
	findpos(wp, old, &passlink, &passoffset);
	rpasslink = passlink;
	rpassoffset = passoffset;

	cnt = new - old;
	while (cnt > 0) {
		c = getnext(wp);
		cnt--;
		if (c == '\n') {
			mappos(wp, cp);
			if (cp->vp > rpsize)
				break;
		} else if (c == TAB) {
			cp->hp = (cp->hp / wp->tabstop) * wp->tabstop +
					wp->tabstop;
		} else {
			cp->hp += rcsize[c];
		}
		if (cp->hp >= rllen && realvp(wp, cp) > rpsize)
			break;
	}
	return (new - cnt);
}


/*---------------------------------------------------------------------------
|
| Get 'cp' for maximun column in this line.
| Update 'cp' to the cursor position when moving from old to this column.
| Return position in file that is actually taken.
|
+---------------------------------------------------------------------------*/

LOCAL epos_t
getcol(wp, begin, maxcol, cp)
		ewin_t	*wp;
		epos_t	begin;
	register int	maxcol;
	register cpos_t	*cp;
{
	register headr_t *rpasslink;	/* state for getnext - these should */
	register int	rpassoffset;	/* set by any caller and left alone */
	register Uchar	c;
	register Uchar	*rcsize;
	register int	ocol;
	register int	col;
	register epos_t	cnt;

	if (begin >= wp->eof)
		return (begin);
	rcsize = csize;
	findpos(wp, begin, &passlink, &passoffset);
	rpasslink = passlink;
	rpassoffset = passoffset;

	cnt = wp->eof - begin;
	col = realhp(wp, cp);
	while (cnt > 0 && col <= maxcol) {
		c = getnext(wp);
		cnt--;
		if (c == '\n') {
			mappos(wp, cp);
			col = realhp(wp, cp);
			break;
		} else if (c == TAB) {
			ocol = cp->hp;
			cp->hp = (cp->hp / wp->tabstop) * wp->tabstop +
					wp->tabstop;
			col += cp->hp - ocol;
		} else {
			cp->hp += rcsize[c];
			col += rcsize[c];
		}
	}
	return (wp->eof - cnt);
}


/*---------------------------------------------------------------------------
|
| Find the file offset that belongs to a specific visible screen column.
| Start position needs to be at the beginning of a line.
|
+---------------------------------------------------------------------------*/

EXPORT epos_t
findcol(wp, col, begin)
		ewin_t	*wp;
	register int	col;
	register epos_t	begin;
{
	cpos_t a;

	a.hp = 0;
	a.vp = 1;

	while (begin < wp->eof && a.hp < col) {
		countpos(wp, begin, begin+1, &a);
		/*
		 * If this line is too short, give up.
		 */
		if (a.hp == 0)
			break;
		begin++;
	}
	return (begin);
}

LOCAL	int	tabcnt;		/* # of tabs found by last findmatch call */
LOCAL	BOOL	linewrap;	/* The changes caused a diffrent vpos    */

/*---------------------------------------------------------------------------
|
| Find the minimum number of characters needed to print for updating the
| screen to a correct new state.
|
+---------------------------------------------------------------------------*/

LOCAL epos_t
findmatch(wp, begin, a, b)
		ewin_t	*wp;
		epos_t	begin;
	register cpos_t	*a;
	register cpos_t	*b;
{
	register headr_t *rpasslink;	/* state for getnext - these should */
	register int	rpassoffset;	/* set by any caller and left alone */
	register Uchar	c;
	register Uchar	*rcsize;
	register Uchar	size;
	register epos_t	cnt;

	tabcnt = 0;
	linewrap = a->vp != b->vp || a->hp/wp->llen != b->hp/wp->llen;
	if (begin >= wp->eof)
		return (begin);
	rcsize = csize;
	findpos(wp, begin, &passlink, &passoffset);
	rpasslink = passlink;
	rpassoffset = passoffset;

	cnt = wp->eof - begin;
	while (nomatch(a->hp, b->hp) && cnt > 0) {
		c = getnext(wp);
		cnt--;
		if (c == '\n') {
			mappos(wp, a);
			mappos(wp, b);
			return (wp->eof - cnt);
		}
		if (c == TAB) {
			a->hp = (a->hp / wp->tabstop) * wp->tabstop +
					wp->tabstop;
			b->hp = (b->hp / wp->tabstop) * wp->tabstop +
					wp->tabstop;
			tabcnt++;
		} else {
			a->hp += size = rcsize[c];
			b->hp += size;
		}
	}
	return (wp->eof - cnt);
}


/*---------------------------------------------------------------------------
|
| Check if we are ready with matching the screen update.
| This is true if there was a linewrap and we are at the start of the line or
| if folding the lines would take us to the same virtual horizontal position.
|
+---------------------------------------------------------------------------*/

#ifndef	nomatch
nomatch(hp1, hp2)
{
	if (linewrap)
		return (hp1 || hp2);
	else
		return (modu(hp1, wp->llen) != modu(hp2, wp->llen));
}
#endif

LOCAL	int	gotshorter;	/* There were visible deletions on screen */

/*---------------------------------------------------------------------------
|
| Update the display to reflect the effects of a insert or delete operation.
| 'old' is the file offset where the cursor currently is located.
| 'new' is the file offset where the cursor should be.
|
+---------------------------------------------------------------------------*/
EXPORT void
dispup(wp, old, new)
	ewin_t	*wp;
	epos_t	old;
	epos_t	new;
{
	FDBG(("  dispup (%lld, %lld) P: %d.%d %s", (Llong)old, (Llong)new,
					cpos.vp, cpos.hp,
					(new > old)? "Delete":"Insert"));

	if ((gotshorter = (new > old)) != 0) {
		dispdel(wp, old, new);
	} else {
		dispins(wp, old, new);
	}
}


/*---------------------------------------------------------------------------
|
| Update the display to reflect the effects of a delete operation.
| 'old' is where the deleted text starts now (before deleting)
| 'new' is where the remaining characters have been before doing the deletion.
|
+---------------------------------------------------------------------------*/

LOCAL void
dispdel(wp, old, new)
	ewin_t	*wp;
	epos_t	old;
	epos_t	new;
{
	epos_t	save = old;
	epos_t	skip;
	int	size;
	int	bottomv;
	int	bottomh;
	int	col = cursor.hp;
	cpos_t	o;
	cpos_t	n;

	/*
	 * First iniatilize to current cursor position.
	 * Later, 'n' is updated to where the cursor should be,
	 * 'o' is updated to where the appropriate characters currently are.
	 */
	o.vp = n.vp = cursor.vp;
	o.hp = n.hp = cursor.hp;

	/*
	 * Compute the visible size of deleted characters and the position
	 * on screen where typing these caracters would take us.
	 */
	countpos(wp, old, new, &o);
	size = o.hp - n.hp;

	/*
	 * Check how many characters we would have to type until the
	 * screen remains the same as before deleting the characters.
	 */
	old = findmatch(wp, new, &n, &o);

	linewrap = realvp(wp, &o) != realvp(wp, &n);

	/*
	 * If we did not find any tab and we only have to retype the rest of
	 * the current line use delete char.
	 * If there is no charater remaining after the current position
	 * don't use delete char.
	 */
	if (old != 0 && o.hp == 0 && o.vp == realvp(wp, &cursor) + 1)
		if (tabcnt == 0 && f_del_char)
			if (size > 0 && size < (old-new)/2 && save+1 < old) {
				DELETE_CHAR(wp, size);
				old = new;

				FSDBG(("DCdispdel(%lld, %lld) P: %d.%d",
					(Llong)old, (Llong)new,
					cpos.vp, cpos.hp));
			}

	/*
	 * Limit the number if deleted lines to 2/3 of the screen and
	 * require that at least one old line remains on screen.
	 */
	if ((size = realvp(wp, &o) - realvp(wp, &n)) > 0 && new < wp->eof) {
		if (f_del_line && size>>1 <= wp->psize/3+1 &&
		    size+cpos.vp < wp->psize-1 &&
		    cpos.vp < wp->psize) {
			/*
			 * First redraw to end of current screen line.
			 */
			n.vp = cursor.vp;
			n.hp = cursor.hp;
			skip = getcol(wp, new, wp->llen, &n);
			if (skip < wp->eof) {
				nl_ostop = TRUE;
				typescn(wp, new, col, skip);
				nl_ostop = FALSE;

				FSDBG(("TSdispdel(%lld, %lld) P: %d.%d",
					(Llong)old, (Llong)new,
					cpos.vp, cpos.hp));

				/* find end of screen */
				new = countpos(wp, old, wp->eof, &o);
				col = o.hp;
				DELETE_LINE(wp, size);

				FSDBG(("DLdispdel(%lld, %lld) P: %d.%d",
					(Llong)old, (Llong)new,
					cpos.vp, cpos.hp));

				if (new == wp->eof) {
					/*
					 * Das Ende der Datei ist bereits
					 * sichtbar, Bildschirmupdate beendet.
					 */
					goto out;
					/* need goto to print dbg */
				}

				FSDBG(("BVdispdel(%lld, %lld) O: %d.%d",
						(Llong)old, (Llong)new,
						o.vp, o.hp));
				/*
				 * Berechnung der realen Cursorposition, ab
				 * der der Bildchirm beschrieben werden muß.
				 */
				bottomv = realvp(wp, &o);
				bottomv = min(bottomv, wp->psize+1);
				bottomh = bottomv > wp->psize ? 0 :
							realhp(wp, &o);

				if (b_auto_right_margin && !f_ins_char) {
					/*
					 * In diesem Fall ist das letzte
					 * Zeichen auf dem Bildschirm nicht
					 * geschrieben worden.
					 */
					if (bottomh == 0) {
						bottomv--;
						bottomh = wp->llen;
					} else {
						/*
						 * Das sollte nie passieren!
						 */
						cdbg("bottomh: %d",
							bottomh);
					}
				}
				MOVE_CURSOR(wp, bottomv-size, bottomh);

				FSDBG(("MCdispdel(%lld, %lld) P: %d.%d",
					(Llong)old, (Llong)new,
					cpos.vp, cpos.hp));

				typerest(wp, new, &o);

				FSDBG(("TRdispdel(%lld, %lld) P: %d.%d",
					(Llong)old, (Llong)new,
					cpos.vp, cpos.hp));
			}
		}
		/*
		 * There is no delele-line feature or we don't want
		 * to use it, redraw to end.
		 */
		old = wp->eof;
	}
	typescn(wp, new, col, old);
out:
	FSDBG(("TSdispdel(%lld, %lld) P: %d.%d",
			(Llong)old, (Llong)new,
			cpos.vp, cpos.hp));
}


/*---------------------------------------------------------------------------
|
| Update the display to reflect the effects of an insert operation.
| 'new' is where the new (inserted characters) start
| 'old' is where the old characters will be after doing the insertion.
|
+---------------------------------------------------------------------------*/

LOCAL void
dispins(wp, old, new)
	ewin_t	*wp;
	epos_t	old;
	epos_t	new;
{
	epos_t	save = old;
	int	size;
	int	col = cursor.hp;
	cpos_t	o;
	cpos_t	n;

	/*
	 * First iniatilize to current cursor position.
	 * Later, 'n' is updated to where the cursor should be,
	 * 'o' is updated to where the appropriate characters currently are.
	 */
	o.vp = n.vp = cursor.vp;
	o.hp = n.hp = cursor.hp;

	/*
	 * Compute the visible size of inserted characters and the position
	 * on screen where typing the caracters would take us.
	 */
	countpos(wp, new, old, &n);
	size = n.hp - o.hp;

	/*
	 * Check how many characters we have to type until the screen
	 * remains the same as before inserting the characters.
	 */
	old = findmatch(wp, old, &n, &o);

	linewrap = realvp(wp, &o) != realvp(wp, &n);

	/*
	 * If we did not find any tab and we only have to retype the rest of
	 * the current line use insert char.
	 * If there is no charater remaining after the current position
	 * don't use insert char.
	 */
	if (old != wp->eof && n.hp == 0 && n.vp == realvp(wp, &cursor) + 1) {
		if (tabcnt == 0 && f_ins_char) {
			if (size > 0 && size < (old-new)/3 && save+1 < old) {
				/* XXX Besser direkt Inhalt schreiben */
				insert_pad(wp, size);

				FSDBG(("ICdispins(%lld, %lld) P: %d.%d",
					(Llong)old, (Llong)new,
					cpos.vp, cpos.hp));

				MOVE_CURSOR(wp, cpos.vp, cpos.hp - size);
				old = save;

				FSDBG(("MCdispins(%lld, %lld) P: %d.%d",
					(Llong)old, (Llong)new,
					cpos.vp, cpos.hp));
			}
		}
	} else if (o.hp) {
		mappos(wp, &o);
	}

	/*
	 * Limit the number if inserted lines to half the screen and
	 * require that at least one old line remains on screen.
	 */
	if ((size = (realvp(wp, &n)-realvp(wp, &o)))*2 > wp->psize ||
			size+cpos.vp >= wp->psize-1 ||
			old == wp->eof) {
		typescn(wp, new, col, wp->eof);		/* Retype rest */

		FSDBG(("TS1dispins(%lld, %lld) P: %d.%d",
				(Llong)old, (Llong)new,
				cpos.vp, cpos.hp));
	} else {
		/*
		 * We decided that is is worth to insert lines.
		 */
		if (size > 0) {
			if (f_ins_line) {
				if (col > 0)
					MOVE_CURSOR(wp, cpos.vp + 1, 0);
				INSERT_LINE(wp, size);

				FSDBG(("ILdispins(%lld, %lld) P: %d.%d",
					(Llong)old, (Llong)new,
					cpos.vp, cpos.hp));
				if (col > 0)
					setcursor(wp);
			} else {
				old = wp->eof;
			}
		}
		typescn(wp, new, col, old);

		FSDBG(("TS2dispins(%lld, %lld) P: %d.%d",
			(Llong)old, (Llong)new,
			cpos.vp, cpos.hp));
	}
}


/*---------------------------------------------------------------------------
|
| Return the 'real' vertical position of 'cp'.
| When computing cursor position, the vertical position is only updated to
| the correct visible value if we at the start of a line. This computes the
| visible vertical position as an effect of possible folding of long lines.
| Take care not to increment if hpos is 0.
|
+---------------------------------------------------------------------------*/

EXPORT int
realvp(wp, cp)
		ewin_t	*wp;
	register cpos_t	*cp;
{
#ifdef	MARKWRAP
	return (cp->hp >= (wp->llen+wp->markwrap) ?
			cp->vp + (cp->hp - wp->markwrap) / wp->llen : cp->vp);
#else
	return (cp->hp > wp->llen ? cp->vp + (cp->hp - 1) / wp->llen : cp->vp);
#endif
}


/*---------------------------------------------------------------------------
|
| Return the 'real' horizontal position of 'cp'.
| When computing cursor position, the vertical position is only updated to
| the correct visible value if we at the start of a line. This computes the
| visible horizontal position as an effect of possible folding of long lines.
| Take care not to increment if hpos is 0.
|
+---------------------------------------------------------------------------*/

EXPORT int
realhp(wp, cp)
	ewin_t	*wp;
	cpos_t	*cp;
{
	register int	h = cp->hp;

#ifdef	MARKWRAP
	return (h >= (wp->llen+wp->markwrap) ?
			(h - wp->markwrap) % wp->llen + wp->markwrap : h);
#else
	return (h > wp->llen ? (h - 1) % wp->llen + 1 : h);
#endif
}


/*---------------------------------------------------------------------------
|
| Type characters from 'x' to 'y' starting on column 'col'.
| Stop if we reached 'eof' or the end of the screen.
| Use typescx() to do the work, then set 'ewindow' to the last character
| position on screen.
|
+---------------------------------------------------------------------------*/

LOCAL void
typescn(wp, begin, col, end)
	ewin_t	*wp;
	epos_t	begin;
	int	col;
	epos_t	end;
{
	epos_t	lpos;

	if ((lpos = typescx(wp, begin, col, end)) >= 0)
		wp->ewindow = lpos;
	if (wp->ewindow < wp->window)	/* XXX ??? */
		wp->ewindow = wp->eof;
}

LOCAL	int	lcol;			/* Remembered last typed column. */

/*---------------------------------------------------------------------------
|
| Type characters from 'x' to 'y' starting on column 'col'.
| Stop if we reached 'eof' or the end of the screen.
| Return the offset of the last character typed if 'y' would not fit on screen
| or -1 if the whole number of characters could be typed.
| Should only be called from typescn().
|
+---------------------------------------------------------------------------*/

LOCAL epos_t
typescx(wp, begin, col, end)
		ewin_t	*wp;
	register epos_t	begin;
		int	col;
		epos_t	end;
{
	register headr_t *rpasslink;	/* state for getnext - these should */
	register int	rpassoffset;	/* set by any caller and left alone */
	register Uchar	c;
	register Uchar	*s;
	register Uchar	**rctab;
	register epos_t	lposition;	/* last pos in buffer to be typed */

	rctab = ctab;
	findpos(wp, begin, &passlink, &passoffset);
	rpasslink = passlink;
	rpassoffset = passoffset;

	lcol = col;
	lposition = min(end, wp->eof);

	while (begin < lposition) {
		if (wp->markvalid && begin == wp->mark)
			onmark();
		c = getnext(wp);
		begin++;
		if (c == '\n') {
			if (!outnl(wp, begin < lposition || gotshorter ||
						linewrap || wp->visible))
				return (begin);
		} else if (c == '\r' && wp->dosmode && peeknext() == '\n') {
			/*EMPTY*/
			;
		} else if (c == TAB) {
			if (!outtab(wp))
				return (begin);
		} else {
			s = rctab[c];
			while (*s)
				if (!outch(wp, *s++))
					return (begin);
		}
		if (markon)
			offmark();
	}

	/*
	 * If the last position to be typed is 'eof' take care of the EOF
	 * marker.
	 */
	if (lposition == wp->eof) {
		if (wp->mark == wp->eof && wp->markvalid) {
			onmark();
			addchar((Uchar) (wp->visible ? '>' : ' '));
			offmark();
		} else {
			addchar((Uchar) (wp->visible ? '>' : ' '));
		}

		/*
		 * If the file got shorter, erase the rest of the screen if
		 * the change was visible on more than line orerase the rest
		 * of the line if we only had to type one line.
		 */
		if (gotshorter) {
			if (linewrap)
				CLEAR_TO_EOF_SCREEN(wp);
			else
				CLEAR_TO_EOF_LINE(wp);
		}
	}
	return (-1);
}


/*
 * Gibt den Rest eines Buchstabens aus,
 * wenn durch Hochscrollen der hintere Teil sichtbar wird.
 * Das passiert, wenn ein Buchstabe vorher nicht komplett auf dem
 * Bildschirm sichtbar war.
 *
 * Pos ist die Cursorposition hinter dem wickelnden char.
 * Wenn pos nicht hinter die letzte Zeile zeigt,
 * oder der Rest grÖßer als der dort stehende Buchstabe ist,
 * wird nichts ausgegeben.
 */
LOCAL void
typerest(wp, begin, cp)
		ewin_t	*wp;
	register epos_t	begin;
	register cpos_t	*cp;
{
	register headr_t *rpasslink;	/* state for getnext - these should */
	register int	rpassoffset;	/* set by any caller and left alone */
	register Uchar	c;		/* wrapping character		    */
	register Uchar	*s;		/* string to output for 'c'	    */
	register int	rest;		/* number of untyped chars in 'c'   */
		int	p;

	/*
	 * Wenn die Zeile wickelt, dann ist realvp(cp) mindestens auf psize+1
	 */
	if (realvp(wp, cp) - wp->psize <= 0)
		return;

	p = wp->psize + 1 - cp->vp;

	rest = cp->hp - p * wp->llen;

	DBG(("typerest: cp: %d.%d psize: %d p: %d rest: %d",
			cp->vp, cp->hp, wp->psize, p, rest));

	if (rest <= 0)
		return;

	lcol = cp->hp - rest;
	findpos(wp, --begin, &passlink, &passoffset);
	rpasslink = passlink;
	rpassoffset = passoffset;

	if (wp->markvalid && begin == wp->mark)
		onmark();
	c = getnext(wp);
	if (c == TAB) {
		if (!outtab(wp))
			return;
	} else {
		s = ctab[c];
		p = csize[c];
		if (rest > p)
			goto out;
		s += p - rest;
		while (*s)
			if (!outch(wp, *s++))
				return;
	}
out:
	if (markon)
		offmark();
}


/*---------------------------------------------------------------------------
|
| The external interface to (re-)type the whole screen or parts of it.
| Typescreen does no optimization like dispup().
| The cursor must be set to the position where typing should start.
| It first sets some variables to force typescn() to do no optimization.
|
+---------------------------------------------------------------------------*/

EXPORT void
typescreen(wp, begin, col, end)
		ewin_t	*wp;
	register epos_t	begin;
		int	col;
		epos_t	end;
{
	gotshorter	= TRUE;
	linewrap	= FALSE;
	typescn(wp, begin, col, end);
}

/*
 * lastchar & lastmark sind ein Versuch den letzten Buchstaben auf dem Schirm
 * zu merken, damit man ihn mit insert char zum Beschreiben der letzten
 * Bildschirmposition verwenden kann.
 * Das funktioniert aber nur, wenn die letzten Buchstaben auf dem Bildschirm
 * nacheinander ausgegeben werden.
 */
LOCAL	Uchar	lastchar;
LOCAL	int	lastmark;

/*---------------------------------------------------------------------------
|
| Output one character to the screen, take care of the screens's linelength
| and pagesize. Mark a wrapping line with a backslash if wanted.
| Return FALSE if the character was the last character that fit on screen.
|
+---------------------------------------------------------------------------*/

LOCAL BOOL
outch(wp, c)
	ewin_t	*wp;
	Uchar	c;
{
/*cdbg("llen: %d xxx: %d\n", wp->llen, (wp->llen+markwrap-1));*/
#ifdef	MARKWRAP
	if (cpos.hp >= (wp->llen+wp->markwrap-1)) {
		Uchar lastc = wp->markwrap ? '\\' : c;
#else
	if (cpos.hp >= wp->llen) {
#endif

		if (cpos.vp >= wp->psize) {
			if (!b_auto_right_margin) {

#ifdef	MARKWRAP
				addchar(lastc);
#else
				addchar((Uchar) '\\');
#endif
				/*
				 * nun ist cpos.hp > llen
				 */
			} else {
#ifdef	MARKWRAP
				siaddchar(wp, lastc);
#else
				siaddchar(wp, (Uchar) '\\');
#endif
			}
			if (markon)
				offmark();
			return (FALSE);
		}
/*
 * XXX hier ist der Fehler, daß der letzte Buchstabe nochmal auf der neuen
 * XXX Zeile ausgegeben wird.
 */
#ifdef	MARKWRAP
		addchar(lastc);
#else
		addchar((Uchar) '\\');
#endif
		/*
		 * auch hier ist cpos.hp > llen
		 */
		if (b_auto_right_margin) {
			/*
			 * Flush mark buffer, then move cursor.
			 */
			if (markon) {
				offmark();
				onmark();
			}
			MOVE_CURSOR_ABS(wp, cpos.vp, cpos.hp);
		}
		addchar((Uchar) '\n');
		if (nl_ostop)
			return (TRUE);
#ifdef	MARKWRAP
		if (!wp->markwrap)
			goto out;
#endif
	}
	addchar(c);
lastchar = c;
lastmark = markon;
out:
	lcol++;
	return (TRUE);
}


/*---------------------------------------------------------------------------
|
| Output a NL (^J) (depending on current mode [visible])
|
+---------------------------------------------------------------------------*/

LOCAL BOOL
outnl(wp, nlflg)
	ewin_t	*wp;
	BOOL	nlflg;
{
	if (!b_auto_right_margin || cpos.vp < wp->psize || cpos.hp < wp->llen)
		addchar((Uchar) (wp->visible ? '$' : ' '));
	else
		siaddchar(wp, (Uchar) (wp->visible ? '$' : ' '));

	if (b_auto_right_margin && cpos.hp >= wp->llen) {
		MOVE_CURSOR_ABS(wp, cpos.vp, cpos.hp);
	}
	if (nlflg && cpos.hp <= wp->llen)
		CLEAR_TO_EOF_LINE(wp);
	if (cpos.vp >= wp->psize) {
		if (markon)
			offmark();
		return (FALSE);
	}
	if (nlflg)
		addchar((Uchar) '\n');
	lcol = 0;
	return (TRUE);
}


/*---------------------------------------------------------------------------
|
| Output a TAB (^I) in expanded form (depending on current mode)
|
+---------------------------------------------------------------------------*/

LOCAL BOOL
outtab(wp)
	ewin_t	*wp;
{
	register int	rtabstop = wp->tabstop;

	do {
		if (!outch(wp, (Uchar) (wp->visible ?
					(modu(lcol+1, rtabstop)?'.' : ':') :
					' ')))
			return (FALSE);
	} while (modu(lcol, rtabstop));
	return (TRUE);
}


/*---------------------------------------------------------------------------
|
| Simulate addchar() at last screen position by using insert char.
|
+---------------------------------------------------------------------------*/

LOCAL BOOL
siaddchar(wp, c)
	ewin_t	*wp;
	Uchar	c;
{
	char	str[2];
	int	smark = markon;

	if (!f_ins_char)
		return (FALSE);

	if (markon)
		offmark();
	CURSOR_LEFT(wp, 1);
	if (smark)
		onmark();

	addchar(c);

	if (markon)
		offmark();

	str[0] = lastchar;
	str[1] = '\0';
	CURSOR_LEFT(wp, 1);
	INSERT_CHAR(wp, str);

	if (lastmark) {
		CURSOR_LEFT(wp, 1);
		onmark();
		addchar(lastchar);
		offmark();
	}
	if (smark)
		onmark();
	return (TRUE);
}


/*---------------------------------------------------------------------------
|
| Insert a string of 'size' spaces
|
+---------------------------------------------------------------------------*/

LOCAL void
insert_pad(wp, size)
	ewin_t	*wp;
	int	size;
{
		char	padding[256];
	register int	idx;
	register int	amt = size;

	while (size > 0) {
		if (amt >= sizeof (padding))
			amt = sizeof (padding) - 1;

		for (idx = 0; idx < amt; )
			padding[idx++] = ' ';
		padding[idx] = '\0';
		INSERT_CHAR(wp, padding);
		size -= amt;
	}
}
