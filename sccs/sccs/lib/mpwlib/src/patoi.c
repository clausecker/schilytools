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
 * @(#)patoi.c	1.4 09/11/01 J. Schilling
 */
#if defined(sun)
#ident "@(#)patoi.c 1.4 09/11/01 J. Schilling"
#endif
/*
 * @(#)patoi.c 1.6 06/12/12
 */

#if defined(sun)
#ident	"@(#)patoi.c"
#ident	"@(#)sccs:lib/mpwlib/patoi.c"
#endif

#include <defines.h>

/*
	Function to convert ascii string to integer.  Converts
	positive numbers only.  Returns -1 if non-numeric
	character encountered.
*/

int
patoi(s)
register char *s;
{
	register int i;
	char str[2];

	i = 0;
	sprintf(str,"%c",*s);
	while(strcoll((const char *) str, (const char *) "0") >= 0 &&
	      strcoll((const char *) str, (const char *) "9") <= 0) {
	  i = 10 * i + (*s - '0');
	  s++;
	  sprintf(str,"%c",*s);
	}

	if(*s) return(-1);
	return(i);
}
