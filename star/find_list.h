/* @(#)find_list.h	1.7 07/04/04 Copyright 2004-2007 J. Schilling */
/*
 *	Copyright (c) 2004-2007 J. Schilling
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

#ifndef	_LIST_H
#define	_LIST_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef _INCL_STDIO_H
#include <stdio.h>
#define	_INCL_STDIO_H
#endif

extern	void	find_list		__PR((FILE *std[3], struct stat *fs, char *name, char *sname, struct WALK *state));

#endif	/* _LIST_H */
