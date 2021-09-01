#ifndef _MKSH_MKSH_H
#define _MKSH_MKSH_H
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
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)mksh.h 1.7 06/12/12
 */

#pragma	ident	"@(#)mksh.h	1.7	06/12/12"

/*
 * Copyright 2017 J. Schilling
 *
 * @(#)mksh.h	1.4 21/08/16 2017 J. Schilling
 */

/*
 * Included files
 */
#if defined(DISTRIBUTED) || defined(MAKETOOL) /* tolik */
#	include <dm/Avo_DmakeCommand.h>
#endif

#include <mksh/defs.h>

#if defined(DISTRIBUTED) || defined(MAKETOOL) /* tolik */

extern int	do_job(Avo_DmakeCommand *cmd_list[], char *env_list[], char *stdout_file, char *stderr_file, char *cwd, char *cnwd, int ignore, int silent, pathpt vroot_path, char *shell, int nice_prio);

#endif /* TEAMWARE_MAKE_CMN */

#endif
