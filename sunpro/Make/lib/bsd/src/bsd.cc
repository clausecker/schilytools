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
/*
 * Copyright 2004 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)bsd.cc 1.6 06/12/12
 */

#pragma	ident	"@(#)bsd.cc	1.6	06/12/12"

/*
 * This file contains modifications Copyright 2017 J. Schilling
 *
 * @(#)bsd.cc	1.2 17/04/23 2017 J. Schilling
 */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)bsd.cc	1.2 17/04/23 2017 J. Schilling";
#endif

#include <signal.h>

#include <bsd/bsd.h>

/* External references.
 */

/* Forward references.
 */

/* Static data.
 */

extern SIG_PF
bsd_signal (int Signal, SIG_PF Handler)
{
  auto SIG_PF                   previous_handler;
#ifdef SUN5_0
#ifdef sun
  previous_handler = sigset (Signal, Handler);
#else
  auto struct sigaction         new_action;
  auto struct sigaction         old_action;

  new_action.sa_flags = SA_SIGINFO;
  new_action.sa_handler = Handler;
  (void) sigemptyset (&new_action.sa_mask);
  (void) sigaddset (&new_action.sa_mask, Signal);

  (void) sigaction (Signal, &new_action, &old_action);

  previous_handler = (SIG_PF) old_action.sa_handler;
#endif
#elif defined(linux)
  previous_handler = sigset (Signal, Handler);
#else
  previous_handler = signal (Signal, Handler);
#endif
  return previous_handler;
}

extern void
bsd_signals (void)
{
  static int                    initialized = 0;

  if (initialized == 0)
    {
      initialized = 1;
#if !defined(SUN5_0) && !defined(linux)
#if defined(SIGHUP)
      (void) bsd_signal (SIGHUP, SIG_DFL);
#endif
#if defined(SIGINT)
      (void) bsd_signal (SIGINT, SIG_DFL);
#endif
#if defined(SIGQUIT)
      (void) bsd_signal (SIGQUIT, SIG_DFL);
#endif
#if defined(SIGILL)
      (void) bsd_signal (SIGILL, SIG_DFL);
#endif
#if defined(SIGTRAP)
      (void) bsd_signal (SIGTRAP, SIG_DFL);
#endif
#if defined(SIGIOT)
      (void) bsd_signal (SIGIOT, SIG_DFL);
#endif
#if defined(SIGABRT)
      (void) bsd_signal (SIGABRT, SIG_DFL);
#endif
#if defined(SIGEMT)
      (void) bsd_signal (SIGEMT, SIG_DFL);
#endif
#if defined(SIGFPE)
      (void) bsd_signal (SIGFPE, SIG_DFL);
#endif
#if defined(SIGBUS)
      (void) bsd_signal (SIGBUS, SIG_DFL);
#endif
#if defined(SIGSEGV)
      (void) bsd_signal (SIGSEGV, SIG_DFL);
#endif
#if defined(SIGSYS)
      (void) bsd_signal (SIGSYS, SIG_DFL);
#endif
#if defined(SIGPIPE)
      (void) bsd_signal (SIGPIPE, SIG_DFL);
#endif
#if defined(SIGALRM)
      (void) bsd_signal (SIGALRM, SIG_DFL);
#endif
#if defined(SIGTERM)
      (void) bsd_signal (SIGTERM, SIG_DFL);
#endif
#if defined(SIGUSR1)
      (void) bsd_signal (SIGUSR1, SIG_DFL);
#endif
#if defined(SIGUSR2)
      (void) bsd_signal (SIGUSR2, SIG_DFL);
#endif
#if defined(SIGCLD)
      (void) bsd_signal (SIGCLD, SIG_DFL);
#endif
#if defined(SIGCHLD)
      (void) bsd_signal (SIGCHLD, SIG_DFL);
#endif
#if defined(SIGPWR)
      (void) bsd_signal (SIGPWR, SIG_DFL);
#endif
#if defined(SIGWINCH)
      (void) bsd_signal (SIGWINCH, SIG_DFL);
#endif
#if defined(SIGURG)
      (void) bsd_signal (SIGURG, SIG_DFL);
#endif
#if defined(SIGIO)
      (void) bsd_signal (SIGIO, SIG_DFL);
#else
#if defined(SIGPOLL)
      (void) bsd_signal (SIGPOLL, SIG_DFL);
#endif
#endif
#if defined(SIGTSTP)
      (void) bsd_signal (SIGTSTP, SIG_DFL);
#endif
#if defined(SIGCONT)
      (void) bsd_signal (SIGCONT, SIG_DFL);
#endif
#if defined(SIGTTIN)
      (void) bsd_signal (SIGTTIN, SIG_DFL);
#endif
#if defined(SIGTTOU)
      (void) bsd_signal (SIGTTOU, SIG_DFL);
#endif
#if defined(SIGVTALRM)
      (void) bsd_signal (SIGVTALRM, SIG_DFL);
#endif
#if defined(SIGPROF)
      (void) bsd_signal (SIGPROF, SIG_DFL);
#endif
#if defined(SIGXCPU)
      (void) bsd_signal (SIGXCPU, SIG_DFL);
#endif
#if defined(SIGXFSZ)
      (void) bsd_signal (SIGXFSZ, SIG_DFL);
#endif
#endif
    }

  return;
}
