/* @(#)ttymodes.c	1.31 09/07/11 Copyright 1986,1995-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)ttymodes.c	1.31 09/07/11 Copyright 1986,1995-2009 J. Schilling";
#endif
/*
 *	ttymodes.c
 *
 *	Terminal handling for bsh
 *
 *	Copyright (c) 1986,1995-2009 J. Schilling
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

#include <schily/stdio.h>
#include <schily/unistd.h>
#include <schily/termios.h>

#include "bsh.h"

#ifdef INTERACTIVE

#define	OLD_MODE	1
#define	INS_MODE	2
#define	APP_MODE	3

extern	pid_t	mypgrp;

	BOOL	ins_mode	= FALSE;
	BOOL	i_should_echo	= FALSE;

LOCAL	BOOL	cmdmodes	= FALSE;


LOCAL	BOOL	tty_init	= FALSE;

#	ifdef	USE_V7_TTY

LOCAL	struct sgttyb	ins	= {0};
LOCAL	struct sgttyb	app	= {0};
LOCAL	struct sgttyb	old	= {0};
LOCAL	struct tchars	oldt	= {0};
LOCAL	struct tchars	inst	= {0};
LOCAL	struct tchars	appt	= {0};
#ifdef	TIOCSLTC
LOCAL	struct ltchars	oldl	= {0};
LOCAL	struct ltchars	insl	= {0};
LOCAL	struct ltchars	appl	= {0};
#endif

#ifdef	NTTYDISC
LOCAL	int	olddisc	= -1;
LOCAL	int	disc	= NTTYDISC;
#endif	/* NTTYDISC */

#	else	/* USE_V7_TTY */

LOCAL	struct termios	ins	= {0};
LOCAL	struct termios	app	= {0};
LOCAL	struct termios	old	= {0};

#	endif	/* USE_V7_TTY */

#ifdef	JOBCONTROL
LOCAL	pid_t	oldpgrp	= 0;
#endif	/* JOBCONTROL */

EXPORT	void	reset_line_disc		__PR((void));
EXPORT	void	reset_tty_pgrp		__PR((void));
EXPORT	void	reset_tty_modes		__PR((void));
EXPORT	void	set_append_modes	__PR((FILE * f));
EXPORT	void	set_insert_modes	__PR((FILE * f));
#ifdef	JOBCONTROL
LOCAL	void	init_tty_pgrp		__PR((void));
#endif
EXPORT	void	get_tty_modes		__PR((FILE * f));
LOCAL	void	init_tty_modes		__PR((void));
LOCAL	void	set_tty_modes		__PR((FILE * f, int mode));
EXPORT	pid_t	tty_getpgrp		__PR((int f));
EXPORT	int	tty_setpgrp		__PR((int f, pid_t pgrp));

EXPORT void
reset_line_disc()
{
#if	defined(USE_V7_TTY) && defined(NTTYDISC)
	FILE	*f;

	if ((f = getinfile()) && olddisc != NTTYDISC)
		ioctl(fdown(f), TIOCSETD, (char *) &olddisc);

#endif	/* defined(USE_V7_TTY) && defined(NTTYDISC) */
}

EXPORT void
reset_tty_pgrp()
{
#ifdef	JOBCONTROL
	FILE	*f;

	if ((f = getinfile()) != 0 && oldpgrp != 0)
		tty_setpgrp(fdown(f), oldpgrp);
#endif	/* JOBCONTROL */
}

EXPORT void
reset_tty_modes()
{
#if	defined(USE_V7_TTY) && defined(NTTYDISC)

	if (olddisc < 0)
		init_line_disc();

#endif	/* defined(USE_V7_TTY) && defined(NTTYDISC) */

	set_tty_modes(getinfile(), OLD_MODE);
	cmdmodes = FALSE;
}

EXPORT void
set_append_modes(f)
	FILE	*f;
{
	if (ins_mode) {
		set_tty_modes(f, APP_MODE);
		i_should_echo	= TRUE;
		ins_mode	= FALSE;
		cmdmodes	= TRUE;
	}
}

EXPORT void
set_insert_modes(f)
	FILE	*f;
{
	if (!ins_mode) {
		set_tty_modes(f, INS_MODE);
		ins_mode = TRUE;
		cmdmodes = TRUE;
	}
}

/*
 *	Beim Umschalten der Linedisziplin geht der Input verloren.
 *	Um nach dem Starten des 'bsh' jederzeit Kommandos eingeben zu
 *	koennen wird nur einmal vor dem Starten des ersten
 *	interaktven Kommandos auf die neue Linedisziplin umgeschaltet.
 *	Dies geschieht in 'reset_tty_modes()'.
 *	Die erste Zeile wird mit der davor aktiven Disziplin editiert.
 *	Da die neue Linedisziplin nur fuer die Prozesskontrolle von
 *	laufenden Kommandos benoetigt wird, ist das kein Fehler.
 */
#if	defined(USE_V7_TTY) && defined(NTTYDISC)
EXPORT void
init_line_disc()
{
	int	fno;
	FILE	*f;

	if (f = getinfile()) {
		fno = fdown(f);
		ioctl(fno, TIOCGETD, (char *)&olddisc);
		if (olddisc != NTTYDISC)
			ioctl(fno, TIOCSETD, (char *)&disc);
	}
}
#endif	/* defined(USE_V7_TTY) && defined(NTTYDISC) */

#ifdef	JOBCONTROL
LOCAL void
init_tty_pgrp()
{
	int	fno;
	FILE	*f;

	if ((f = getinfile()) != NULL) {
		fno = fdown(f);
		oldpgrp = tty_getpgrp(fno);
		tty_setpgrp(fno, mypgrp);
	}
}
#endif	/* JOBCONTROL */

EXPORT void
get_tty_modes(f)
	FILE	*f;
{
	if (!cmdmodes) {
#ifdef	USE_V7_TTY
		ioctl(fdown(f), TIOCGETP, (char *) &old);
		ins.sg_ispeed = old.sg_ispeed;
		ins.sg_ospeed = old.sg_ospeed;
		app.sg_ispeed = old.sg_ispeed;
		app.sg_ospeed = old.sg_ospeed;
		ins.sg_flags = old.sg_flags;
		app.sg_flags = old.sg_flags;
		ins.sg_flags |= (CBREAK|CRMOD);
		ins.sg_flags &= ~(ECHO|RAW);
		app.sg_flags |= (CBREAK|CRMOD);
		app.sg_flags &= ~(ECHO|RAW);
		ioctl(fdown(f), TIOCGETC, (char *)&oldt);
		inst.t_startc	= oldt.t_startc;
		inst.t_stopc	= oldt.t_stopc;
		appt.t_startc	= oldt.t_startc;
		appt.t_stopc	= oldt.t_stopc;
#		ifdef	TIOCSLTC
		ioctl(fdown(f), TIOCGLTC, (char *)&oldl);
#		endif
#else	/* USE_V7_TTY */
#	ifdef	TCSANOW
		tcgetattr(fdown(f), &old);
#	else
		ioctl(fdown(f), TCGETS, (char *)&old);
#	endif

		app.c_iflag = ins.c_iflag = old.c_iflag;
		app.c_oflag = ins.c_oflag = old.c_oflag;
		app.c_cflag = ins.c_cflag = old.c_cflag;
		app.c_lflag = ins.c_lflag = old.c_lflag;

		ins.c_iflag |= (IGNBRK);
		ins.c_iflag &= ~(BRKINT|INLCR|ICRNL);
		ins.c_oflag |= (OPOST);
		ins.c_lflag &= ~(ISIG|ICANON|ECHO);

		app.c_iflag |= (IGNBRK);
		app.c_iflag &= ~(BRKINT|INLCR|ICRNL);
		app.c_oflag |= (OPOST);
		app.c_lflag &= ~(ISIG|ICANON|ECHO);
#endif	/* USE_V7_TTY */
		if (!tty_init) {
			init_tty_modes();
#ifdef	JOBCONTROL
			init_tty_pgrp();
#endif	/* JOBCONTROL */
			tty_init = TRUE;
		}
	}
}


LOCAL void
init_tty_modes()
{
#ifdef	USE_V7_TTY
	movebytes((char *) &old, (char *) &app, sizeof (old));
	movebytes((char *) &old, (char *) &ins, sizeof (old));
	ins.sg_erase	= -1;
	ins.sg_kill	= -1;
	app.sg_erase	= -1;
	app.sg_kill	= -1;
	ins.sg_flags |= (CBREAK|CRMOD);
	ins.sg_flags &= ~(ECHO|RAW);
	app.sg_flags |= (CBREAK|CRMOD);
	app.sg_flags &= ~(ECHO|RAW);
	movebytes((char *)&oldt, (char *)&inst, sizeof (inst));
	inst.t_intrc	= -1;
	inst.t_quitc	= -1;
#ifdef	__never__
	Auf keinen Fall !!!
	inst.t_startc	= -1;
	inst.t_stopc	= -1;
#endif
	inst.t_eofc	= -1;
	inst.t_brkc	= -1;
#	ifdef	TIOCSLTC
	insl.t_suspc	= -1;
	insl.t_dsuspc	= -1;
	insl.t_rprntc	= -1;
	insl.t_flushc	= -1;
	insl.t_werasc	= -1;
	insl.t_lnextc	= -1;
	movebytes((char *)&insl, (char *)&appl, sizeof (insl));
#	endif
	movebytes((char *)&inst, (char *)&appt, sizeof (inst));
#else
	movebytes((char *)&old, (char *)&app, sizeof (old));
	movebytes((char *)&old, (char *)&ins, sizeof (old));

	ins.c_iflag |= (IGNBRK);
	ins.c_iflag &= ~(BRKINT|INLCR|ICRNL);
	ins.c_oflag |= (OPOST);
	ins.c_lflag &= ~(ISIG|ICANON|ECHO);
	ins.c_cc[VMIN] = 1;
	ins.c_cc[VTIME] = 0;
	app.c_iflag |= (IGNBRK);
	app.c_iflag &= ~(BRKINT|INLCR|ICRNL);
	app.c_oflag |= (OPOST);
	app.c_lflag &= ~(ISIG|ICANON|ECHO);
	app.c_cc[VMIN] = 1;
	app.c_cc[VTIME] = 0;
#endif	/* USE_V7_TTY */
}


LOCAL void
set_tty_modes(f, mode)
	FILE	*f;
	int	mode;
{
	int	fno;

	if (f == NULL)
		return;

	fno = fdown(f);

	switch (mode) {

#ifdef	USE_V7_TTY
	case	OLD_MODE:
			ioctl(fno, TIOCSETN, (char *)&old);
			ioctl(fno, TIOCSETC, (char *)&oldt);
#			ifdef	TIOCSLTC
				ioctl(fno, TIOCSLTC, (char *)&oldl);
#			endif
			break;
	case	APP_MODE:
			ioctl(fno, TIOCSETN, (char *)&app);
			ioctl(fno, TIOCSETC, (char *)&appt);
#			ifdef	TIOCSLTC
				ioctl(fno, TIOCSLTC, (char *)&appl);
#			endif
			break;
	case	INS_MODE:
			ioctl(fno, TIOCSETN, (char *)&ins);
			ioctl(fno, TIOCSETC, (char *)&inst);
#			ifdef	TIOCSLTC
				ioctl(fno, TIOCSLTC, (char *)&insl);
#			endif
			break;
#else
#	ifdef	TCSANOW

	case	OLD_MODE:
			tcsetattr(fno, TCSADRAIN, &old);
			break;
	case	APP_MODE:
			tcsetattr(fno, TCSADRAIN, &app);
			break;
	case	INS_MODE:
			tcsetattr(fno, TCSADRAIN, &ins);
			break;
#	else

	case	OLD_MODE:
			ioctl(fno, TCSETSW, (char *)&old);
			break;
	case	APP_MODE:
			ioctl(fno, TCSETSW, (char *)&app);
			break;
	case	INS_MODE:
			ioctl(fno, TCSETSW, (char *)&ins);
			break;
#endif
#endif	/* USE_V7_TTY */
	}
}

#endif /* INTERACTIVE */

#ifdef	JOBCONTROL
EXPORT pid_t
tty_getpgrp(f)
	int	f;
{
#ifdef	HAVE_TCGETPGRP
	return (tcgetpgrp(f));
#else
#ifdef	TIOCGPGRP
	pid_t	pgrp;

	if (ioctl(f, TIOCGPGRP, (char *)&pgrp) < 0)
		return ((pid_t)-1);
	return (pgrp);
#else
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return ((pid_t)-1);
#endif
#endif	/* HAVE_TCGETPGRP */
}

#ifdef	PROTOTYPES
EXPORT int
tty_setpgrp(int f, pid_t pgrp)
#else
EXPORT int
tty_setpgrp(f, pgrp)
	int	f;
	pid_t	pgrp;
#endif
{
#ifdef	HAVE_TCGETPGRP
	return (tcsetpgrp(f, pgrp));
#else
#ifdef	TIOCSPGRP
	return (ioctl(f, TIOCSPGRP, (char *) &pgrp));
#else
	return (0);
#endif
#endif	/* HAVE_TCSETPGRP */
}
#else
EXPORT pid_t
tty_getpgrp(f)
	int	f;
{
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return ((pid_t)-1);
}

EXPORT int
tty_setpgrp(f, pgrp)
	int	f;
	pid_t	pgrp;
{
	return (0);
}
#endif

