/* @(#)header.h	1.2 18/05/17 Copyright 2001-2018 J. Schilling */
/*
 *	Defitions for the stream archive internal interfaces.
 *
 *	Copyright (c) 2001-2018 J. Schilling
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

#ifndef	_HEADER_H
#define	_HEADER_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif

#ifndef _SCHILY_STRAR_H
#include <schily/strar.h>
#endif

/*
 * Flags for gen_text()
 */
#define	T_ADDSLASH	1	/* Add slash to the argument	*/
#define	T_UTF8		2	/* Convert arg to UTF-8 coding	*/

/*
 * Transfer direction types for utf8_init()
 */
#define	S_CREATE	1
#define	S_EXTRACT	2

extern	void	strar_xbreset		__PR((void));
extern	char	*strar_gxbuf		__PR((void));
extern	int	strar_gxbsize		__PR((void));
extern	int	strar_gxblen		__PR((void));
extern	void	strar_xbgrow		__PR((int newsize));

extern	void	strar_gen_text		__PR((char *keyword,
						char *arg, int alen,
						Uint flags));
extern	void	strar_gen_xtime		__PR((char *keyword, time_t sec,
						Ulong nsec));
extern	void	strar_gen_number	__PR((char *keyword, Llong arg));
extern	void	strar_gen_unumber	__PR((char *keyword, ULlong arg));

extern	BOOL	strar_xhparse		__PR((FINFO *info, char	*p, char *ep));

/*
 * unicode.c
 */
extern	void	utf8_codeset	__PR((const char *code_set));
extern	void	utf8_init	__PR((int type));
extern	void	utf8_fini	__PR((void));
extern	int	to_utf8		__PR((Uchar *to, int tolen,
					Uchar *from, int len));
extern	BOOL	from_utf8	__PR((Uchar *to, int tolen,
					Uchar *from, int *len));

#endif	/* _HEADER_H */
