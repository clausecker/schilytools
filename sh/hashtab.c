/* @(#)hashtab.c	1.2 15/08/08 Copyright 1986-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)hashtab.c	1.2 15/08/08 Copyright 1986-2015 J. Schilling";
#endif
/*
 *	Copyright (c) 1986-2015 J. Schilling
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

#ifdef	DO_HASHCMDS

#include "defs.h"
#include "hashtab.h"

struct abbtab abbtab[] = {
	{ 'a', "#a[g|l]\tname value - add new alias" },
	{ 'b', "#b[g|l]\tname value - add new begin alias" },
	{ 'd', "#d[g|l]\tname - delete alias" },
	{ 'h', "#h\tdisplay complete # help" },
	{ '?', "#?\tdisplay complete # help" },
	{ 'l', "#l[g|l][h] [name] - list aliases" },
	{ 'p', "#p[g|l][a|b] name value - push alias definition" },
	{ 's', "#s[g|l]\tset/display default table for # commands" },
	{ ' ', "# \tcomment in commandfiles" },
	{ '\0', NULL }
};
#endif	/* DO_HASHCMDS */
