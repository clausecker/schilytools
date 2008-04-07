/* @(#)fetchdir.h	1.5 08/04/06 Copyright 2002-2008 J. Schilling */
/*
 *	Copyright (c) 2002-2008 J. Schilling
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

#ifndef _SCHILY_FETCHDIR_H
#define	_SCHILY_FETCHDIR_H

#ifndef	_SCHILY_DIRENT_H
#include <schily/dirent.h>			/* Includes mconfig.h if needed	    */
#endif

#ifdef	__cplusplus
extern "C" {
#endif

extern	char	*fetchdir	__PR((char *dir, int *entp, int *lenp, ino_t **inop));
extern	char	*dfetchdir	__PR((DIR *dir, char *dirname, int *entp, int *lenp, ino_t **inop));
extern	int	fdircomp	__PR((const void *p1, const void *p2));
extern	char	**sortdir	__PR((char *dir, int *entp));
extern	int	cmpdir		__PR((int ents1, int ents2,
					char **ep1, char **ep2,
					char **oa, char **od,
					int *alenp, int *dlenp));

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_FETCHDIR_H */
