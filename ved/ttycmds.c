/* @(#)ttycmds.c	1.30 21/07/07 Copyright 1984-2021 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)ttycmds.c	1.30 21/07/07 Copyright 1984-2021 J. Schilling";
#endif
/*
 *	Lower layer support routines for terminal.c
 *
 *	Copyright (c) 1984-2021 J. Schilling
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
 * Exports:
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
 * If a specific function is not available in a parameterized form directly,
 * this functionality is emulated by multiple calls to the simple function.
 * e.g. if your terminal does not support to move the cursor 'n' lines down,
 * this package calls the cursor-down-single function 'n' times.
 */
#include "ved.h"
#include "ttys.h"
#include <schily/termcap.h>

LOCAL	int (*sputchar)		__PR((int c))	= 0;	/* scr putc for tputs*/

EXPORT	void (*TTYclrscreen)	__PR((ewin_t *wp))	= 0;	/* clr screen */
EXPORT	void (*TTYclr_endscr)	__PR((ewin_t *wp))	= 0;	/* clr to end of scr */
EXPORT	void (*TTYclrendln)	__PR((void))	= 0;	/* clr to end of line*/
EXPORT	void (*TTYdelchars)	__PR((int))	= 0;	/* delete chars */
EXPORT	void (*TTYdellines)	__PR((int))	= 0;	/* delete lines */
EXPORT	void (*TTYinschars)	__PR((char *))	= 0;	/* insert chars */
EXPORT	void (*TTYinslines)	__PR((int))	= 0;	/* insert lines */
EXPORT	void (*TTYaltvideo)	__PR((char *))	= 0;	/* alternate video */
EXPORT	void (*TTYhome)		__PR((void))	= 0;	/* cursor home */
EXPORT	void (*TTYup)		__PR((int))	= 0;	/* cursor up */
EXPORT	void (*TTYdown)		__PR((int))	= 0;	/* cursor down */
EXPORT	void (*TTYleft)		__PR((int))	= 0;	/* cursor left */
EXPORT	void (*TTYright)	__PR((int))	= 0;	/* cursor right */
EXPORT	void (*TTYcursor)	__PR((int y, int x)) = 0; /* move cursor abs */
EXPORT	void (*TTYsscroll)	__PR((int, int)) = 0;	/* set scroll region */
EXPORT	void (*TTYsup)		__PR((int))	= 0;	/* scroll up */
EXPORT	void (*TTYsdown)	__PR((int))	= 0;	/* scroll down */


LOCAL	char	_stbuf[1024];	/* storage for the decoded termcap entries */
LOCAL	char	*stbp;		/* export storage pointer for map.c */
LOCAL	int	pagesize;	/* number of lines in a page */
LOCAL	int	linelength;	/* numver of characters in a line */

/*
 * We add kb=\b to the ansi terminal capabilities in order to be
 * friendly to people who are using a PC type keyboard.
 */
LOCAL	char	builtin_ansi[] =
    "sx|ansi|any ansi terminal with pessimistic assumptions:\
	:co#80:li#24:cl=50\\E[;H\\E[2J:bs:am:cm=\\E[%i%d;%dH:\
	:nd=\\E[C:up=\\E[A:ce=\\E[K:ho=\\E[H:pt:kb=\b:";

#ifdef	USE_DUMB
LOCAL	char	builtin_dumb[] = "xx|dumb:";
#endif

#undef	HZ		/* param.h included ???			*/
#undef	UC		/* is defined (unsigned char *) in ved.h*/

char	AM;		/* Automatic Margins			*/
char	BS;		/* Backspace works			*/
char	CA;		/* Cursor Adressable			*/
char	DA;		/* Display retained above the Screen	*/
char	DB;		/* Display retained below the Screen	*/
char	EO;		/* ca Erase Overstrikes with a blank	*/
char	HC;		/* Hardcopy Terminal			*/
char	HZ;		/* Cannot print ~s (Hazeltine)		*/
char	IN;		/* Insert Mode distinguish nulls	*/
char	MI;		/* Can move in Insert Mode		*/
char	MS;		/* Save to move in standout Mode	*/
char	NC;		/* Not correctly working CR		*/
char	NS;		/* CRT that does not Scroll		*/
char	OS;		/* Overstrike works			*/
char	UL;		/* Underline Character overstrikes	*/
char	XB;		/* Beehive (f1=ESC, f2=^C)		*/
char	XN;		/* Newline ignored after 80 cols	*/
char	XT;		/* TABS destructive, so magic		*/
char	XS;		/* Standout not erased by overwriting	*/
char	XX;		/* Tektronix Insert Line		*/

char	*AL;		/* Add 1 line				*/
char	*AL_PARM;	/* Add n lines				*/
#ifdef	OLD_TERMCAP	/* 'BC' is now defined in tgoto.c	*/
char	*BC;		/* Back Cursor movement (Cursor left)	*/
#endif
char	*BT;		/* Back Tab				*/
char	*CD;		/* Clear to End of Display		*/
char	*CE;		/* Clear to End of Line			*/
char	*CL;		/* Clear Scereen			*/
char	*CM;		/* Cursor Motion			*/
char	*CR;		/* Carriage return			*/
#undef	CS		/* We may have included sys/regset.h	*/
char	*CS;		/* Change scrolling region		*/
char	*DC;		/* Delete 1 character			*/
char	*DC_PARM;	/* Delete n characters			*/
char	*DL;		/* Delete 1 line			*/
char	*DL_PARM;	/* Delete n lines			*/
char	*DM;		/* Delete mode enter			*/
char	*DO;		/* Cursor down 1 line			*/
char	*DOWN_PARM;	/* Cursor down n lines			*/
char	*ED;		/* End delete mode			*/
char	*EI;		/* End insert mode			*/
char	*HO;		/* Home cursor				*/
char	*IC;		/* Insert 1 character			*/
char	*IC_PARM;	/* Insert n characters			*/
char	*IM;		/* Insert mode (add im=: if has ic=)	*/
char	*IP;		/* Insert pad after using IM and EI	*/
char	*KD;		/* Keypad down arrow			*/
char	*KE;		/* Keypad end transmit mode		*/
char	*KH;		/* Keypad home key			*/
char	*KEK;		/* Keypad end key			*/
char	*KDK;		/* Keypad delete key			*/
char	*KL;		/* Keypad left arrow			*/
char	*KR;		/* Keypad right arrow			*/
char	*KS;		/* Keypad start transmit mode		*/
char	*KU;		/* Keypad up arrow			*/
char	*LEFT_PARM;	/* Cursor left n chars			*/
char	*LL;		/* Move to last line, first column	*/
char	*MA;		/* Arrow key map			*/
char	*MD;		/* Bold mode start			*/
char	*ME;		/* All attribute end			*/
char	*ND;		/* Non destructive space (Cursor right)	*/
char	*NL;		/* Linefeed if not NL			*/
char	*RC;		/* Restore cursor			*/
char	*RIGHT_PARM;	/* Right n chars			*/
char	*SC;		/* Save cursor				*/
char	*SE;		/* Standout end				*/
char	*SF;		/* Scroll forward 1 line		*/
char	*SF_PARM;	/* Scroll forward n lines		*/
char	*SO;		/* Standout begin			*/
char	*SR;		/* Scroll reverse 1 line		*/
char	*SR_PARM;	/* Scroll reverse n lines		*/
char	*TA;		/* Tab if not ^I or need padding	*/
char	*TE;		/* Terminal end sequence		*/
char	*TI;		/* Terminal init sequence		*/
char	*UC;		/* Underscore one char and move past it	*/
char	*UE;		/* Underscore mode end			*/
#ifdef	OLD_TERMCAP	/* 'UP' is now defined in tgoto.c	*/
char	*UP;		/* Cursor up 1 line			*/
#endif
char	*UP_PARM;	/* Cursor up n lines			*/
char	*US;		/* Underscore mode start		*/
char	*VB;		/* Visible bell				*/
char	*VE;		/* Visual end sequence			*/
char	*VS;		/* Visual start sequence		*/

/*
 * Various function keys (f0 .. f19)
 */
char	*K0,  *K1,  *K2,  *K3,  *K4,  *K5,  *K6,  *K7,  *K8,  *K9;
char	*K10, *K11, *K12, *K13, *K14, *K15, *K16, *K17, *K18, *K19;

LOCAL	char	*_PC;	/* Pad Character String			*/
#ifdef	OLD_TERMCAP	/* 'PC' is now defined in tputs.c	*/
char	PC;		/* Pad Character			*/
#endif

/*
 * From the tty modes...
 */
char	GT;		/* Gtty told us that we may use tabs	*/
char	NONL;		/* Terminal don't needs linefeed on CR	*/
char	UPPERCASE;	/* Terminal is upper case only		*/
#ifdef	OLD_TERMCAP	/* 'ospeed' is now defined in tputs.c	*/
short	ospeed = -1;	/* Needed to compute padding in tputs()	*/
#endif

LOCAL	char	*tflags[] = {
			&AM, &BS, &DA, &DB, &EO, &HC, &HZ, &IN, &MI,
			&MS, &NC, &NS, &OS, &UL, &XB, &XN, &XT, &XS,
			&XX
		};

LOCAL	char	**tstrs[] = {
			&AL, &BC, &BT, &CD, &CE, &CL, &CM, &CR, &CS,
			&DC, &DL, &DM, &DO, &ED, &EI, &HO, &IC,
			&IM, &IP, &KD, &KE, &KH, &KL, &KR, &KS, &KU,
			&LL, &MA, &MD, &ME, &ND, &NL, &_PC, &RC, &SC, &SE, &SF,
			&SO, &SR, &TA, &TE, &TI, &UC, &UE, &UP, &US,
			&VB, &VS, &VE, &AL_PARM, &DL_PARM, &UP_PARM,
			&DOWN_PARM, &LEFT_PARM, &RIGHT_PARM, &DC_PARM,
			&IC_PARM, &SF_PARM, &SR_PARM,
			&KEK, &KDK,
		};

LOCAL	char	**tfkeys[] = {
		&K0,  &K1,  &K2,  &K3,  &K4,  &K5,  &K6,  &K7,  &K8,  &K9,
		&K10, &K11, &K12, &K13, &K14, &K15, &K16, &K17, &K18, &K19,
		};

EXPORT	char *	tty_start	__PR((int (*)(int c)));
LOCAL	void	tputstr		__PR((char *s));
EXPORT	char **	tty_entry	__PR((void));
EXPORT	void	tty_init	__PR((void));
EXPORT	void	tty_term	__PR((void));
EXPORT	char *	tty_fkey	__PR((int n));
EXPORT	int	tty_pagesize	__PR((void));
EXPORT	int	tty_linelength	__PR((void));
LOCAL	void	cur_abs		__PR((int y, int x));
LOCAL	void	cl_screen	__PR((ewin_t *wp));
LOCAL	void	cl_e_screen	__PR((ewin_t *wp));
LOCAL	void	cl_e_line	__PR((void));
LOCAL	void	home_cursor	__PR((void));
LOCAL	void	up_S		__PR((int n));
LOCAL	void	up_M		__PR((int n));
LOCAL	void	down_S		__PR((int n));
LOCAL	void	down_M		__PR((int n));
LOCAL	void	left_S		__PR((int n));
LOCAL	void	left_M		__PR((int n));
LOCAL	void	right_S		__PR((int n));
LOCAL	void	right_M		__PR((int n));
LOCAL	void	del_c_S		__PR((int n));
LOCAL	void	del_c_M		__PR((int n));
LOCAL	void	del_l_S		__PR((int n));
LOCAL	void	del_l_M		__PR((int n));
LOCAL	void	ins_c_S		__PR((char *str));
LOCAL	void	ins_c_M		__PR((char *str));
LOCAL	void	ins_str		__PR((char *str));
LOCAL	void	ins_l_S		__PR((int n));
LOCAL	void	ins_l_M		__PR((int n));
#ifdef	__needed__
LOCAL	void	alt_S_Svideo	__PR((char *str));
#endif
LOCAL	void	alt_S_video	__PR((char *str));
LOCAL	void	alt_B_video	__PR((char *str));
LOCAL	void	alt_U_video	__PR((char *str));
LOCAL	void	sscr		__PR((int b, int e));
LOCAL	void	sup_S		__PR((int n));
LOCAL	void	sup_M		__PR((int n));
LOCAL	void	sdown_S		__PR((int n));
LOCAL	void	sdown_M		__PR((int n));
LOCAL	void	gettc		__PR((void));
LOCAL	void	setupttycmds	__PR((void));


/*
 * Initialization ot the TERMCAP package.
 * -	Read the TERMCAP data base and set pagesize & linelength.
 * -	Set up function key maps.
 * -	Set up pointers to the implementation functions.
 *
 * outchar() is a pointer to the function that is used to send
 * the control sequences to the terminal.
 *
 * Returns:
 *	NULL	- success
 *	!= NULL	- pointer to string with error message
 */
EXPORT char *
tty_start(outchar)
	int (*outchar) __PR((int c));
{
		int	ret;
	static	char	noterm[] = "unknown";
		char	*tname;
		char	*errstr = NULL;

	/*
	 * Look up the TERMCAP database for the current terminal type.
	 * Use some "smart" defaults if "TERM" is not defined or contains junk.
	 */
	if ((tname = getenv("TERM")) == NULL)
		tname = noterm;
	if (tname[0] == '\0')
		tname = "xx";

/*	unknown = FALSE;*/
	seterrno(0);
	if ((ret = tgetent(NULL, tname)) != 1) {
		char	*bp = tcgetbuf();

/*		unknown++;*/

#ifndef	NO_BUILTIN_ANSI
		if (bp) {
			if (ret < 0) {
				errmsgno(EX_BAD,
				    "Cannot open termcap file '%s'.\n", bp);
			}
			errmsgno(EX_BAD,
			    "Cannot find description for 'TERM=%s'.\n",
			    tname);
			errmsgno(EX_BAD,
			    "Using builtin ansi terminal description.\n");
			errmsgno(EX_BAD,
			    "Please install a termcap file in %s %s.\n",
			    "$HOME/.termcap",
			    "to avoid this message");
			sleep(1);
			strcpy(bp, builtin_ansi);
		} else
#endif
		if (ret == -1) {
			js_snprintf(_stbuf, sizeof (_stbuf),
					"Cannot open termcap file.\n");
			return (_stbuf);
		} else {
			js_snprintf(_stbuf, sizeof (_stbuf),
					"Cannot find terminal type: %s.\n",
								tname);
			if (geterrno() == 0)
				seterrno(EX_BAD);
			return (_stbuf);
		}
	}

	if (errstr)		/* XXX */
		return (errstr);

#ifdef	__never__
	/*
	 * Do not set up  default unless we invented a method to let ved
	 * work on a hardcopy terminal.
	 */
	if ((pagesize = tgetnum("li")) <= 1)
		pagesize = 2;
	if ((linelength = tgetnum("co")) <= 4)
		linelength = 80;
#endif

	/*
	 *  Now, parse the content of the TERMCAP data base buffer.
	 */
	gettc();

	/*
	 * Set up the exported implementation function pointers
	 * and a pointer to the output function.
	 */
	init_fk_maps();
	setupttycmds();
	sputchar = outchar;
	return (0);
}

LOCAL void
tputstr(s)
	register char	*s;
{
	while (*s) {
		if (*s == '\n')
			(*sputchar)('\r');
		(*sputchar)(*s++);
	}
}

/*
 * Return space to be used by tgetstr()
 */
EXPORT char **
tty_entry()
{
	return (&stbp);
}

/*
 * Put Terminal into a mode useful for the TERMCAP packet.
 */
EXPORT void
tty_init()
{
	if (!XT) GT = 0;		/* Do not use tabs if destructive */

	if (TI)
		tputs(TI, 0, sputchar);	/* initialize terminal */
	if (VS)
		tputs(VS, 0, sputchar);	/* make cursor visible */
	if (KS)
		tputs(KS, 0, sputchar);	/* start keypad transmit mode */

}

/*
 * Restore Terminal mode to general purpose mode.
 */
EXPORT void
tty_term()
{
	if (VE && sputchar)
		tputs(VE, 0, sputchar);
	if (KE && sputchar)
		tputs(KE, 0, sputchar);
	if (TE && sputchar)
		tputs(TE, 0, sputchar);

}

/*
 * Return decoded strings for function keys.
 */
EXPORT char *
tty_fkey(n)
	int	n;
{
	if (n >= 0 && n < (sizeof (tfkeys)/sizeof (tfkeys[0])))
		return (*tfkeys[n]);
	return ((char *)0);
}

/*
 * Return pagesize from TERMCAP entry for TERM.
 */
EXPORT int
tty_pagesize()
{
	return (pagesize);
}

/*
 * Return linelength from TERMCAP entry for TERM.
 */
EXPORT int
tty_linelength()
{
	return (linelength);
}

/*
 * move cursor absolute (Y/row/vpos, X/col/hpos)
 */
LOCAL void
cur_abs(y, x)
	int	y;
	int	x;
{
	tputs(tgoto(CM, x, y), 0, sputchar);
}

/*
 * clear screen
 */
LOCAL void
cl_screen(wp)
	ewin_t	*wp;
{
	tputs(CL, wp->psize, sputchar);
}

/*
 * clear from cursor to end of screen
 */
LOCAL void
cl_e_screen(wp)
	ewin_t	*wp;
{
	tputs(CD, wp->psize-cpos.vp, sputchar);
}

/*
 * clear from cursor to end of line
 */
LOCAL void
cl_e_line()
{
	tputs(CE, 1, sputchar);
}

/*
 * home cursor
 */
LOCAL void
home_cursor()
{
	tputs(HO, 0, sputchar);
}

/*
 * cursor up - using non parameter mode
 */
LOCAL void
up_S(n)
	register int n;
{
	while (--n >= 0)
		tputs(UP, 0, sputchar);
}

/*
 * cursor up - parameter mode
 */
LOCAL void
up_M(n)
	int	n;
{
	tputs(tgoto(UP_PARM, 0, n), n, sputchar);
}

/*
 * cursor down - using non parameter mode
 */
LOCAL void
down_S(n)
	register int n;
{
	while (--n >= 0)
		tputs(DO, 0, sputchar);
}

/*
 * cursor down - parameter mode
 */
LOCAL void
down_M(n)
	int	n;
{
	tputs(tgoto(DOWN_PARM, 0, n), n, sputchar);
}

/*
 * cursor left - using non parameter mode
 */
LOCAL void
left_S(n)
	register int n;
{
	while (--n >= 0)
		tputs(BC, 0, sputchar);
}

/*
 * cursor left - parameter mode
 */
LOCAL void
left_M(n)
	int	n;
{
	tputs(tgoto(LEFT_PARM, 0, n), n, sputchar);
}

/*
 * cursor right - using non parameter mode
 */
LOCAL void
right_S(n)
	register int n;
{
	while (--n >= 0)
		tputs(ND, 0, sputchar);
}

/*
 * cursor right - parameter mode
 */
LOCAL void
right_M(n)
	int	n;
{
	tputs(tgoto(RIGHT_PARM, 0, n), n, sputchar);
}

/*
 * delete chars - using non parameter mode
 */
LOCAL void
del_c_S(n)
	register int n;
{
	while (--n >= 0)
		tputs(DC, 0, sputchar);
}

/*
 * delete chars - parameter mode
 */
LOCAL void
del_c_M(n)
	int	n;
{
	tputs(tgoto(DC_PARM, 0, n), n, sputchar);
}

/*
 * delete lines - using non parameter mode
 */
LOCAL void
del_l_S(n)
	register int n;
{
	while (--n >= 0)
		tputs(DL, 0, sputchar);
}

/*
 * delete lines - parameter mode
 */
LOCAL void
del_l_M(n)
	int	n;
{
	tputs(tgoto(DL_PARM, 0, n), n, sputchar);
}

/*
 * insert chars - using non parameter mode
 */
LOCAL void
ins_c_S(str)
	register char *str;
{
	while (*str) {
		tputs(IC, 0, sputchar);
		(*sputchar)(*str++);
	}
}

/*
 * insert chars - parameter mode
 */
LOCAL void
ins_c_M(str)
	char *str;
{
	int	n = strlen(str);

	tputs(tgoto(IC_PARM, 0, n), n, sputchar);
	tputstr(str);
}

/*
 * insert string
 */
LOCAL void
ins_str(str)
	char *str;
{
	tputs(IM, 0, sputchar);
	tputstr(str);
	tputs(EI, 0, sputchar);
}

/*
 * insert lines - using non parameter mode
 */
LOCAL void
ins_l_S(n)
	register int n;
{
	while (--n >= 0)
		tputs(AL, 0, sputchar);
}

LOCAL void
/*
 * insert lines - parameter mode
 */
ins_l_M(n)
	int	n;
{
	tputs(tgoto(AL_PARM, 0, n), n, sputchar);
}

#ifdef	__needed__
/*
 * alternate video - using non parameter mode
 * XXX is this function possible with termcap?
 */
LOCAL void
alt_S_Svideo(str)
	register char *str;
{
	while (*str) {
/*		tputs(SO XXX, 0, sputchar);*/
		(*sputchar)(*str++);
	}
}
#endif

/*
 * alternate video - using Standout Mode
 */
LOCAL void
alt_S_video(str)
	register char *str;
{
	tputs(SO, 0, sputchar);
	tputstr(str);
	tputs(SE, 0, sputchar);
}

/*
 * alternate video - using Bold Mode
 */
LOCAL void
alt_B_video(str)
	register char *str;
{
	tputs(MD, 0, sputchar);
	tputstr(str);
	tputs(ME, 0, sputchar);
}

/*
 * alternate video - using Underline Mode
 */
LOCAL void
alt_U_video(str)
	register char *str;
{
	tputs(US, 0, sputchar);
	tputstr(str);
	tputs(UE, 0, sputchar);
}

/*
 * set scroll region
 */
LOCAL void
sscr(b, e)
	int	b;
	int	e;
{
	tputs(tgoto(CS, e, b), 0, sputchar);
}

/*
 * scroll up - using non parameter mode
 */
LOCAL void
sup_S(n)
	register int n;
{
	while (--n >= 0)
		tputs(SF, 0, sputchar);
}

/*
 * scroll up - parameter mode
 */
LOCAL void
sup_M(n)
	int	n;
{
	tputs(tgoto(SF_PARM, 0, n), n, sputchar);
}

/*
 * scroll down - using non parameter mode
 */
LOCAL void
sdown_S(n)
	register int n;
{
	while (--n >= 0)
		tputs(SR, 0, sputchar);
}

/*
 * scroll down - parameter mode
 */
LOCAL void
sdown_M(n)
	int	n;
{
	tputs(tgoto(SR_PARM, 0, n), n, sputchar);
}

/*
 * Get Flags and Strings from Termcap Entry
 */
LOCAL void
gettc()
{
	register char	*np;
	register char	**fp;
	register char	***sp;
		char	*sbp;

	sbp = _stbuf;

	/*
	 * Parse boolean flags.
	 */
	np = "ambsdadbeohchzinmimsncnsosulxbxnxtxsxx";
	fp = tflags;
	do {
		*(*fp++) = tgetflag(np);
		np += 2;
	} while (*np);

	/*
	 * Parse string capabilities.
	 */
	np = "albcbtcdceclcmcrcsdcdldmdoedeihoicimipkdkekhklkrkskullmamdmendnlpcrcscsesfsosrtatetiucueupusvbvsveALDLUPDOLERIDCICSFSR@7kD";
	sp = tstrs;
	do {
		*(*sp++) = tgetstr(np, &sbp);
		np += 2;
	} while (*np);

	/*
	 * Parse function key values.
	 */
	np = "k0k1k2k3k4k5k6k7k8k9k;F1F2F3F4F5F6F7F8F9";
	sp = tfkeys;
	do {
		*(*sp++) = tgetstr(np, &sbp);
		np += 2;
	} while (*np);

	stbp = sbp;

	if (XS) {
		SO = SE = NULL;
	} else {
		/*
		 * Enter/Exit Standout Glitch present?
		 */
		if (tgetnum("sg") > 0)	/* # of glitch chars left by so or se*/
			SO = NULL;
		/*
		 * Enter/Exit Underline Glitch present?
		 */
		if (tgetnum("ug") > 0)	/* # of glitch chars left by us or ue*/
			US = NULL;
		if (!SO && US) {
			SO = US;
			SE = UE;
		}
	}

	/*
	 * Handle funny termcap capabilities
	 */
	if (CS && SC && RC) AL = DL = "";	/* XXX */
	if (AL_PARM && AL == NULL) AL = "";
	if (DL_PARM && DL == NULL) DL = "";
	if (IC && IM == NULL) IM = "";
	if (IC && EI == NULL) EI = "";
	if (IM == NULL || EI == NULL) IM = EI = NULL;
	if (!XT) GT = 0;	/* Do not use tabs if destructive */
	if (!GT) BT = NULL;	/* If we can't tab, we can't backtab either */

	if (tgoto(CM, 2, 2)[0] == 'O')
		CA = FALSE, CM = 0;
	else
		CA = TRUE;

	PC = _PC ? _PC[0] : '\0';

/*	if (unknown)*/
/*		return ERR;*/
/*	return OK;*/
}

LOCAL void
setupttycmds()
{
#ifdef CURSOR
		char	*ku;
		char	*kd;
		char	*kr;
		char	*kl;
		char	*kB;
		char	*kE;
#endif

#ifdef CURSOR
	ku = tgetstr("ku", &sbp);		/* Corsor up */
	kd = tgetstr("kd", &sbp);		/* Corsor down */
	kr = tgetstr("kr", &sbp);		/* Corsor forward */
	kl = tgetstr("kl", &sbp);		/* Corsor left */
	kB = tgetstr("kB", &sbp);		/* Corsor leftmost */
	kE = tgetstr("kE", &sbp);		/* Corsor rightmost */
	if (ku) _add_map(ku, "");		/* Corsor up */
	if (kd) _add_map(kd, "");		/* Corsor down */
	if (kr) _add_map(kr, "");		/* Corsor forward */
	if (kl) _add_map(kl, "");		/* Corsor left */
	if (kB) _add_map(kB, "");		/* Corsor leftmost */
	if (kE)	_add_map(kE, "");		/* Corsor rightmost */
#endif

	if (CA)
		TTYcursor = cur_abs;
	if (CL)
		TTYclrscreen = cl_screen;
	if (CD)
		TTYclr_endscr = cl_e_screen;
	if (CE)
		TTYclrendln = cl_e_line;
	if (HO)
		TTYhome = home_cursor;

	if (UP_PARM)
		TTYup = up_M;
	else if (UP)
		TTYup = up_S;
	if (DOWN_PARM)
		TTYdown = down_M;
	else if (DO)
		TTYdown = down_S;
	if (LEFT_PARM)
		TTYleft = left_M;
	else if (BC)
		TTYleft = left_S;
	if (RIGHT_PARM)
		TTYright = right_M;
	else if (ND)
		TTYright = right_S;

	if (DC_PARM)
		TTYdelchars = del_c_M;
	else if (DC)
		TTYdelchars = del_c_S;
	if (DL_PARM)
		TTYdellines = del_l_M;
	else if (DL && *DL)
		TTYdellines = del_l_S;

	if (IC_PARM)
		TTYinschars = ins_c_M;
	/*
	 * Prüfen, wann IM und EI im vi verwendet werden !!!
	 * Es sieht so aus, als ob im vi hier der volle Pfusch abgeht.
	 * Mal nimmt er er IM und EI, mal nimmt er IC.
	 * Man muß hier wohl sehr vorsichtig sein, was die Richtigkeit
	 * der Termcap Einträge angeht, denn die sind wohl auf das Chaos
	 * im vi abgestimmt. Nach vorsichtiger Betrachtung scheint der vi
	 * auf eine sehr komplexe Art beides geschachtelt zu verwenden.
	 */
	else if (IM && *IM)
		TTYinschars = ins_str;
	else if (IC)
		TTYinschars = ins_c_S;
	if (AL_PARM)
		TTYinslines = ins_l_M;
	else if (AL && *AL)
		TTYinslines = ins_l_S;

	if (SO && SE)
		TTYaltvideo = alt_S_video;
	else if (MD && ME)
		TTYaltvideo = alt_B_video;
	else if (US && UE)
		TTYaltvideo = alt_U_video;

	if (CS)
		TTYsscroll = sscr;

	if (SF_PARM)
		TTYsup = sup_M;
	else if (!NS) {
		SF = "\n";
		TTYsup = sup_S;
	} else if (SF)
		TTYsup = sup_S;

	if (SR_PARM)
		TTYsdown = sdown_M;
	else if (SR)
		TTYsdown = sdown_S;
}
