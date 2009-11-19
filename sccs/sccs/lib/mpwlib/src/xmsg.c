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
 * This file contains modifications Copyright 2009 J. Schilling
 *
 * @(#)xmsg.c	1.3 09/11/08 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)xmsg.c 1.3 09/11/08 J. Schilling"
#endif
/*
 * @(#)xmsg.c 1.6 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)xmsg.c"
#pragma ident	"@(#)sccs:lib/mpwlib/xmsg.c"
#endif
# include	<defines.h>

/*
	Call fatal with an appropriate error message
	based on errno.  If no good message can be made up, it makes
	up a simple message.
	The second argument is a pointer to the calling functions
	name (a string); it's used in the manufactured message.
*/
int
xmsg(file,func)
const char *file, *func;
{
	register char *str;
	char d[FILESIZE];
	extern char SccsError[];

	switch (errno) {
	case ENFILE:
		str = gettext("no file (ut3)");
		break;
	case ENOENT:
		sprintf(str = SccsError,gettext("`%s' nonexistent (ut4)"),
			file);
		break;
	case EACCES:
		copy(file,d);
		sprintf(str = SccsError,gettext("directory `%s' unwritable (ut2)"),
			dname(d));
		break;
	case ENOSPC:
		str = gettext("no space! (ut10)");
		break;
	case EFBIG:
		str = gettext("write error (ut8)");
		break;
	default:
		sprintf(str = SccsError,gettext("errno = %d, function = `%s' (ut11)"),
			errno,
			func);
		break;
	}
	return(fatal(str));
}




