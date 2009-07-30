/* @(#)abbtab.c	1.7 09/07/11 Copyright 1986-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)abbtab.c	1.7 09/07/11 Copyright 1986-2009 J. Schilling";
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
#include "abbtab.h"

struct abbtab abbtab[] = {
	{ 'a', "#a[g|l]\tname value - add new abbrev" },
	{ 'b', "#b[g|l]\tname value - add new begin abbrev" },
	{ 'd', "#d[g|l]\tname - delete abbrev" },
	{ 'h', "#h\tdisplay complete # help" },
	{ '?', "#?\tdisplay complete # help" },
	{ 'l', "#l[g|l][h] [name] - list abbrevs" },
	{ 'p', "#p[g|l][a|b] name value - push abbrev definition" },
	{ 's', "#s[g|l]\tset/display default table for # commands" },
	{ 'v', "#v[on|off] set/display verbose mode" },
	{ '!', "#!\tshell [args] - execute cmdfile with alternate shell" },
	{ ' ', "# \tcomment in commandfiles" },
	{ '\0', NULL }
};
