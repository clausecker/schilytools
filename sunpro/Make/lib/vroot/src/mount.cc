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
 * Copyright 1995 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)mount.cc 1.5 06/12/12
 */

#pragma	ident	"@(#)mount.cc	1.5	06/12/12"

/*
 * Copyright 2017 J. Schilling
 *
 * @(#)mount.cc	1.4 21/08/16 2017 J. Schilling
 */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)mount.cc	1.4 21/08/16 2017 J. Schilling";
#endif

#include <vroot/vroot.h>
#include <vroot/args.h>

#ifdef	HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

#ifndef HP_UX
extern int mount(const char *spec, const char *dir, int mflag, ...);
#endif

static int	mount_thunk(char *path)
{
	vroot_result= mount(path, vroot_args.mount.name, vroot_args.mount.mode);
	return(vroot_result == 0);
}

int	mount_vroot(char *target, char *name, int mode, pathpt vroot_path, pathpt vroot_vroot)
{
	vroot_args.mount.name= name;
	vroot_args.mount.mode= mode;
	translate_with_thunk(target, mount_thunk, vroot_path, vroot_vroot, rw_read);
	return(vroot_result);
}
