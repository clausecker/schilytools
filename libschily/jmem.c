/* @(#)jmem.c	1.10 07/04/03 Copyright 1998-2007 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)jmem.c	1.10 07/04/03 Copyright 1998-2007 J. Schilling";
#endif
/*
 *	Memory handling with error checking
 *
 *	Copyright (c) 1998-2007 J. Schilling
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
#include <schily/jmpdefs.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

EXPORT	void	*__jmalloc	__PR((size_t size, char *msg, sigjmps_t *jmp));
EXPORT	void	*__jrealloc	__PR((void *ptr, size_t size, char *msg, sigjmps_t *jmp));
EXPORT	char	*__jsavestr	__PR((const char *s, sigjmps_t *jmp));

EXPORT void *
__jmalloc(size, msg, jmp)
	size_t		size;
	char		*msg;
	sigjmps_t	*jmp;
{
	void	*ret;

	ret = malloc(size);
	if (ret == NULL) {
		int	err = geterrno();

		errmsg(gettext("Cannot allocate memory for %s.\n"), msg);
		if (jmp == JM_EXIT)
			comexit(err);
		if (jmp != NULL)
			siglongjmp(jmp->jb, 1);
	}
	return (ret);
}

EXPORT void *
__jrealloc(ptr, size, msg, jmp)
	void		*ptr;
	size_t		size;
	char		*msg;
	sigjmps_t	*jmp;
{
	void	*ret;

	if (ptr == NULL)
		ret = malloc(size);
	else
		ret = realloc(ptr, size);
	if (ret == NULL) {
		int	err = geterrno();

		errmsg(gettext("Cannot realloc memory for %s.\n"), msg);
		if (jmp == JM_EXIT)
			comexit(err);
		if (jmp != NULL)
			siglongjmp(jmp->jb, 1);
	}
	return (ret);
}

EXPORT char *
__jsavestr(s, jmp)
	const char	*s;
	sigjmps_t		*jmp;
{
	char	*ret = __jmalloc(strlen(s)+1, "saved string", jmp);

	if (ret != NULL)
		strcpy(ret, s);
	return (ret);
}
