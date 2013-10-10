/* @(#)str.c	1.6 13/09/25 Copyright 2006-2013 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)str.c	1.6 13/09/25 Copyright 2006-2013 J. Schilling";
#endif
/*
 *	Some selected strings from bsh needed by the command line editor.
 *
 *	Copyright (c) 2006-2013 J. Schilling
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

#include <schily/stdio.h>

char	mapname[]	= ".bshmap";
char	historyname[]	= ".history";

char	histname[]	= "HISTORY";
char	ignoreeofname[]	= "IGNOREEOF";
char	termname[]	= "TERM";
char	termcapname[]	= "TERMCAP";

char	ecantopen[]	= "Can't open '%s'. %s";
char	enotfound[]	= "Not found.";
char	ebadpattern[]	= "Bad pattern.";

char	for_read[]	= "r";
char	for_wct[]	= "wct";

char	nullstr[]	= "";
char	slash[]		= "/";
char	eql[]		= "=";
char	on[]		= "on";

/*
 *	Conditions
 */
char	sn_no_mem[]	= "no_memory";

/*
 *	Parser
 */
char	special[]	= ";&|()<>% \t";
