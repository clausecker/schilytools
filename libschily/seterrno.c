/* @(#)seterrno.c	1.10 06/09/26 Copyright 1985, 1995-2003 J. Schilling */
/*
 *	Set error number
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

#include <schily/standard.h>
#include <schily/errno.h>
#include <schily/schily.h>

#ifdef	seterrno
#undef	seterrno
#endif

EXPORT int
seterrno(err)
	int	err;
{
	int	old = errno;

	errno = err;
	return (old);
}
