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


/*
 * Copyright (c) 1996, by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ident	"@(#)setbrk.c	1.10	05/06/08 SMI"	/* SVr4.0 1.8.1.1	*/

#include "defs.h"

/*
 * This file contains modifications Copyright 2008 J. Schilling
 *
 * @(#)setbrk.c	1.4 08/12/22 2008 J. Schilling
 */
#ifndef lint
static	const char sccsid[] =
	"@(#)setbrk.c	1.4 08/12/22 2008 J. Schilling";
#endif

/*
 *	UNIX shell
 */

#ifdef	NO_USER_MALLOC
#undef	HAVE_SBRK
#endif

unsigned char *setbrk	__PR((int incr));

unsigned char*
setbrk(incr)
int incr;
{
#ifdef	HAVE_SBRK
	register unsigned char *a = (unsigned char *)sbrk(incr);

	brkend = a + incr;
	return (a);
#else
	write(STDERR_FILENO, "Cannot work, sbrk(2) not supported.\n", 36);
	exit(-1);
	return (0);
#endif
}
