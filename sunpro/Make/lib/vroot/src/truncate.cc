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
 * Copyright 1993 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)truncate.cc 1.4 06/12/12
 */

#pragma	ident	"@(#)truncate.cc	1.4	06/12/12"

/*
 * Copyright 2017-2020 J. Schilling
 *
 * @(#)truncate.cc	1.5 21/08/15 2017-2020 J. Schilling
 */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)truncate.cc	1.5 21/08/15 2017-2020 J. Schilling";
#endif

#if defined(SCHILY_BUILD) || defined(SCHILY_INCLUDES)
#include <schily/unistd.h>
#else
#include <unistd.h>
#endif

extern int truncate(const char *path, off_t length);

#include <vroot/vroot.h>
#include <vroot/args.h>

static int	truncate_thunk(char *path)
{
	vroot_result= truncate(path, vroot_args.truncate.length);
	return(vroot_result == 0);
}

int	truncate_vroot(char *path, int length, pathpt vroot_path, pathpt vroot_vroot)
{
	vroot_args.truncate.length= length;
	translate_with_thunk(path, truncate_thunk, vroot_path, vroot_vroot, rw_read);
	return(vroot_result);
}
