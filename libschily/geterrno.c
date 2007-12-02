/* @(#)geterrno.c	1.11 06/09/26 Copyright 1985, 1995-2003 J. Schilling */
/*
 *	Get error number
 *
 *	Copyright (c) 1985, 1995-2003 J. Schilling
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

#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/schily.h>

#ifdef	geterrno
#undef	geterrno
#endif

EXPORT int
geterrno()

{
	return (errno);
}
