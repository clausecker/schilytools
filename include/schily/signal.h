/* @(#)signal.h	1.7 06/09/13 Copyright 1997 J. Schilling */
/*
 *	Signal abstraction for BSD/SVR4 signals
 *
 *	Copyright (c) 1997 J. Schilling
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

#ifndef	_SCHILY_SIGNAL_H
#define	_SCHILY_SIGNAL_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_SIGSET
/*
 * Try to by default use the function that sets up signal handlers in a way
 * that does not reset the handler after it has been called.
 */
#define	signal		sigset
#endif

#ifdef	HAVE_SIGPROCMASK
#define	block_sigs(a)	{ \
				sigset_t	__new;	\
							\
				sigfillset(&__new);	\
				sigprocmask(SIG_BLOCK, &__new, &a);\
			}
#define	unblock_sig(s)	{ \
				sigset_t	__new;	\
							\
				sigemptyset(&__new);	\
				sigaddset(&__new, (s));	\
				sigprocmask(SIG_UNBLOCK, &__new, NULL);\
			}
#define	restore_sigs(a)	sigprocmask(SIG_SETMASK, &a, 0);

#else	/* !HAVE_SIGPROCMASK */

#define	sigset_t	int
#define	block_sigs(a)	a = sigblock(0xFFFFFFFF)
#define	restore_sigs(a)	sigsetmask(a);
#define	unblock_sig(s)	{ \
				int	__old, __new;	\
							\
				block_sigs(__old);	\
				__new = sigmask(s);	\
				__new = __old & ~__new;	\
				sigsetmask(__new);	\
			}

#endif	/* HAVE_SIGPROCMASK */

#endif	/* _SCHILY_SIGNAL_H */
