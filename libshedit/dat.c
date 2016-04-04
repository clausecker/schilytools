/* @(#)dat.c	1.4 16/03/29 Copyright 2006-2013 J. Schilling */
/*
 *	Global data
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

#include <schily/mconfig.h>
#include <schily/unistd.h>	/* STDIN_FILENO */

#include <schily/stdio.h>

int	__in__	= STDIN_FILENO;
int	__out__	= STDOUT_FILENO;
int	__err__	= STDERR_FILENO;

FILE	*gstd[3];
char    **evarray;

int	delim;
int	ctlc;
int	ex_status;
int	ttyflg = 1;
int	prflg = 1;
int	prompt;
char	*prompts[2] = { "prompt1 > ", "prompt2 > " };
char	*inithome = ".";

char	*(*__get_env)	__PR((char *name));
void	(*__put_env)	__PR((char *name));
