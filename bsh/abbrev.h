/* @(#)abbrev.h	1.11 09/05/17 Copyright 1985-2009 J. Schilling */
/*
 *	Copyright (c) 1985-2009 J. Schilling
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

typedef int abidx_t;


#define	ABTABS		2	/* .globals & .locals				*/

#define	GLOBAL_AB	0	/* The idx for the '.globals' abbrev./aliases */
#define	LOCAL_AB	1	/* The idx for the '.locals' abbrev./aliases */

extern	void	ab_read		__PR((abidx_t tab, char *fname));
extern	void	ab_sname	__PR((abidx_t tab, char *fname));
extern	char	*ab_gname	__PR((abidx_t tab));
extern	void	ab_use		__PR((abidx_t tab, char *fname));
extern	void	ab_close	__PR((abidx_t tab));
extern	void	ab_insert	__PR((abidx_t tab, char *name, char *val, BOOL beg));
extern	void	ab_push		__PR((abidx_t tab, char *name, char *val, BOOL beg));
extern	void	ab_delete	__PR((abidx_t tab, char *name));
extern	char	*ab_value	__PR((abidx_t tab, char *name, BOOL beg));
extern	void	ab_dump		__PR((abidx_t tab, FILE *file, BOOL histflg));
extern	void	ab_list		__PR((abidx_t tab, char *pattern, FILE *file, BOOL histflg));
