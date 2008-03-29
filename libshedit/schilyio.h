/* @(#)schilyio.h	1.2 08/02/03 Copyright 2006-2008 J. Schilling */
/*
 *	Replacement for libschily/stdio/schilyio.h to allow
 *	FILE * -> int *
 *
 *	Copyright (c) 2006-2008 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/unistd.h>	/* include <sys/types.h> try to get size_t */
#include <schily/stdlib.h>	/* Try again for size_t	*/

#include <stdio.h>
#include <schily/varargs.h>
#include <schily/standard.h>
#include <schily/schily.h>

#define	down2(f, a, b)
