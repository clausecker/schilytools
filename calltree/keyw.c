/* @(#)keyw.c	1.14 09/07/11 Copyright 1985, 1999-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)keyw.c	1.14 09/07/11 Copyright 1985, 1999-2009 J. Schilling";
#endif
/*
 *	A program to produce a static calltree for C-functions
 *
 *	C langugage key words
 *
 *	Copyright (c) 1985, 1999-2009 J. Schilling
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
#include <schily/standard.h>
#include "sym.h"
#include "clex.h"

EXPORT	void	initkeyw	__PR((void));
EXPORT	BOOL	keyword		__PR((char * name));

LOCAL	sym_t	*keywords = NULL;

LOCAL	char *keywlist[] = {
		"char", "double", "enum", "float", "int", "long", "short",
		"struct", "union", "void",

		"auto", "const", "extern", "register", "signed", "static",
		"unsigned", "volatile",

		"break", "case", "continue", "default", "do", "else","for",
		"goto", "if", "return", "sizeof", "switch", "typedef", "while",

		0
};

EXPORT void
initkeyw()
{
	register char	**np = keywlist;

	while (*np != 0)
		lookup(*np++, &keywords, L_CREATE);
}

EXPORT BOOL
keyword(name)
	char	*name;
{
	return (lookup(name, &keywords, L_LOOK) != NULL);
}
