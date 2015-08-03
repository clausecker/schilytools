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
 * Copyright 2009-2015 J. Schilling
 * @(#)name.h	1.6 15/07/09 2009-2015 J. Schilling
 */
/*
 *	UNIX shell
 */


#define	N_ENVCHG 0020			/* This was changed after env import */
#define	N_RDONLY 0010			/* This node is read-only forever */
#define	N_EXPORT 0004			/* This node will be exported to env */
#define	N_ENVNAM 0002			/* This node was imported from env */
#define	N_FUNCTN 0001			/* This is a function, not a param */

#define	N_DEFAULT 0			/* No attributes (yet) */

struct namnod
{
	struct namnod	*namlft;	/* Left name tree */
	struct namnod	*namrgt;	/* Right name tree */
	unsigned char	*namid;		/* Node name, e.g. "HOME" */
	unsigned char	*namval;	/* Node value, e.g. "/home/joe" */
	unsigned char	*namenv;	/* Imported value or function node */
	int		namflg;		/* Flags, see above */
};

#endif /* _NAME_H */
