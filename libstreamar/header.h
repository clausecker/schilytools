/* @(#)header.h	1.1 17/02/14 Copyright 2001-2017 J. Schilling */
/*
 *	Defitions for the stream archive internal interfaces.
 *
 *	Copyright (c) 2001-2017 J. Schilling
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

extern	int	to_utf8			__PR((Uchar *to, Uchar *from));
extern	int	to_utf8l		__PR((Uchar *to, Uchar *from,
						int len));
extern	BOOL	from_utf8		__PR((Uchar *to, Uchar *from));
extern	BOOL	from_utf8l		__PR((Uchar *to, Uchar *from,
						int *len));

#endif	/* _HEADER_H */
