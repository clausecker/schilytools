/* @(#)strsubs.h	1.8 03/11/23 Copyright 1995, 1999 J. Schilling */
/*
 *	A program to produce a static calltree for C-functions
 *
 *	string handling and allocation
 *
 *	Copyright (c) 1985, 1999 J. Schilling
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

extern	char	*xmalloc	__PR((unsigned int amt, char *txt));
extern	char	*concat		__PR((char *first, ...));
