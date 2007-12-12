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
 * Copyright 1994 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * This file contains modifications Copyright 2006-2007 J. Schilling
 *
 * @(#)sname.c	1.3 07/01/11 J. Schilling
 */
#if defined(sun) || defined(__GNUC__)

#ident "@(#)sname.c 1.3 07/01/11 J. Schilling"
#endif
/*
 * @(#)sname.c 1.3 06/12/12
 */

#ident	"@(#)sname.c"
#ident	"@(#)sccs:lib/mpwlib/sname.c"
/*
	Returns pointer to "simple" name of path name; that is,
	pointer to first character after last "/".  If no slashes,
	returns pointer to first char of arg.
	If the string ends in a slash, returns a pointer to the first
	character after the preceeding slash, or the first character.
*/
#include <defines.h>

char	*sname(s)
char *s;
{
	register char *p;
	register int n;
	register int j;

	n = strlen(s);
	--n;
	if (s[n] == '/') {
		for (j=n; j >= 0; --j)
			if (s[j] != '/') {
				s[++j] = '\0';
				break;
			}
	}

	for(p=s; *p; p++) if(*p == '/') s = p + 1;
	return(s);
}
