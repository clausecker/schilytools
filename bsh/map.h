/* @(#)map.h	1.4 03/10/23 Copyright 1986-2003 J. Schilling */
/*
 *	Copyright (c) 1986-2003 J. Schilling
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

#define	rmap(c)		(maptab[c] && rxmap(c))

extern	unsigned char	maptab[];

extern	int	mapgetc		__PR((void));
extern	void	map_init	__PR((void));
extern	int	rxmap		__PR((int c));
extern	int	gmap		__PR((void));
extern	void	remap		__PR((void));
extern	BOOL	add_map		__PR((char *from, char *to, char *comment));
extern	BOOL	del_map		__PR((char *from));
extern	void	list_map	__PR((FILE *f));
