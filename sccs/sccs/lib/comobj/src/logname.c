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
 * @(#)logname.c	1.5 09/11/08 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)logname.c 1.5 09/11/08 J. Schilling"
#endif
/*
 * @(#)logname.c 1.7 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)logname.c"
#pragma ident	"@(#)sccs:lib/comobj/logname.c"
#endif
# include	<defines.h>
# include	<pwd.h>

char saveid[50];		/* sync with hdr/defines.h */
char	*logname()
{
	struct passwd *log_name;
	uid_t uid;

	uid = getuid();
	log_name = getpwuid(uid);
	if (!log_name)
	   return(0);
	else
	   strcpy(saveid,log_name->pw_name);
	return(saveid);
}
