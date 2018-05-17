/* @(#)btab.h	1.11 08/03/27 Copyright 1986-2008 J. Schilling */
/*
 *	Copyright (c) 1986-2008 J. Schilling
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

typedef struct {
	char	*b_name;
	int	b_argc;
	void	(*b_func) __PR((Argvec * vp, FILE ** std, int flag));
	char	*b_help;
} btab;

extern btab	bitab[];
extern int	n_builtin;

/*
 * alias.c
 */
extern	void	balias		__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bunalias	__PR((Argvec * vp, FILE ** std, int flag));

/*
 * builtin.c
 */
extern	void	wrong_args	__PR((Argvec * vp, FILE ** std));
extern	void	unimplemented	__PR((Argvec * vp, FILE ** std));
extern	BOOL	busage	__PR((Argvec * vp, FILE ** std));
extern	BOOL	toint	__PR((FILE **std, char *s, int *i));
extern	void	bnallo	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bdummy	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bsetcmd	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bunset	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bsetenv	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bunsetenv	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bconcat	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bmap	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bunmap	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bexit	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	beval	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bdo	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	benv	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bwait	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bkill	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bsuspend	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bresume	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bfg	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bpgrp	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bsync	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bumask	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bsetmask	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	blogout	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	becho	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bshift	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bremap	__PR((Argvec * vp, FILE ** std, int flag));
#ifdef	INTERACTIVE
extern	void	bsavehist	__PR((Argvec * vp, FILE ** std, int flag));
#endif
extern	void	bhistory	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bsource	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	brepeat	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bexec	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	blogin	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	berrstr	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	btype	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	btrue	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bfalse	__PR((Argvec * vp, FILE ** std, int flag));
#ifdef	DO_FIND
extern	void	bfind	__PR((Argvec * vp, FILE ** std, int flag));
#endif
extern	BOOL	builtin	__PR((Argvec * vp, FILE ** std, int flag));

/*
 * call.c
 */
extern	void	bsignal		__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bfunc		__PR((Argvec * vp, FILE ** std, int flag));
extern	void	breturn		__PR((Argvec * vp, FILE ** std, int flag));

/*
 * cond.c
 */
extern	void	bif		__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bfor		__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bloop		__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bread		__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bswitch		__PR((Argvec * vp, FILE ** std, int flag));

/*
 * dirs.c
 */
extern	void	bpwd		__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bdirs		__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bcd		__PR((Argvec * vp, FILE ** std, int flag));

/*
 * test.c
 */
extern	void	bcompute	__PR((Argvec * vp, FILE ** std, int flag));
extern	void	btest		__PR((Argvec * vp, FILE ** std, int flag));
extern	void	bexpr		__PR((Argvec * vp, FILE ** std, int flag));

/*
 * idops.c
 */
#ifdef	DO_SUID
extern	void	bsuid		__PR((Argvec * vp, FILE ** std, int flag));
#endif

/*
 * limit.c
 */
extern	void	blimit		__PR((Argvec * vp, FILE ** std, int flag));
extern	void	btime		__PR((Argvec * vp, FILE ** std, int flag));
