/* @(#)eaccess.c	1.4 10/05/08 Copyright 2004-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)eaccess.c	1.4 10/05/08 Copyright 2004-2010 J. Schilling";
#endif
/*
 * Implement the best possible emulation for eaccess()
 *
 * Copyright 2004-2010 J. Schilling
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

#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/schily.h>

#ifndef	HAVE_EACCESS
EXPORT	int	eaccess		__PR((const char *name, int mode));

EXPORT int
eaccess(name, mode)
	const	char	*name;
		int	mode;
{
#ifdef	HAVE_EUIDACCESS
	return (euidaccess(name, mode));
#else
#ifdef	HAVE_ACCESS_E_OK
	return (access(name, E_OK|mode));
#else
	return (access(name, mode));
#endif
#endif
}
#endif
