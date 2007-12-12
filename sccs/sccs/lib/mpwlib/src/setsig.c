/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * This file contains modifications Copyright 2006-2007 J. Schilling
 *
 * @(#)setsig.c	1.3 07/01/11 J. Schilling
 */
#if defined(sun) || defined(__GNUC__)

#ident "@(#)setsig.c 1.3 07/01/11 J. Schilling"
#endif
/*
 * @(#)setsig.c 1.8 06/12/12
 */

#ident	"@(#)setsig.c"
#ident	"@(#)sccs:lib/mpwlib/setsig.c"
# include       <defines.h>
# include       <i18n.h>
# include	<signal.h>

#define ONSIG	16

/*
	General-purpose signal setting routine.
	All non-ignored, non-caught signals are caught.
	If a signal other than hangup, interrupt, or quit is caught,
	a "user-oriented" message is printed on file descriptor 2 with
	a number for help(I).
	If hangup, interrupt or quit is caught, that signal	
	is set to ignore.
	Termination is like that of "fatal",
	via "clean_up(sig)" (sig is the signal number)
	and "exit(userexit(1))".
 
	If the file "dump.core" exists in the current directory
	the function commits
	suicide to produce a core dump
	(after calling clean_up, but before calling userexit).
*/


static char	*Mesg[ONSIG]={
	0,
	0,	/* Hangup */
	0,	/* Interrupt */
	0,	/* Quit */
	"Illegal instruction",
	"Trace/BPT trap",
	"IOT trap",
	"EMT trap",
	"Floating exception",
	"Killed",
	"Bus error",
	"Memory fault",
	"Bad system call",
	0,	/* Broken pipe */
	"Alarm clock"
};

static void setsig1	__PR((int sig));

void
setsig()
{
	register int j;
	register void (*n)();

	for (j=1; j<ONSIG; j++)
		if (j != SIGBUS)
			if ((n=signal(j,setsig1)) != NULL)
				signal(j,n);
}


static void
setsig1(sig)
int sig;
{
	static int die = 0;
	
	if (die++) {
#ifdef	SIGIOT
		signal(SIGIOT,0);
#endif
#ifdef	SIGEMT
		signal(SIGEMT,0);
#endif
#ifdef	SIGILL
		signal(SIGILL,0);
#endif
		exit(1);
	}
	if (Mesg[sig]) {
		(void) write(2, gettext("SIGNAL: "), length("SIGNAL: "));
		(void) write(2, gettext(Mesg[sig]), length(Mesg[sig]));
		(void) write(2, NOGETTEXT(" (ut12)\n"), length(" (ut12)\n"));
	}
	else
		signal(sig,SIG_IGN);
	clean_up();
	if(open(NOGETTEXT("dump.core"), O_RDONLY|O_BINARY) > 0) {
#ifdef	SIGIOT
		signal(SIGIOT,0);
#endif
#ifdef	SIGEMT
		signal(SIGEMT,0);
#endif
#ifdef	SIGILL
		signal(SIGILL,0);
#endif
		abort();
	}
	exit(userexit(1));
}
