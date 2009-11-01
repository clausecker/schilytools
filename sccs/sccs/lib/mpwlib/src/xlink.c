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
 * @(#)xlink.c	1.4 09/11/01 J. Schilling
 */
#if defined(sun)
#ident "@(#)xlink.c 1.4 09/11/01 J. Schilling"
#endif
/*
 * @(#)xlink.c 1.7 06/12/12
 */

#if defined(sun)
#ident	"@(#)xlink.c"
#ident	"@(#)sccs:lib/mpwlib/xlink.c"
#endif
/*
	Interface to link(II) which handles all error conditions.
	Returns 0 on success,
	fatal() on failure.
*/

# include       <defines.h>
# include       <i18n.h>

int
xlink(f1,f2)
const char	*f1, *f2;
{
	extern char SccsError[];

	if (link(f1,f2)) {
		if (errno == EEXIST || errno == EXDEV) {
			sprintf(SccsError, gettext("can't link `%s' to `%s' (%d)"),
				f2,
				f1,
				errno == EEXIST ? 111 : 112);
			return(fatal(SccsError));
		}
		if (errno == EACCES)
			f1 = f2;
		return(xmsg(f1,NOGETTEXT("xlink")));
	}
	return(0);
}
