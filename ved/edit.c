/* @(#)edit.c	1.27 18/08/23 Copyright 1984-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)edit.c	1.27 18/08/23 Copyright 1984-2018 J. Schilling";
#endif
/*
 *	Main editing loop of VED (Visual EDitor)
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

#include "ved.h"
#include <schily/signal.h>
#include <schily/setjmp.h>
#include <schily/jmpdefs.h>

extern	void	(*chartab[NCTAB][0200])	__PR((ewin_t *));
extern	int	ctabidx;	/* The table idx where we take commands from */

LOCAL	sigjmp_buf	jmp;
extern	sigjmps_t	*sjp;

extern	long	charstyped;

LOCAL	void	intr		__PR((int sig));
EXPORT	void	edit		__PR((ewin_t *wp));

/* ARGSUSED */
LOCAL void
intr(sig)
	int	sig;
{
	signal(SIGINT, intr);
	if (sjp)
		siglongjmp(sjp->jb, TRUE);
	else
		siglongjmp(jmp, TRUE);
}

EXPORT void
edit(wp)
	ewin_t	*wp;
{
	register void	(*f)			__PR((ewin_t *));
	register echar_t c;
	register Uchar	cc;		/* Used to index the binding arrays */
	register epos_t sdot;
		BOOL	in_command = FALSE;

	extern	 int	intrchar;
	extern	 BOOL	interrupted;

	vedstartstats();

	sdot = wp->dot;
	if (sigsetjmp(jmp, 1)) {
		if (in_command) {
/*			dot = sdot;*/
			update(wp);
			writemsg(wp, C NULL);
			writenum(wp, wp->curnum = wp->number = 1L);
			abortmsg(wp);
			flush();
		} else {
			if (intrchar > 0)
				interrupted++;
		}
	} else {
		signal(SIGINT, intr);
	}
	for (;;) {
		sdot = wp->dot;
		in_command = FALSE;
		c = gchar(wp);
		charstyped++;
/*cdbg("got: '%c' (%d)", c, c);*/
		in_command = TRUE;
		cc = c & 0xFF;
		wp->lastch = c;

		/*
		 * Choose the right function to call for the current character.
		 * Refresh the system line or parts of it if needed.
		 */
		f = chartab[ctabidx][cc];
		ctabidx = CTAB;
		refreshsysline(wp);

		/*
		 * Call the function bound to the current character and update
		 * the display if needed.
		 * esccmd/altcmd/altesccmd may be set here.
		 */
		(*f)(wp);
		update(wp);

		/*
		 * We want to keep the cursor on the same column if possible
		 * but shorter lines may force us to move the cursor to the
		 * left. To achieve this, we keep a remembered copy of the
		 * actual column that is not updated on corsur up/down
		 * movement.
		 */
		if ((wp->eflags & COLUPDATE) != 0)
			wp->column = cursor.hp;
		else
			wp->eflags |= COLUPDATE;

		/*
		 * Make a curnum copy of number to allow anyone to modify it.
		 * Reset number to 1 except when the last character changed
		 * only the state or the last command requested us to save it.
		 */
		wp->curnum = wp->number;
		if (wp->number != 1 && ctabidx == CTAB) {
			if ((wp->eflags & SAVENUM) != 0) {
				wp->eflags &= ~SAVENUM;
			} else {
				writenum(wp, wp->curnum = wp->number = 1L);
			}
		}

		/*
		 * If KEEPDEL is not set the next delete operation will remove
		 * the current content of the delete/rubout buffer first.
		 * A delete operation will set DELDONE. We transform this
		 * into KEEPDEL if the current command did not only change
		 * the state. If the last command did not move the cursor
		 * keep the deletions too.
		 */
		if ((wp->eflags & KEEPDEL) != 0 && sdot == wp->dot)
			wp->eflags |= DELDONE;

		if ((wp->eflags & (KEEPDEL|DELDONE)) != 0 && ctabidx == CTAB) {
			wp->eflags &= ~KEEPDEL;

			if ((wp->eflags & DELDONE) != 0)
				wp->eflags |= KEEPDEL;

			wp->eflags &= ~DELDONE;
		}
		/*
		 * Usually, all deletions are saved into the delete/rubout
		 * buffer. However take operations have their own buffer.
		 * Set SAVEDEL here to choose standard behavior, take routines
		 * will clear it later to prevent wasting the delete buffer.
		 */
		wp->eflags |= SAVEDEL;

		/*
		 * Now write out everything that is in the screen buffer.
		 */
		flush();
/*		writeerr("wp, eflags: 0x%X", wp->eflags);*/
	}
}
