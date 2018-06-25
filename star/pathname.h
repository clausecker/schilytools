/* @(#)pathname.h	1.5 18/06/20 Copyright 2004-2018 J. Schilling */
/*
 *	Copyright (c) 2004-2018 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef	_PATHNAME_H
#define	_PATHNAME_H

#ifndef _SCHILY_TYPES_H
#include <schily/types.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef	EOF
#define	PS_F	FILE		/* stdio.h included	*/
#else
#define	PS_F	void		/* stdio.h not included	*/
#endif

#define	PS_INCR		1024	/* How to increment ps_path size */
#define	PS_SILENT	(PS_F *)0 /* No messages		 */
#define	PS_STDERR	(PS_F *)1 /* Messages to stderr		 */
#define	PS_EXIT		(PS_F *)2 /* Messages to stderr + exit	 */

typedef struct pathstore {
	char	*ps_path;	/* The current path name	*/
	size_t	ps_tail;	/* Where to append to path name	*/
	size_t	ps_size;	/* Current size of 'ps_path'	*/
} pathstore_t;

extern	int		init_pspace	__PR((PS_F *f, pathstore_t *pathp));
extern	ssize_t		incr_pspace	__PR((PS_F *f, pathstore_t *pathp,
								size_t amt));
extern	ssize_t		set_pspace	__PR((PS_F *f, pathstore_t *pathp,
								size_t amt));
extern	ssize_t		grow_pspace	__PR((PS_F *f, pathstore_t *pathp,
								size_t amt));
extern	void		free_pspace	__PR((pathstore_t *pathp));
extern	int		strlcpy_pspace	__PR((PS_F *f, pathstore_t *pathp,
						const char *nm, size_t nlen));
extern	int		strcpy_pspace	__PR((PS_F *f, pathstore_t *pathp,
							const char *nm));

#ifdef	__cplusplus
}
#endif

#endif	/* _PATHNAME_H */
