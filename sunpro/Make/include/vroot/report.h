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
/*
 * Copyright 1994 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)report.h 1.6 06/12/12
 */

#pragma	ident	"@(#)report.h	1.6	06/12/12"

/*
 * This file contains modifications Copyright 2017 J. Schilling
 *
 * @(#)report.h	1.2 17/04/23 2017 J. Schilling
 */

#ifndef _REPORT_H_
#define _REPORT_H_

#include <stdio.h>

extern FILE	*get_report_file(void);
extern char	*get_target_being_reported_for(void);
extern void	report_dependency(register const char *name);
extern int	file_lock(char *name, char *lockname, int *file_locked, int timeout);
#ifdef NSE
extern char	*setenv(char *name, char *value);
#endif

#define SUNPRO_DEPENDENCIES "SUNPRO_DEPENDENCIES"
#define LD 	"LD"
#define COMP 	"COMP"

/* the following definitions define the interface between make and
 * NSE - the two systems must track each other.
 */
#define NSE_DEPINFO 		".nse_depinfo"
#define NSE_DEPINFO_LOCK 	".nse_depinfo.lock"
#define NSE_DEP_ENV 		"NSE_DEP"
#define NSE_TFS_PUSH 		"/usr/nse/bin/tfs_push"
#define NSE_TFS_PUSH_LEN 	8
#define NSE_VARIANT_ENV 	"NSE_VARIANT"
#define NSE_RT_SOURCE_NAME 	"Shared_Source"

#endif
