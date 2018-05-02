/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may use this file only in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
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
 * Copyright 2006-2018 J. Schilling
 *
 * @(#)logname.c	1.10 18/04/30 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)logname.c 1.10 18/04/30 J. Schilling"
#endif
/*
 * @(#)logname.c 1.7 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)logname.c"
#pragma ident	"@(#)sccs:lib/comobj/logname.c"
#endif
#include	<defines.h>
#include	<schily/pwd.h>

char saveid[50];		/* sync with hdr/defines.h */

char *
logname()
{
	struct passwd *log_name;
	uid_t uid;

	uid = getuid();
	log_name = getpwuid(uid);
	if (!log_name) {
		return (0);
	} else {
		strlcpy(saveid, log_name->pw_name, sizeof (saveid));
		sccs_user(saveid);
	}
	return (saveid);
}

char *
sccs_user(uname)
	char	*uname;
{
	register char	*p;

	/*
	 * Cygwin allows usernames that include spaces.
	 * Since this would break our history file format,
	 * we replace spaces by underscores.
	 */
	for (p = uname; *p != '\0'; p++) {
		if (*p == ' ')
			*p = '_';
	}
	return (uname);
}
