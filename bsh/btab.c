/* @(#)btab.c	1.17 09/07/11 Copyright 1986-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)btab.c	1.17 09/07/11 Copyright 1986-2009 J. Schilling";
#endif
/*
 *	Copyright (c) 1986-2009 J. Schilling
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
#include "bsh.h"
#include "str.h"
#include "btab.h"

						/* Musz sortiert sein !!! */

	/*	name		argc	func to call	help text */
btab bitab[] = {
	{	"$",		1,	bfg,		ufg	},
	{	".",		0,	bsource,	udot	},
	{	":",		0,	bdummy,		NULL	},
	{	"@",		0,	bexpr,		uexpr	},
	{	"[",		0,	btest,		ubrack	},
	{	"alias",	0,	balias,		ualias	},
	{	"alloc",	0,	balloc,		NULL	},
	{	"bg",		0,	bfg,		ufg	},
	{	"break",	1,	bnallo,		NULL	},
	{	"case",		0,	bdummy,		NULL	},
	{	"cd",		0,	bcd,		ucd	},
	{	"chdir",	0,	bcd,		ucd	},
	{	"compute",	0,	bcompute,	utest	},
	{	"concat",	0,	bconcat,	uconcat	},
	{	"cwd",		0,	bcd,		ucd	},
	{	"dirs",		1,	bdirs,		nullstr	},
	{	"do",		0,	bdo,		udo	},
	{	"echo",		0,	becho,		uecho	},
	{	"else",		0,	bnallo,		NULL	},
	{	"end",		0,	bnallo,		NULL	},
	{	"env",		0,	benv,		uenv	},
	{	"err",		0,	becho,		uecho	},
	{	"errstr",	2,	berrstr,	uerrstr	},
	{	"eval",		0,	beval,		ueval	},
	{	"exec",		0,	bexec,		uexec	},
	{	"exit",		0,	bexit,		uexit	},
	{	"false",	0,	bfalse,		NULL	},
	{	"fg",		0,	bfg,		ufg	},
	{	"fi",		0,	bnallo,		NULL	},
#ifdef	DO_FIND
	{	"find",		0,	bfind,		(char *)-1 },
#endif
	{	"for",		0,	bfor,		NULL	},
	{	"function",	0,	bfunc,		ufunc	},
	{	"glob",		0,	becho,		uglob	},
	{	"history",	1,	bhistory,	nullstr	},
	{	"if",		0,	bif,		NULL	},
	{	"kill",		0,	bkill,		ukill	},
	{	"killpg",	0,	bkill,		ukill	},
	{	"limit",	0,	blimit,		uulimit	},
	{	"login",	0,	blogin,		ulogin	},
	{	"logout",	1,	blogout,	nullstr	},
	{	"loop",		1,	bloop,		NULL	},
	{	"map",		0,	bmap,		umap	},
	{	"pgrp",		0,	bpgrp,		upgrp	},
	{	"popd",		0,	bcd,		upopd	},
	{	"pushd",	0,	bcd,		upushd	},
	{	"pwd",		1,	bpwd,		nullstr	},
	{	"read",		2,	bread,		uread	},
	{	"remap",	1,	bremap,		nullstr	},
	{	"repeat",	0,	brepeat,	urepeat	},
	{	"resume",	2,	bresume,	uresume	},
	{	"return",	0,	breturn,	ureturn	},
#ifdef	INTERACTIVE
	{	"savehistory",	1,	bsavehist,	nullstr	},
#endif
	{	"set",		0,	bsetcmd,	uset	},
	{	"setenv",	0,	bsetenv,	usetenv	},
	{	"setmask",	0,	bsetmask,	usetmask},
	{	"shift",	0,	bshift,		ushift	},
	{	"signal",	0,	bsignal,	usignal	},
	{	"source",	0,	bsource,	usource	},
	{	"stop",		0,	bsuspend,	ustop	},
#ifdef	DO_SUID
	{	"suid",		0,	bsuid,		usuid	},
#endif
	{	"suspend",	0,	bsuspend,	ususpend},
	{	"switch",	3,	bswitch,	NULL	},
	{	"sync",		1,	bsync,		nullstr	},
	{	"test",		0,	btest,		utest	},
	{	"then",		0,	bnallo,		NULL	},
	{	"time_",	0,	btime,		uutime	},
	{	"true",		0,	btrue,		NULL	},
	{	"type",		0,	btype,		utype	},
	{	"umask",	0,	bumask,		uumask	},
	{	"unalias",	0,	bunalias,	uunalias},
	{	"unmap",	2,	bunmap,		uunmap	},
	{	"unset",	2,	bunset,		uunset	},
	{	"unsetenv",	2,	bunsetenv,	uunsetenv},
	{	"wait",		0,	bwait,		uwait	},
};

int	n_builtin	= sizeof (bitab) / sizeof (btab);
