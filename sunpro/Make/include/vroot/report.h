/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
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
/*
 * Copyright 1994 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)report.h 1.6 06/12/12
 */

#pragma	ident	"@(#)report.h	1.6	06/12/12"

/*
 * Copyright 2017 J. Schilling
 * Copyright 2023 the schilytools team
 *
 * @(#)report.h	1.5 21/08/15 2017 J. Schilling
 */

#ifndef _REPORT_H_
#define _REPORT_H_

#if defined(SCHILY_BUILD) || defined(SCHILY_INCLUDES)
#include <schily/stdio.h>
#else
#include <stdio.h>
#endif

extern FILE	*get_report_file(void);
extern char	*get_target_being_reported_for(void);
extern void	report_dependency(const char *name);
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
