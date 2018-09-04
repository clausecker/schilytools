/* @(#)format.h	1.1 18/09/03 Copyright 2018 J. Schilling */
/*
 *	Definitions used in common by format.c and fconf.c
 *
 *	Copyright (c) 2018 J. Schilling
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

#ifndef	_FORMAT_H
#define	_FORMAT_H

/*
 * printf() modifier flags
 */
#define	MINUSFLG	1	/* '-' flag */
#define	PLUSFLG		2	/* '+' flag */
#define	SPACEFLG	4	/* ' ' flag */
#define	HASHFLG		8	/* '#' flag */
#define	APOFLG		16	/* '\'' flag */
#define	GOTDOT		32	/* '.' found */
#define	GOTSTAR		64	/* '*' found */
#define	UPPERFLG	128	/* %E/%F/%G */

extern	int	_ftoes __PR((char *, double, int, int, int));
extern	int	_ftofs __PR((char *, double, int, int, int));
#ifdef	HAVE_LONGDOUBLE
extern	int	_qftoes __PR((char *, long double, int, int, int));
extern	int	_qftofs __PR((char *, long double, int, int, int));
#endif

#endif	/* _FORMAT_H */
