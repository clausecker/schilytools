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
/*
 * Copyright (c) 1999 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef	_SH_POLICY_H
#define	_SH_POLICY_H

#if defined(sun)
#pragma ident	"@(#)sh_policy.h	1.2	05/06/08 SMI"
#endif

/*
 * This file contains modifications Copyright 2008-2013 J. Schilling
 *
 * @(#)sh_policy.h	1.7 13/09/20 2008-2013 J. Schilling
 */

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef	SCHILY_INCLUDES
#include <schily/mconfig.h>
#ifdef	HAVE_EXEC_ATTR_H
#include <exec_attr.h>
#endif
#else
#include <exec_attr.h>

#define	__PR(a)	a
#endif

#define	PFEXEC		"/usr/bin/pfexec"
#define	ALL_PROFILE	"All"

#define	ERR_PASSWD	"can't get passwd entry"
#define	ERR_MEM		"can't allocate memory"
#define	ERR_CWD		"can't get current working directory"
#define	ERR_PATH	"resolved pathname too large"
#define	ERR_PROFILE	"not in profile"
#define	ERR_REALPATH	"can't get real path"
#define	ERR_SET_PFEXEC	"can't setpflags(PRIV_PFEXEC)"

#define	NOATTRS	0	/* command in profile but w'out attributes */

#define	SECPOLICY_WARN	1
#define	SECPOLICY_ERROR	2

/*
 * Shell Policy Interface Functions
 */
extern void secpolicy_init	__PR((void));
extern void secpolicy_end	__PR((void));
extern int secpolicy_pfexec	__PR((const char *, char **, const char **));
extern void secpolicy_print	__PR((int, const char *));


#ifdef	__cplusplus
}
#endif

#endif	/* _SH_POLICY_H */
