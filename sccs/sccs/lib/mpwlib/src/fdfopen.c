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
 * Copyright 2006-2020 J. Schilling
 *
 * @(#)fdfopen.c	1.6 20/09/06 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)fdfopen.c 1.6 20/09/06 J. Schilling"
#endif
/*
 * @(#)fdfopen.c 1.5 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)fdfopen.c"
#pragma ident	"@(#)sccs:lib/mpwlib/fdfopen.c"
#endif
/*
	Interfaces with /lib/libS.a
	First arg is file descriptor, second is read/write mode (0/1).
	Returns file pointer on success,
	NULL on failure (no file structures available).
*/

# include	<defines.h>
# include	<macros.h>

FILE *
fdfopen(fd, mode)
register int fd, mode;
{
	if (fstat(fd, &Statbuf) < 0)
		return(NULL);
	mode &= O_ACCMODE;
	if (mode == O_WRONLY)
		return(fdopen(fd,"wb"));
	else if (mode == O_RDWR)
		return(fdopen(fd,"rb+"));
	else
		return(fdopen(fd,"rb"));
}
