/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
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

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#if defined(sun)
#pragma ident	"@(#)error.c	1.11	06/06/16 SMI"
#endif

#include "defs.h"

/*
 * Copyright 2008-2016 J. Schilling
 *
 * @(#)error.c	1.21 16/01/06 2008-2016 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)error.c	1.21 16/01/06 2008-2016 J. Schilling";
#endif

/*
 * UNIX shell
 */


/* ========	error handling	======== */

	void error	__PR((const char *s));
static void failed_body	__PR((unsigned char *s1, const char *s2,
				unsigned char *s3, int gflag));
	void failed_real __PR((int err, unsigned char *s1, const char *s2,
				unsigned char *s3));
	void failure_real __PR((int err, unsigned char *s1, const char *s2,
				unsigned char *s3,
				int gflag));
	void exvalsh	__PR((int xno));
	void exitsh	__PR((int xno));
#ifdef	DO_DOT_SH_PARAMS
	void rmtemp	__PR((struct ionod *base));
	void rmfunctmp	__PR((void));
#endif

/*
 * As error() finally calls exitsh(), it should only be called if scripts
 * need to be aborted as a result of the error to report.
 *
 * error() used to have the __NORETURN tag, but since we support
 * the "command" builtin, we need to be able to prevent the exit
 * on error (or longjmp to prompt) behavior via (flags & noexit) != 0.
 */
void
error(s)
	const char	*s;
{
	failed_real(ERROR, _gettext(s), NULL, NULL);
}

static void
failed_body(s1, s2, s3, gflag)
	unsigned char	*s1;
	const char	*s2;
	unsigned char	*s3;
	int	gflag;
{
	prp();
	if (gflag)
		prs(_gettext((const char *)s1));
	else
		prs_cntl(s1);
	if (s2) {
		prs((unsigned char *)colon);
		prs(_gettext(s2));
	}
	if (s3)
		prs(s3);
	newline();
}

/*
 * Called from "fatal errors", from locations where either a real exit() is
 * expected or from an interactive command where a longjmp() to the next prompt
 * is expected.
 *
 * failed_real() used to have the __NORETURN tag, but since we support
 * the "command" builtin, we need to be able to prevent the exit
 * on error (or longjmp to prompt) behavior via (flags & noexit) != 0.
 */
void
failed_real(err, s1, s2, s3)
	int		err;
	unsigned char	*s1;
	const char	*s2;
	unsigned char	*s3;
{
	failed_body(s1, s2, s3, 0);
#if !defined(NO_VFORK) || defined(DO_POSIX_SPEC_BLTIN)
	namscan(popval);
#endif
	if (!(flags & noexit))
		exitsh(err);

	exvalsh(err);
}

/*
 * A normal error that usually does not cause an exit() of the shell.
 * Except when "set -e" has been issued, we just set up $? and return.
 */
void
failure_real(err, s1, s2, s3, gflag)
	int		err;
	unsigned char	*s1;
	const char	*s2;
	unsigned char	*s3;
	int		gflag;
{
	failed_body(s1, s2, s3, gflag);

	if (flags & errflg)
		exitsh(err);

	exvalsh(err);
}

void
exvalsh(xno)
	int	xno;
{
	flags |= eflag;
	exitval = xno;
	exval_set(xno);
	exitset();
}

void
exitsh(xno)
	int	xno;
{
	/*
	 * Arrive here from `FATAL' errors
	 *  a) exit command,
	 *  b) default trap,
	 *  c) fault with no trap set.
	 *
	 * Action is to return to command level or exit.
	 */
	exvalsh(xno);
	if ((flags & (forcexit | forked | errflg | ttyflg)) != ttyflg) {
		/*
		 * If not from a "tty" or when special flags are set,
		 * do a real exit().
		 */
		done(0);
	} else {
		/*
		 * The standard error case from "tty" causes a longjmp()
		 * to the next prompt.
		 */
		clearup();
		restore(0);
		(void) setb(STDOUT_FILENO);
		execbrk = breakcnt = funcnt = 0;
		longjmp(errshell, 1);
	}
}

#ifdef	DO_DOT_SH_PARAMS
void
exval_clear()
{
	ex.ex_code = 0;
	ex.ex_status = 0;
	ex.ex_pid = mypid;
	ex.ex_signo = 0;
}

void
exval_sig()
{
	ex.ex_code = CLD_KILLED;
	ex.ex_status = trapsig;
	ex.ex_pid = mypid;
	ex.ex_signo = 0;
}

void
exval_set(xno)
	int	xno;
{
	if (ex.ex_code == CLD_EXITED && xno != ex.ex_status) {
		ex.ex_status = xno;
		return;
	} else if (ex.ex_code) {
		return;
	}
	ex.ex_code = CLD_EXITED;
	ex.ex_status = xno;
	ex.ex_pid = mypid;
	ex.ex_signo = 0;
}
#endif

/*
 * Previous sbrk() based versions of the Bourne Shell fed this function
 * with an aproximate address from the stak and used:
 * "while (iotemp > base)".
 *
 * This version of the shell is fed by the previous real value of "iotemp"
 * and for this reason we may safely use "while (iotemp != base)" and no
 * longer depend on memory order.
 */
void
rmtemp(base)
	struct ionod	*base;
{
	while (iotemp && iotemp != base) {
		unlink(iotemp->ioname);
		free(iotemp->iolink);
		iotemp = iotemp->iolst;
	}
}

void
rmfunctmp()
{
	while (fiotemp) {
		unlink(fiotemp->ioname);
		fiotemp = fiotemp->iolst;
	}
}
