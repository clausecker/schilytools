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

/*
 * Copyright 1990 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#ifndef	_HASH_H
#define	_HASH_H

#ifdef	SCHILY_INCLUDES
#include <schily/mconfig.h>
#endif

#if defined(sun)
#pragma ident	"@(#)hash.h	1.8	05/09/13 SMI"
#endif

/*
 * This file contains modifications Copyright 2008-2015 J. Schilling
 *
 * @(#)hash.h	1.8 15/07/11 2008-2015 J. Schilling
 */

/*
 *	UNIX shell
 */
#ifdef	__cplusplus
extern "C" {
#endif

#define		HASHZAP		0x03FF	/* Mask all but BUILTIN / FUNCTION */
#define		CDMARK		0x8000	/* Mark outdated "::" based entries */

#define		NOTFOUND	0x0000	/* Cannot exec, reason in low 8 bits */
#define		BUILTIN		0x0100	/* Builtin, builtin # in low 8 bits */
#define		FUNCTION	0x0200	/* Data value for all functions */
#define		COMMAND		0x0400	/* Command, PATH index in low 8 bits */
#define		REL_COMMAND	0x0800	/* Relative command from "::" in PATH */
#define		PATH_COMMAND	0x1000	/* Command with PATH= in local env */
#define		DOT_COMMAND	0x8800	/* CDMARK | REL_COMMAND */

#define		hashtype(x)	(x & 0x1F00)
#define		hashdata(x)	(x & 0x00FF)


typedef struct entry
{
	unsigned char	*key;		/* Hash key string (command name) */
	short		data;		/* Hash data, see flags above */
	unsigned char	hits;		/* # of hash hits % 256 */
	unsigned char	cost;		/* cost: # of PATH entries to search */
	struct entry	*next;
} ENTRY;

extern ENTRY	*hfind	__PR((unsigned char *));
extern ENTRY	*henter	__PR((ENTRY));
extern void	hcreate	__PR((void));
extern void	hscan	__PR((void (*uscan)(ENTRY *)));

#ifdef	__cplusplus
}
#endif

#endif /* !_HASH_H */
