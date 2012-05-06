/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


#if defined(sun)
#pragma ident	"@(#)name.h	1.7	05/06/08 SMI"	/* SVr4.0 1.7.1.1 */
#endif

#ifndef	_NAME_H
#define	_NAME_H

/*
 * This file contains modifications Copyright 2009-2012 J. Schilling
 * @(#)name.h	1.5 12/04/22 2009-2012 J. Schilling
 */
/*
 *	UNIX shell
 */


#define	N_ENVCHG 0020
#define	N_RDONLY 0010
#define	N_EXPORT 0004
#define	N_ENVNAM 0002
#define	N_FUNCTN 0001

#define	N_DEFAULT 0

struct namnod
{
	struct namnod	*namlft;
	struct namnod	*namrgt;
	unsigned char	*namid;
	unsigned char	*namval;
	unsigned char	*namenv;
	int	namflg;
};

#endif /* _NAME_H */
