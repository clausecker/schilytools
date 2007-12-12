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
 * This file contains modifications Copyright 2006-2007 J. Schilling
 *
 * @(#)xopen.c	1.3 07/01/11 J. Schilling
 */
#if defined(sun) || defined(__GNUC__)

#ident "@(#)xopen.c 1.3 07/01/11 J. Schilling"
#endif
/*
 * @(#)xopen.c 1.8 06/12/12
 */

#ident	"@(#)xopen.c"
#ident	"@(#)sccs:lib/mpwlib/xopen.c"
/*
	Interface to open(II) which differentiates among the various
	open errors.
	Returns file descriptor on success,
	fatal() on failure.
*/

# include <defines.h>
# include <i18n.h>
# include <ccstypes.h>

int
xopen(name,mode)
char name[];
int mode;
{
	register int fd;
	extern char SccsError[];

	if ((fd = open(name,mode)) < 0) {
		if(errno == EACCES) {
			if(mode == 0)
				sprintf(SccsError,gettext("`%s' unreadable (ut5)"),
					name);
			else if(mode == 1)
				sprintf(SccsError,gettext("`%s' unwritable (ut6)"),
					name);
			else
				sprintf(SccsError,gettext("`%s' unreadable or unwritable (ut7)"),
					name);
			fd = fatal(SccsError);
		}
		else
			fd = xmsg(name,NOGETTEXT("xopen"));
	}
	return(fd);
}
