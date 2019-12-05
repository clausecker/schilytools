/* @(#)schilyio.h	1.5 19/12/04 Copyright 2006-2019 J. Schilling */
/*
 *	Replacement for libschily/stdio/schilyio.h to allow
 *	FILE * -> int *
 *
 *	Copyright (c) 2006-2019 J. Schilling
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
#include <schily/unistd.h>	/* include <sys/types.h> try to get size_t */
#include <schily/stdlib.h>	/* Try again for size_t	*/

#undef	FAST_GETC_PUTC
#include <mystdio.h>
#include <schily/varargs.h>
#include <schily/standard.h>
#include <schily/schily.h>

#define	down2(f, a, b)
