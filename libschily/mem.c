/* @(#)mem.c	1.7 06/11/05 Copyright 1998-2006 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)mem.c	1.7 06/11/05 Copyright 1998-2006 J. Schilling";
#endif
/*
 *	Memory handling with error checking
 *
 *	Copyright (c) 1998-2006 J. Schilling
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
#include <stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

EXPORT	void	*__malloc	__PR((size_t size, char *msg));
EXPORT	void	*__realloc	__PR((void *ptr, size_t size, char *msg));
EXPORT	char	*__savestr	__PR((const char *s));

EXPORT void *
__malloc(size, msg)
	size_t	size;
	char	*msg;
{
	void	*ret;

	ret = malloc(size);
	if (ret == NULL) {
		comerr(gettext("Cannot allocate memory for %s.\n"), msg);
		/* NOTREACHED */
	}
	return (ret);
}

EXPORT void *
__realloc(ptr, size, msg)
	void	*ptr;
	size_t	size;
	char	*msg;
{
	void	*ret;

	if (ptr == NULL)
		ret = malloc(size);
	else
		ret = realloc(ptr, size);
	if (ret == NULL) {
		comerr(gettext("Cannot realloc memory for %s.\n"), msg);
		/* NOTREACHED */
	}
	return (ret);
}

EXPORT char *
__savestr(s)
	const char	*s;
{
	char	*ret = __malloc(strlen(s)+1, "saved string");

	strcpy(ret, s);
	return (ret);
}
