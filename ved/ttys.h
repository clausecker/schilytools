/* @(#)ttys.h	1.9 04/03/12 Copyright 1984-2004 J. Schilling */
/*
 *	Definitione for the internal lower layer support for terminal.h
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
 * The external interface of ttycmds.c has the following functions:
 *
 *	tty_start()	- Parse TERMCAP entry and do set up for this package
 *	tty_entry(i)	- Return pointer to decoded TERMCAP entries
 *	tty_fkey(i)	- Return TERMCAP mapping for function key 'i'
 *	tty_pagesize()	- Return the terminal's pagesize from TERMCAP entry
 *	tty_linelength()- Return the terminal's linelength from TERMCAP entry
 *	tty_init()	- Initialize terminal (send startup sequence)
 *	tty_term()	- Restore terminal to default mode (send term sequence)
 *
 *	TTY*		- Several pointers to apropriate functions
 *			  implementing TERMCAP functionality.
 *			  These pointers are not intended for direct use by the
 *			  higher level software but for terminal.c
 *
 * If a specific TERMCAP functionality is not present the corresponding
 * TTY* function pointer is a NULL pointer.
 *
 * tty_pagesize() and tty_linelength() return the "real" size of the
 * screen as returnd by tgetent(). Corrections for "auto_right_margin"
 * are done in the upper layers.
 */

extern	void (*TTYclrscreen)	__PR((ewin_t *wp));	/* clr screen */
extern	void (*TTYclr_endscr)	__PR((ewin_t *wp));	/* clr to end of scr */
extern	void (*TTYclrendln)	__PR((void));		/* clr to end of line*/
extern	void (*TTYdelchars)	__PR((int));		/* delete chars */
extern	void (*TTYdellines)	__PR((int));		/* delete lines */
extern	void (*TTYinschars)	__PR((char *));		/* insert chars */
extern	void (*TTYinslines)	__PR((int));		/* insert lines */
extern	void (*TTYaltvideo)	__PR((char *));		/* alternate video */
extern	void (*TTYhome)		__PR((void));		/* cursor home */
extern	void (*TTYup)		__PR((int));		/* cursor up */
extern	void (*TTYdown)		__PR((int));		/* cursor down */
extern	void (*TTYleft)		__PR((int));		/* cursor left */
extern	void (*TTYright)	__PR((int));		/* cursor right */
extern	void (*TTYcursor)	__PR((int y, int x));	/* move cursor abs */
extern	void (*TTYsscroll)	__PR((int, int));	/* set scroll region */
extern	void (*TTYsup)		__PR((int));		/* scroll up */
extern	void (*TTYsdown)	__PR((int));		/* scroll down */


extern	char *	tty_start	__PR((int (*)(int c)));
extern	char **	tty_entry	__PR((void));
extern	void	tty_init	__PR((void));
extern	void	tty_term	__PR((void));
extern	char *	tty_fkey	__PR((int n));
extern	int	tty_pagesize	__PR((void));
extern	int	tty_linelength	__PR((void));
