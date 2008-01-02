/* @(#)fstream.h	1.14 08/01/02 Copyright 1985-2008 J. Schilling */
/*
 *	Definitions for the file stream package
 *
 *	Copyright (c) 1985-2008 J. Schilling
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

#ifndef _SCHILY_FSTREAM_H
#define	_SCHILY_FSTREAM_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#define	STR_SBUF_SIZE	127	/* Size of "static" stream buffer	*/

#ifdef	WSTRINGS
typedef	short	CHAR;
#else
typedef	char	CHAR;
#endif


typedef struct fstream {
	FILE	*fstr_file;
	CHAR	*fstr_bp;
	CHAR	*fstr_buf;
	int 	(*fstr_func) __PR((struct fstream *, FILE *));
	int	(*fstr_rfunc) __PR((struct fstream *));
	CHAR	fstr_sbuf[STR_SBUF_SIZE + 1];
} fstream;

typedef	int	(*fstr_fun) __PR((struct fstream *, FILE *));
typedef	int	(*fstr_efun) __PR((char *));
typedef	int	(*fstr_rfun) __PR((struct fstream *));

extern	fstream	*mkfstream	__PR((FILE *f, fstr_fun, fstr_rfun, fstr_efun));
extern	void	fsclose		__PR((fstream *fsp));
extern	FILE	*fssetfile	__PR((fstream *fsp, FILE *f));
extern	int	fsgetc		__PR((fstream *fsp));
extern	void	fspushstr	__PR((fstream *fsp, char *ss));
extern	void	fspushcha	__PR((fstream *fsp, int c));

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_FSTREAM_H */
