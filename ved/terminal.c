/* @(#)terminal.c	1.45 19/06/25 Copyright 1984-2019 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)terminal.c	1.45 19/06/25 Copyright 1984-2019 J. Schilling";
#endif
/*
 *	Upper layer support routines for TERMCAP
 *
 *	Copyright (c) 1984-2019 J. Schilling
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
 * This package provides external callable functions for the various
 * TERMCAP functions imported from ttycmds.c.
 *
 * EXPORTS:
 *		t_start()	- init TERMCAP package
 *		t_begin()	- init terminal
 *		t_done()	- restore terminal state
 *		other		- depending on f_* flags
 *
 * Various f_* flags are provided to tell our callers whether a function is
 * available. These flags are used by the screen update routines to select
 * the proper functions.
 *
 * This package implements many workarounds that implement missing
 * functionality using other supported features from the terminal.
 * A terminal can be supported if absolute cursor movement or relative
 * cursor movement and cursor home are possible.
 * All other funtionality can be emulated.
 */
#include "ved.h"
#include "ttys.h"
#include "terminal.h" /* XXX */
#include <schily/fcntl.h>
#include <schily/ioctl.h>	/* Need to be before termios.h (BSD botch) */
#include <schily/termios.h>
#include <schily/signal.h>

/*
 * The following variables are used to tell our users
 * whether a specific function is supported or not.
 */
char f_home		= 0;	/* Function Cursor Home			*/
char f_era_screen	= 1;	/* Function clear to end of screen	*/
char f_era_line		= 1;	/* Function clear to end of line	*/
char f_ins_line		= 0;	/* Function insert line			*/
char f_del_line		= 0;	/* Function lelete line			*/
char f_ins_char		= 0;	/* Function insert character		*/
char f_del_char		= 0;	/* Function delete character		*/
char f_alternate_video	= 0;	/* Function alternate video		*/
char f_move_cursor	= 0;	/* Function move cursor			*/
char f_clear_screen	= 0;	/* Function clear screen & home cursor	*/
char f_left		= 0;	/* Function cursor left			*/
char f_right		= 0;	/* Function cursor right		*/
char f_up		= 0;	/* Function cursor up			*/
char f_down		= 0;	/* Function cursor down			*/
char f_set_scroll_region = 0;	/* Function set scrolling region	*/
char f_scr_up		= 0;	/* Function scroll up			*/
char f_scr_down		= 0;	/* Function scroll down			*/

#ifdef SIGWINCH
LOCAL	void	winch		__PR((int signo));
#endif
EXPORT	Uchar	*t_start	__PR((ewin_t *wp));
EXPORT	void	t_begin		__PR((void));
EXPORT	void	t_done		__PR((void));
EXPORT	void	t_error		__PR((ewin_t *wp, char *s));
EXPORT	void	t_home		__PR((ewin_t *wp));
EXPORT	void	t_left		__PR((ewin_t *wp, int n));
EXPORT	void	t_right		__PR((ewin_t *wp, int n));
EXPORT	void	t_up		__PR((ewin_t *wp, int n));
EXPORT	void	t_down		__PR((ewin_t *wp, int n));
EXPORT	void	t_move		__PR((ewin_t *wp, int row, int col));
EXPORT	void	t_move_abs	__PR((ewin_t *wp, int row, int col));
EXPORT	void	t_clear		__PR((ewin_t *wp));
EXPORT	void	t_cleos		__PR((ewin_t *wp));
EXPORT	void	t_cleol		__PR((ewin_t *wp));
EXPORT	void	t__cleol	__PR((ewin_t *wp, int do_move));
EXPORT	void	t_insch		__PR((ewin_t *wp, char *str));
EXPORT	void	t_insln		__PR((ewin_t *wp, int n));
EXPORT	void	t_delch		__PR((ewin_t *wp, int n));
EXPORT	void	t_delln		__PR((ewin_t *wp, int n));
EXPORT	void	t_alt		__PR((char *str));
EXPORT	void	t_setscroll	__PR((ewin_t *wp, int beg, int end));
EXPORT	void	t_scrup		__PR((ewin_t *wp, int n));
EXPORT	void	t_scrdown	__PR((ewin_t *wp, int n));

/*
 * Support for run-time window size changes.
 */
#ifdef SIGWINCH
LOCAL void
winch(signo)
	int	signo;
{
#if	defined(TIOCGWINSZ) || defined(TIOCGSIZE)
		extern	ewin_t	rootwin;
			int	tty = -1;
			int	opsize = rootwin.psize;
			int	ollen  = rootwin.llen;
	register	int	lines = 0;
	register	int	cols = 0;
#ifdef	TIOCGWINSZ
	struct		winsize ws;
#else
	struct		ttysize	ts;
#endif

	if (signo != 0)
		signal(signo, winch);

#ifdef	HAVE__DEV_TTY
	tty = open("/dev/tty", 0);
#endif
	if (tty == -1)
		tty = fileno(stderr);

#ifdef	TIOCGWINSZ
	if (ioctl(tty, TIOCGWINSZ, (char *)&ws) >= 0) {
		lines = ws.ws_row;
		cols = ws.ws_col;
	}
#else
	if (ioctl(tty, TIOCGSIZE, (char *)&ts) >= 0) {
		lines = ts.ts_lines;
		cols = ts.ts_cols;
	}
#endif
#ifdef	HAVE__DEV_TTY
	if (tty >= 0 && tty != fileno(stderr))
		close(tty);
#endif

	if (lines == 0 || cols == 0 || lines > 999 || cols > 999)
		return;

#if	defined(__CYGWIN32__) || defined(__CYGWIN__)
	cols--;		/* Either a ioctl() or a window bug */
#endif
	rootwin.psize = lines - 1;
	rootwin.llen = cols - 1;
#ifdef	AUTO_MARGIN_BOTCH
	if (b_auto_right_margin) /* XXX */
		rootwin.llen--;
#endif

	if ((signo == SIGWINCH) && (rootwin.psize != opsize || rootwin.llen != ollen)) {
		if (rootwin.optline == opsize/2 || rootwin.optline > rootwin.psize)
			rootwin.optline = rootwin.psize/2;
		{
		extern ewin_t rootwin;	/* XXX -> (ewin_t *)0 ??? */
		initmsgsize(&rootwin);
		writenum(&rootwin, rootwin.curnum);
		vredisp(&rootwin);	/* XXX -> (ewin_t *)0 ??? */
		update(&rootwin);	/* XXX -> (ewin_t *)0 ??? */
		}
		flush();
	}
#endif	/* defined(TIOCGWINSZ) || defined(TIOCGSIZE) */
}
#endif

/*
 * Global export routine to initialize the TERMCAP package.
 *
 * Returns:
 *		NULL	- ok
 *		!= NULL	- error message
 */
EXPORT Uchar *
t_start(wp)
	ewin_t	*wp;
{
	Uchar	*emsg;

	if ((emsg = UC tty_start(putoutchar)) != NULL)
		return (emsg);
	wp->psize = tty_pagesize()-1;
	wp->llen = tty_linelength()-1;
#ifdef	AUTO_MARGIN_BOTCH
	if (b_auto_right_margin) /* XXX */
		wp->llen--;
#endif
#if	defined(__CYGWIN32__) || defined(__CYGWIN__)
	wp->llen--;	/* Either a ioctl() or a window bug */
#endif

#ifdef	SIGWINCH
	signal(SIGWINCH, winch);
	winch(0);
#endif
	if (TTYhome || TTYcursor)
		f_home = 1;
	if (TTYinslines || (TTYcursor && TTYsscroll && TTYsdown))
		f_ins_line = 1;
	if (TTYdellines || (TTYcursor && TTYsscroll && TTYsup))
		f_del_line = 1;
	if (TTYinschars)
		f_ins_char = 1;
	if (TTYdelchars)
		f_del_char = 1;
	if (TTYaltvideo)
		f_alternate_video = 1;
	if (TTYcursor || (TTYhome && TTYdown && TTYright))
		f_move_cursor = 1;
	if (TTYclrscreen)
		f_clear_screen = 1;
	if (TTYleft || TTYcursor)
		f_left = 1;
	if (TTYright || TTYcursor)
		f_right = 1;
	if (TTYup || TTYcursor)
		f_up = 1;
	if (TTYdown || TTYcursor)
		f_down = 1;
	if (TTYsscroll && f_move_cursor)
		f_set_scroll_region = 1;
	if (TTYsup)
		f_scr_up = 1;
	if (TTYsdown)
		f_scr_down = 1;

	return (UC 0);
}


/*
 * Put Terminal into a mode useful for the TERMCAP packet.
 * This is the function that is intended to be called from upper level.
 */
EXPORT void
t_begin()
{
	tty_init();
}

/*
 * Restore Terminal mode to general purpose mode.
 * This is the function that is intended to be called from upper level.
 */
EXPORT void
t_done()
{
	tty_term();
}

/*
 * Indicate that someone tried to call a function that is
 * not available with the current terminal.
 */
EXPORT void
t_error(wp, s)
	ewin_t	*wp;
	char	*s;
{
	if (f_move_cursor) {
		writemsg(wp, "missing terminal cap: %s ", s);
	} else {
		ringbell();
		output(UC "missing terminal cap: ");
		output(UC s);
		output(UC " ");
	}
}

/*
 * Move the cursor home.
 * Do not the emulated t_move(). It may not always work correctly.
 */
EXPORT void
t_home(wp)
	ewin_t	*wp;
{
	if (TTYhome) {
		(*TTYhome)();
		cpos.vp = cpos.hp = 0;
	} else if (TTYcursor) {				/* Nur fuer t_error */
		(*TTYcursor)(cpos.vp = 0, cpos.hp = 0);	/* nicht t_move()!! */
	} else {
		t_error(wp, "HOME");
	}
}

/*
 * Move the cursor left n rows.
 * Be smart and use the function that will emmit the fewest number of chars.
 */
EXPORT void
t_left(wp, n)
		ewin_t	*wp;
	register int	n;
{
	if (n > cpos.hp)
		n = cpos.hp;
	if (n <= 0)
		return;
	if (TTYleft) {
		(*TTYleft)(n);
		cpos.hp -= n;
	} else {
		t_move(wp, cpos.vp, cpos.hp-n);
	}
}

/*
 * Move the cursor right n rows.
 * Be smart and use the function that will emmit the fewest number of chars.
 */
EXPORT void
t_right(wp, n)
		ewin_t	*wp;
	register int	n;
{
	if (n > wp->llen - cpos.hp)
		n = wp->llen - cpos.hp;
	if (n <= 0)
		return;
	if (TTYright) {
		(*TTYright)(n);
		cpos.hp += n;
	} else {
		t_move(wp, cpos.vp, cpos.hp+n);
	}
}

/*
 * Move the cursor up n lines.
 * Be smart and use the function that will emmit the fewest number of chars.
 */
EXPORT void
t_up(wp, n)
		ewin_t	*wp;
	register int	n;
{
	if (n > cpos.vp)
		n = cpos.vp;
	if (n <= 0)
		return;
	if (TTYup) {
		(*TTYup)(n);
		cpos.vp -= n;
	} else {
		t_move(wp, cpos.vp-n, cpos.hp);
	}
}

/*
 * Move the cursor down n lines.
 * Be smart and use the function that will emmit the fewest number of chars.
 */
EXPORT void
t_down(wp, n)
		ewin_t	*wp;
	register int	n;
{
	if (n > wp->psize - cpos.vp)
		n = wp->psize - cpos.vp;
	if (n <= 0)
		return;
	if (TTYdown) {
		(*TTYdown)(n);
		cpos.vp += n;
	} else {
		t_move(wp, cpos.vp+n, cpos.hp);
	}
}

/*
 * Move cursor. Try to use relative cursor movement if possible.
 */
EXPORT void
t_move(wp, row, col)	/* y , x */
		ewin_t	*wp;
	register int	row;
	register int	col;
{
	register int	size;

	if (row > wp->psize)
		row = wp->psize;
	if (col > wp->llen)
		col = wp->llen;
	if (cpos.vp > wp->psize || cpos.vp < 0 ||	/* Niemand weisz, wo der Curs */
	    cpos.hp > wp->llen || cpos.hp < 0) {	/* wirklich steht also Absolut*/
		t_move_abs(wp, row, col);
	} else if (row == cpos.vp) {
		size = col - cpos.hp;
		if (size <= 0 && TTYleft && size > -300)
			t_left(wp, -size);
		else if (size > 0 && TTYright && size < 300)
			t_right(wp, size);
		else
			t_move_abs(wp, row, col);
	} else if (col == cpos.hp) {
		size = row - cpos.vp;
		if (size <= 0 && TTYup && size > -300)
			t_up(wp, -size);
		else if (size > 0 && TTYdown && size < 300)
			t_down(wp, size);
		else
			t_move_abs(wp, row, col);
	} else {
		t_move_abs(wp, row, col);
	}
}

/*
 * Move cursor. Only absolute cursor movement allowed.
 */
EXPORT void
t_move_abs(wp, row, col)	/* y , x */
	ewin_t	*wp;
	int	row;
	int	col;
{
	if (TTYcursor) {
		(*TTYcursor)(cpos.vp = row, cpos.hp = col);
	} else if (TTYhome && TTYdown && TTYright) {
		(*TTYhome)();
		(*TTYdown)(cpos.vp = row);
		(*TTYright)(cpos.hp = col);
	} else {
		t_error(wp, "MCRA");
	}
}

/*
 * Clear screen.
 */
EXPORT void
t_clear(wp)
	ewin_t	*wp;
{
	if (TTYclrscreen) {
		(*TTYclrscreen)(wp);
		cpos.hp = cpos.vp = 0;
	} else if (TTYhome || TTYcursor) {
		t_home(wp);
		t_cleos(wp);
	} else {
		t_error(wp, "CLSC");
	}
}

/*
 * Clear from cursor to end of screen.
 * Be smart and use emulation if necessary.
 */
EXPORT void
t_cleos(wp)
	ewin_t	*wp;
{
	if (TTYclr_endscr) {
		(*TTYclr_endscr)(wp);
	} else {
			int	ov	= cpos.vp;
			int	oh	= cpos.hp;
		register int	tempv	= cpos.vp;

		/*
		 * We need to erase to the end of screen manually.
		 * Use the function t__cleol() which will do this job
		 * as good as possible.
		 */
		t__cleol(wp, 0);
		while (++tempv <= wp->psize) {
			if (b_auto_right_margin)
				cpos.hp = -1;	/* force abs cursor movement */
			t_move(wp, tempv, 0);
			t__cleol(wp, 0);
		}
		if (b_auto_right_margin)
			cpos.hp = -1;		/* force abs cursor movement */
		t_move(wp, ov, oh);
	}
}

/*
 * Clear from cursor to end of line.
 */
EXPORT void
t_cleol(wp)
	ewin_t	*wp;
{
	t__cleol(wp, 1);
}

/*
 * Clear from cursor to end of line.
 * Be smart and use emulation if necessary.
 */
EXPORT void
t__cleol(wp, do_move)
	ewin_t	*wp;
	int	do_move;
{
	if (TTYclrendln) {
		(*TTYclrendln)();
	} else {
			int	oh = cpos.hp;
		register int	temp;

		/*
		 * We need to erase to the end of the line manually.
		 * Be carefully with terminals that do automatic wrapping.
		 */
		temp = wp->llen-cpos.hp+1;
		if (b_auto_right_margin && cpos.vp >= wp->psize)
			temp--;
		while (--temp >= 0) {
			putoutchar(' ');
			cpos.hp++;
		}
		if (do_move) {
			if (b_auto_right_margin)
				cpos.hp = -1;	/* force abs cursor movement */
			t_move(wp, cpos.vp, oh);
		}
	}
}

/*
 * Insert n chars.
 */
EXPORT void
t_insch(wp, str)
	ewin_t	*wp;
	char	*str;

{
	if (TTYinschars) {
		(*TTYinschars)(str);
		cpos.hp += strlen(str);
	} else {
		t_error(wp, "INSC");
	}
}

/*
 * Insert n lines.
 */
EXPORT void
t_insln(wp, n)
	ewin_t	*wp;
	int	n;
{
	if (n <= 0)
		return;
	if (TTYinslines) {
		(*TTYinslines)(n);
	} else if (TTYsscroll && TTYsdown && TTYcursor) {
		(*TTYsscroll)(cpos.vp, wp->psize);
		(*TTYcursor)(cpos.vp, 0);
		(*TTYsdown)(n);
		(*TTYsscroll)(0, wp->psize);
		(*TTYcursor)(cpos.vp, cpos.hp);
	} else {
		t_error(wp, "INSL");
	}
}

/*
 * Delete n chars.
 */
EXPORT void
t_delch(wp, n)
	ewin_t	*wp;
	int	n;
{
	if (n <= 0)
		return;
	if (TTYdelchars)
		(*TTYdelchars)(n);
	else
		t_error(wp, "DELC");
}

/*
 * Delete n lines.
 */
EXPORT void
t_delln(wp, n)
	ewin_t	*wp;
	int	n;
{
	if (n <= 0)
		return;
	if (TTYdellines) {
		(*TTYdellines)(n);
/*		cpos.hp = 0;*/	/* das ist definitiv falsch */
		if (cpos.hp)	/* niemand weis, wo der Cursor danach steht */
			cpos.hp = -1;
	} else if (TTYsscroll && TTYsup && TTYcursor) {
		(*TTYsscroll)(cpos.vp, wp->psize);
		(*TTYcursor)(wp->psize, 0);
		(*TTYsup)(n);
		(*TTYsscroll)(0, wp->psize);
		(*TTYcursor)(cpos.vp, cpos.hp = 0);
	} else {
		t_error(wp, "DELL");
	}
}

/*
 * Output 'string' in stand-out mode.
 */
EXPORT void
t_alt(str)
	char	*str;
{
	if (TTYaltvideo)
		(*TTYaltvideo)(str);
	else
		output(UC str);
}

/*
 * Set scrolling region.
 */
EXPORT void
t_setscroll(wp, beg, end)
		ewin_t	*wp;
	register int	beg;
	register int	end;
{
	if (end > wp->psize)
		end = wp->psize;
	if (beg < 0)
		beg = 0;

	if (TTYsscroll) {
		(*TTYsscroll)(beg, end);
		t_move(wp, cpos.vp, cpos.hp);
	} else {
		t_error(wp, "SSCR");
	}
}

/*
 * Scroll content of current scrolling region up.
 */
EXPORT void
t_scrup(wp, n)
		ewin_t	*wp;
	register int	n;
{
	if (n > wp->psize - cpos.vp)
		n = wp->psize - cpos.vp;
	if (n <= 0)
		return;
	if (TTYsup) {
		(*TTYsup)(n);
		cpos.vp += n;
	} else {
		t_error(wp, "SCUP");
	}
}

/*
 * Scroll content of current scrolling region down.
 */
EXPORT void
t_scrdown(wp, n)
		ewin_t	*wp;
	register int	n;
{
	if (n > cpos.vp)
		n = cpos.vp;
	if (n <= 0)
		return;
	if (TTYsdown) {
		(*TTYsdown)(n);
		cpos.vp -= n;
	} else {
		t_error(wp, "SDWN");
	}
}
