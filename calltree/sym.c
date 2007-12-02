/* @(#)sym.c	1.19 06/09/13 Copyright 1985, 1999 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)sym.c	1.19 06/09/13 Copyright 1985, 1999 J. Schilling";
#endif
/*
 *	A program to produce a static calltree for C-functions
 *	symbol handling
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

#include <schily/mconfig.h>
#include <stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include "sym.h"
#include "strsubs.h"

LOCAL	sym_t	*newsym		__PR((char *name, sym_t *gtab));
EXPORT	sym_t	*lookup		__PR((char *name, sym_t **table, sym_t *gtab));

LOCAL sym_t *
newsym(name, gtab)
	char	*name;
	sym_t	*gtab;
{
	sym_t *tmp;
	sym_t *t = (sym_t *)0;

	tmp = (sym_t *) xmalloc(sizeof (sym_t), "allocsym");
	tmp->s_left		= (sym_t *) NULL;
	tmp->s_right		= (sym_t *) NULL;
	tmp->s_filename		= (char *) NULL;
	tmp->s_sym		= (sym_t *) NULL;
	tmp->s_lineno		= 0;
	tmp->s_flags		= 0;

	if (gtab != L_CREATE)
		t = lookup(name, &gtab, FALSE);
	if (t) {
		tmp->s_name = t->s_name;
	} else {
		tmp->s_name = xmalloc(strlen(name)+1, "allocsym string");
	}
	strcpy(tmp->s_name, name);
	return (tmp);
}

EXPORT sym_t *
lookup(name, table, gtab)
		char	*name;
	register sym_t	**table;
		sym_t	*gtab;
{
	register int	cmp;

	if (*table) {
		cmp = strcmp(name, (*table)->s_name);
		if (cmp == 0)
			return (*table);
		else if (cmp < 0)
			return (lookup(name, &((*table)->s_left), gtab));
		else if (cmp > 0)
			return (lookup(name, &((*table)->s_right), gtab));

	} else if (gtab != L_LOOK) {
		return (*table = newsym(name, gtab));
	}
	return ((sym_t *)0);
}
