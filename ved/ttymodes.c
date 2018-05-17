/* @(#)ttymodes.c	1.25 11/08/11 Copyright 1984-2011 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)ttymodes.c	1.25 11/08/11 Copyright 1984-2011 J. Schilling";
#endif
/*
 *	Terminal driver tty mode handling
 *
 *	Copyright (c) 1984-2011 J. Schilling
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
#include <schily/termios.h>

	int	intrchar = -1;
LOCAL	BOOL	got_modes = FALSE;

#ifndef tos
#ifdef	JOS
LOCAL	TTYMODE old = DEFAULT_TTY_MODES;
LOCAL	TTYMODE tty;
#else
#ifdef	USE_V7_TTY
LOCAL	struct sgttyb	old;
LOCAL	struct sgttyb	tty;
LOCAL	struct tchars	oldt;
LOCAL	struct tchars	ttyt;
LOCAL	struct ltchars	oldl;
LOCAL	struct ltchars	ttyl;
LOCAL	int		lbit;

#else	/* USE_V7_TTY */

#ifdef	USE_TERMIOS
LOCAL	struct termios	old;
LOCAL	struct termios	tty;
#endif

#endif	/* USE_V7_TTY */
#endif
#endif /* tos */

/*
 * From the tty modes...
 */
extern	char	GT;		/* Gtty told us that we may use tabs	*/
extern	char	NONL;		/* Terminal don't needs linefeed on CR	*/
extern	char	UPPERCASE;	/* Terminal is upper case only		*/
extern	short	ospeed;

EXPORT	void	get_modes	__PR((ewin_t *wp));
EXPORT	void	set_modes	__PR((void));
LOCAL	void	set_ttymodes	__PR((void));
EXPORT	void	set_oldmodes	__PR((void));

/*
 * Get old (pre editing) tty modes from driver
 */
EXPORT void
get_modes(wp)
	ewin_t	*wp;
{
#ifndef	tos
#ifdef	JOS
	spfun(1, GTTY, &old);
	if (old.t_i8bit == TRUE || old.t_o8bit == TRUE) wp->raw8 = TRUE;
#else
#	ifdef	USE_V7_TTY
		ioctl(STDIN_FILENO, TIOCGETP, &old);
		ospeed = old.sg_ospeed;
		if ((old.sg_flags & RAW)) wp->raw8 = TRUE;

		GT = (old.sg_flags & XTABS) != XTABS;
		NONL = (old.sg_flags & CRMOD) == 0;
		UPPERCASE = (old.sg_flags & LCASE) != 0;
#ifdef	LPASS8
		ioctl(STDIN_FILENO, TIOCLGET, &lbit);
		if (lbit & LPASS8)
			wp->raw8 = TRUE;
#endif
		ioctl(STDIN_FILENO, TIOCGETC, &oldt);
		intrchar = oldt.t_intrc;

		ioctl(STDIN_FILENO, TIOCGLTC, &oldl);

#	else	/* USE_V7_TTY */

#	ifdef	USE_TERMIOS
#		ifdef	TCSANOW
		tcgetattr(STDIN_FILENO, &old);
		ospeed = (short)cfgetospeed(&old);
#		else
		ioctl(STDIN_FILENO, TCGETS, &old);
#ifndef	CBAUD					/* FreeBSD */
		ospeed = old.c_ospeed;
#else
		ospeed = old.c_cflag & CBAUD;
#endif
#		endif
		if (!(old.c_iflag & ISTRIP) && (old.c_cflag & CSIZE) == CS8)
			wp->raw8 = TRUE;
		intrchar = old.c_cc[VINTR];

		GT = (old.c_oflag & XTABS) != XTABS;
#if	ONLCR == 0
		NONL = FALSE;
#else
		NONL = (old.c_oflag & ONLCR) == 0;
#endif
#ifndef	IUCLC
		UPPERCASE = FALSE;
#else
		UPPERCASE = (old.c_iflag & IUCLC) != 0;
#endif
#	endif	/* USE_TERMIOS */
#	endif	/* USE_V7_TTY */
#endif
#endif /* tos */
	got_modes = TRUE;
}

/*
 * Set-up editing tty modes and then set the tty driver to editing mode
 */
EXPORT void
set_modes()
{
#ifndef	tos
#ifdef	JOS
	movebytes((char *) &old, (char *) &tty, sizeof (TTYMODE));
	CLEAR_ECHO_MODES(tty);
	tty.t_wakealpha = TRUE;		tty.t_wakectl = TRUE;
	tty.t_escape = FALSE;		tty.t_irawedit = TRUE;
	tty.t_eof = FALSE;		tty.t_icrlf = FALSE;
	tty.t_prctl = FALSE;
	tty.t_octl = FALSE;		tty.t_ocrlf = FALSE;
	tty.t_oxtab = FALSE;
#else
#	ifdef	USE_V7_TTY
	movebytes((char *) &old, (char *) &tty, sizeof (struct sgttyb));
	movebytes((char *) &oldt, (char *) &ttyt, sizeof (struct tchars));
	tty.sg_flags &= ~(ECHO|CRMOD);
	tty.sg_flags |= CBREAK;
/*	ttyt.t_intrc = -1;*/
	ttyt.t_quitc = -1;
/*
	ttyt.t_startc = -1;
	ttyt.t_stopc = -1;
*/
	ttyt.t_eofc = -1;
	ttyt.t_brkc = -1;
	ttyl.t_suspc = -1;
	ttyl.t_dsuspc = -1;
	ttyl.t_rprntc = -1;
	ttyl.t_flushc = -1;
	ttyl.t_werasc = -1;
	ttyl.t_lnextc = -1;

#	else	/* USE_V7_TTY */

#	ifdef	USE_TERMIOS
	movebytes((char *) &old, (char *) &tty, sizeof (struct termios));
	tty.c_iflag |= (IXON|IGNBRK);
	tty.c_iflag &= ~(BRKINT|INLCR|ICRNL);
/*	tty.c_oflag |= 0;*/
	tty.c_oflag &= ~(ONLCR|OCRNL|ONLRET|XTABS);
	tty.c_lflag |= (ISIG);
	tty.c_lflag &= ~(ICANON|ECHO);
	/*
	 * Keep VINTR to allow to interrupt commands.
	 */
	tty.c_cc[VQUIT] = _POSIX_VDISABLE;
	tty.c_cc[VERASE] = _POSIX_VDISABLE;
	tty.c_cc[VKILL] = _POSIX_VDISABLE;
	/*
	 * On HP-UX, VMIN/VTIME are not mapped to VEOF/VEOL.
	 */
	tty.c_cc[VEOF] = _POSIX_VDISABLE;
	tty.c_cc[VEOL] = _POSIX_VDISABLE;
#ifdef	VEOL2
	tty.c_cc[VEOL2] = _POSIX_VDISABLE;
#endif
#ifdef	VSWTCH
	tty.c_cc[VSWTCH] = _POSIX_VDISABLE;
#endif
	/*
	 * Keep VSTART/VSTOP.
	 */
#ifdef	VSUSP
	tty.c_cc[VSUSP] = _POSIX_VDISABLE;
#endif
#ifdef	VDSUSP
	tty.c_cc[VDSUSP] = _POSIX_VDISABLE;
#endif
#ifdef	VREPRINT
	tty.c_cc[VREPRINT] = _POSIX_VDISABLE;
#endif
#ifdef	VDISCARD
	tty.c_cc[VDISCARD] = _POSIX_VDISABLE;
#endif
#ifdef	VWERASE
	tty.c_cc[VWERASE] = _POSIX_VDISABLE;
#endif
#ifdef	VLNEXT
	tty.c_cc[VLNEXT] = _POSIX_VDISABLE;
#endif
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;
#	endif	/* USE_TERMIOS */
#	endif	/* USE_V7_TTY */
#endif
#endif	/* tos */
	set_ttymodes();
}

/*
 * Set tty modes to editing tty modes
 */
LOCAL void
set_ttymodes()
{
#ifndef	tos
#ifdef	JOS
	spfun(1, STTY, &tty);
#else
#	ifdef	USE_V7_TTY
		ioctl(STDIN_FILENO, TIOCSETN, &tty);
		ioctl(STDIN_FILENO, TIOCSETC, &ttyt);
		ioctl(STDIN_FILENO, TIOCSLTC, &ttyl);
#	else	/* USE_V7_TTY */

#	ifdef	USE_TERMIOS
#		ifdef	TCSANOW
		tcsetattr(STDIN_FILENO, TCSADRAIN, &tty);
#		else
		ioctl(STDIN_FILENO, TCSETSW, &tty);
#		endif
#	endif	/* USE_TERMIOS */
#	endif	/* USE_V7_TTY */
#endif
#endif	/* tos */
}

/*
 * Re-set tty modes to saved old modes
 */
EXPORT void
set_oldmodes()
{
	if (!got_modes)
		return;
#ifndef	tos
#ifdef	JOS
	spfun(1, STTY, &old);
#else
#	ifdef	USE_V7_TTY
		ioctl(STDIN_FILENO, TIOCSETN, &old);
		ioctl(STDIN_FILENO, TIOCSETC, &oldt);
		ioctl(STDIN_FILENO, TIOCSLTC, &oldl);
#	else	/* USE_V7_TTY */

#	ifdef	USE_TERMIOS
#		ifdef	TCSANOW
		tcsetattr(STDIN_FILENO, TCSADRAIN, &old);
#		else
		ioctl(STDIN_FILENO, TCSETSW, &old);
#		endif
#	endif	/* USE_TERMIOS */
#	endif	/* USE_V7_TTY */
#endif
#endif	/* tos */
}
