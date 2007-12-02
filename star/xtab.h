/* @(#)xtab.h	1.4 06/10/31 Copyright 2001-2006 J. Schilling */
/*
 *	Copyright (c) 2001-2006 J. Schilling
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

#ifndef	_XTAB_H
#define	_XTAB_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
typedef	void (*xt_function)	__PR((FINFO *, char *, int, char *, int));

typedef struct {
	char		*x_name;
	int		x_namelen;
	xt_function	x_func;
	int		x_flag;
} xtab_t;

#endif	/* _XTAB_H */
