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
 * Copyright 1999 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)args.h 1.7 06/12/12
 */

#pragma	ident	"@(#)args.h	1.7	06/12/12"

/*
 * Copyright 2017-2018 J. Schilling
 * Copyright 2023 the schilytools team
 *
 * @(#)args.h	1.5 21/08/15 2017-2018 J. Schilling
 */

#ifndef _ARGS_H_
#define _ARGS_H_

#if defined(SCHILY_BUILD) || defined(SCHILY_INCLUDES)
#include <schily/unistd.h>
#include <schily/errno.h>
#include <schily/time.h>
#include <schily/param.h>
#include <schily/stdio.h>
#include <schily/fcntl.h>	/* also sys/file.h if present */
#include <schily/types.h>
#include <schily/stat.h>
#else
#include <errno.h>
#include <sys/time.h>
#include <sys/param.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#endif

#ifdef	__never__		/* we do not need it */
#ifdef	HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif
#endif

typedef enum { rw_read, rw_write} rwt, *rwpt;

extern	void	translate_with_thunk(char *filename, int (*thunk) (char *), pathpt path_vector, pathpt vroot_vector, rwt rw);

union Args {
	struct { int mode;} access;
	struct { int mode;} chmod;
	struct { int user; int group;} chown;
	struct { int mode;} creat;
	struct { char **argv; char **environ;} execve;
	struct { struct stat *buffer;} lstat;
	struct { int mode;} mkdir;
	struct { char *name; int mode;} mount;
	struct { int flags; int mode;} open;
	struct { char *buffer; int buffer_size;} readlink;
	struct { struct stat *buffer;} stat;
#ifndef SUN5_0
	struct { struct statfs *buffer;} statfs;
#endif
	struct { int length;} truncate;
	struct { struct timeval *time;} utimes;
};

extern	union Args	vroot_args;
extern	int		vroot_result;

#endif
