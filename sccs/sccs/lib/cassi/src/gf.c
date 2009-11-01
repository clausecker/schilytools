/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * This file contains modifications Copyright 2006-2009 J. Schilling
 *
 * @(#)gf.c	1.4 09/11/01 J. Schilling
 */
#if defined(sun)
#ident "@(#)gf.c 1.4 09/11/01 J. Schilling"
#endif
/*
 * @(#)gf.c 1.5 06/12/12
 */

#if defined(sun)
#ident	"@(#)gf.c"
#ident	"@(#)sccs:lib/cassi/gf.c"
#endif

/* EMACS_MODES: c !fill tabstop=4 */

/*
 *	gf -- Get a .FRED file name for a particular application and subsystem.
 *
 *	The resulting pathname is placed in a static area that is overwritten
 *	by each call to gf ().
 *
 */

#include <defines.h>
#include <filehand.h>
#include <i18n.h>

/* Debugging options */

#ifdef TRACE
#define TR(W,X,Y,Z) fprintf (stdout, W, X, Y, Z)
#else
#define TR(W,X,Y,Z) /* W X Y Z */
#endif

#define SIZE 132

char *gf (appl)
char	*appl;
{
	static char	filename[SIZE];
	char		in_line[SIZE], *ptrs[3], *fmat[2], *tmp;

	TR("Gf: entry appl=(%s)\n", appl, EMPTY, EMPTY);
	(void) cat (filename, NOGETTEXT("/usr/lib/M2/"), appl, EMPTY);
	fmat[0] = NOGETTEXT("DBBD");
	fmat[1] = EMPTY;
	TR("Gf: fmat[0]=(%s) fmat[1]=(%s)\n", fmat[0], fmat[1], EMPTY);
	if (sweep (VERIFY, filename, EMPTY, '\n', ':', SIZE, fmat, in_line, ptrs,
	  (int (*) __PR((char *, int, char **))) NULL, (int (*) __PR((char **, char **, int))) NULL) != FOUND) {
		TR("Gf: not found\n", EMPTY, EMPTY, EMPTY);
		return (EMPTY);
		}
	tmp = strrchr (ptrs[1], (char) 01);
	*tmp = '\0';						/* Find and clobber control A. */
	(void) cat (filename, ptrs[1], NOGETTEXT("/.fred/.FRED"), EMPTY);
	TR("Gf: returns (%s)\n", filename, EMPTY, EMPTY);
	return (filename);
}

