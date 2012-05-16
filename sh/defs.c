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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved	*/

#if defined(sun)
#pragma ident	"@(#)defs.c	1.13	05/09/13 SMI"
#endif

/*
 * This file contains modifications Copyright 2008-2012 J. Schilling
 *
 * @(#)defs.c	1.9 12/05/11 2008-2012 J. Schilling
 */
#ifdef	SCHILY_BUILD
#include <schily/mconfig.h>
#endif
#ifndef lint
static	UConst char sccsid[] =
	"@(#)defs.c	1.9 12/05/11 2008-2012 J. Schilling";
#endif

/*
 *	UNIX shell
 */

#ifdef	SCHILY_BUILD
#include		<schily/mconfig.h>
#include		<setjmp.h>
#include		"mode.h"
#include		"name.h"
#include		<schily/param.h>
#include		"defs.h"
#else
#include		<setjmp.h>
#include		"mode.h"
#include		"name.h"
#include		<sys/param.h>
#endif
#ifndef NOFILE
#define	NOFILE 20
#endif
/* temp files and io */

int				output = 2;
int				ioset;
struct ionod	*iotemp;	/* files to be deleted sometime */
struct ionod	*fiotemp;	/* function files to be deleted sometime */
struct ionod	*iopend;	/* documents waiting to be read at NL */
struct fdsave	fdmap[NOFILE];

/* substitution */
int				dolc;
unsigned char			**dolv;
struct dolnod	*argfor;
struct argnod	*gchain;


/* name tree and words */
int				wdval;
int				wdnum;
int				fndef;
int				nohash;
struct argnod	*wdarg;
int				wdset;
BOOL			reserv;

/* special names */
unsigned char			*pcsadr;
unsigned char			*pidadr;
unsigned char			*cmdadr;

/* transput */
int			tmpout_offset;
unsigned int		serial;
int			peekc;
int			peekn;
unsigned char			*comdiv;
long			flags;
int				rwait;	/* flags read waiting */

/* error exits from various parts of shell */
jmp_buf			subshell;
jmp_buf			errshell;

/* fault handling */
BOOL			trapnote;

/* execflgs */
int				exitval;
int				retval;
BOOL			execbrk;
int				loopcnt;
int				breakcnt;
int			funcnt;
int				eflag;
/*
 * The following flag is set if you try to exit with stopped jobs.
 * On the second try the exit will succeed.
 */
int			tried_to_exit;
/*
 * The following flag is set to true if /usr/ucb is found in the path
 * before /usr/bin. This value is checked when executing the echo and test
 * built-in commands. If true, the command behaves as in BSD systems.
 */
int				ucb_builtins;

/* The following stuff is from stak.h	*/

/*
 * staktop = stakbot + local stak size
 */
unsigned char			*stakbas;	/* New stack base after addblok() */
unsigned char			*staktop;	/* Points behind local stak */
unsigned char			*stakbot = 0;	/* Bottom addr for local stak */
struct blk			*stakbsy;	/* Busy elements from addblok() */
unsigned char			*brkend;	/* The first invalid address */
