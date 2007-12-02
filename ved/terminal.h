/* @(#)terminal.h	1.6 04/03/12 Copyright 1989, 1996-2004 J. Schilling */
/*
 *	Definitions tor the exported layer of the TERMPCAP interface.
 *
 *	Copyright (c) 1989, 1996-2004 J. Schilling
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
 * Function names must be unique within 7 chars to allow compilationson systems
 * with a linker that supports only 8 chars in identifiers.
 * CPP supports at least 32 char "identifiers".
 * We redefine short function names to longer upper case names to make
 * the termcap interface functions easy to find and easy to understand.
 */
#define	CURSOR_HOME		t_home		/* ho corsor home	    */
#define	CURSOR_LEFT		t_left		/* le cursor left	    */
#define	CURSOR_RIGHT		t_right		/* nd cursor right	    */
#define	CURSOR_UP		t_up		/* up cursor up		    */
#define	CURSOR_DOWN		t_down		/* do cursor down	    */
#define	MOVE_CURSOR		t_move		/* cm* optimised	    */
#define	MOVE_CURSOR_ABS		t_move_abs	/* cm cursor address	    */
#define	CLEAR_SCREEN		t_clear		/* cl clear screen	    */
#define	CLEAR_TO_EOF_SCREEN	t_cleos		/* cd clear to end of screen */
#define	CLEAR_TO_EOF_LINE	t_cleol		/* cl clear to end of line  */
#define	INSERT_CHAR		t_insch		/* ic insert character	    */
#define	INSERT_LINE		t_insln		/* al insert line	    */
#define	DELETE_CHAR		t_delch		/* dc delete character	    */
#define	DELETE_LINE		t_delln		/* dl delete line	    */
#define	WRITE_ALT		t_alt		/* ** write alt		    */
#define	SET_SCROLL_REGION	t_setscroll	/* cs change scroll region  */
#define	SCROLL_UP		t_scrup		/* sf scroll text up	    */
#define	SCROLL_DOWN		t_scrdown	/* sr scroll text down	    */

/*
 * The following variables are used to tell our users
 * whether a specific function is supported or not.
 */
extern char f_home;		/* Function Cursor Home			*/
extern char f_era_screen;	/* Function clear to end of screen	*/
extern char f_era_line;		/* Function clear to end of line	*/
extern char f_ins_line;		/* Function insert line			*/
extern char f_del_line;		/* Function lelete line			*/
extern char f_ins_char;		/* Function insert character		*/
extern char f_del_char;		/* Function delete character		*/
extern char f_alternate_video;	/* Function alternate video		*/
extern char f_move_cursor;	/* Function move cursor			*/
extern char f_clear_screen;	/* Function clear screen & home cursor	*/
extern char f_left;		/* Function cursor left			*/
extern char f_right;		/* Function cursor right		*/
extern char f_up;		/* Function cursor up			*/
extern char f_down;		/* Function cursor down			*/
extern char f_set_scroll_region; /* Function set scrolling region	*/
extern char f_scr_up;		/* Function scroll up			*/
extern char f_scr_down;		/* Function scroll down			*/


#define	b_auto_right_margin	AM
/* variables telling whether the various characteristics exits.		*/

extern char b_auto_right_margin; /* Terminal does automatic wrapping	*/
