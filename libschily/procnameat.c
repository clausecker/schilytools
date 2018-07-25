/* @(#)procnameat.c	1.3 18/07/16 Copyright 2011-2018 J. Schilling */
/*
 *	Return a path name for a /proc related access to fd/name if possible.
 *
 *	We need to test this at runtime in order to avoid incorrect behavior
 *	from running a program on a newer OS that it has been compiled for.
 *	There are also platforms like FreeBSD where mounting /proc is optioonal.
 *
 **************
 *	NOTE:	The entries /proc/self/{fd|path}/%d are symlinks and thus do
 *		not allow to access paths of unlimited length as possible
 *		with a real openat(fd, name, flags) call.
 **************
 *
 *	If our caller falls back to the fchdir() method, e.g. because we
 *	did not detect /proc, or because /proc/self/{fd|path}/%d results
 *	in ENAMETOOLONG, the resulting code is no longer MT-safe.
 *
 *	Copyright (c) 2011-2018 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/unistd.h>
#include <schily/types.h>
#include <schily/fcntl.h>
#include <schily/maxpath.h>
#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include "at-defs.h"

#ifndef	ENAMETOOLONG
#define	ENAMETOOLONG	EINVAL
#endif

#define	PROC_SELF_PATH_FORMAT	"/proc/self/path/%d/%s"	/* Newer procfs */
#define	PROC_SELF_FD_FORMAT	"/proc/self/fd/%d/%s"	/* Older procfs */
#define	PROC_PID_FD_FORMAT	"/proc/%ld/fd/%d/%s"	/* AIX has no self */

#define	PT_SELF_PATH		1
#define	PT_SELF_FD		2
#define	PT_PID_FD		3

char *
proc_fd2name(buf, fd, name)
	char		*buf;
	int		fd;
	const char	*name;
{
static	int	proc_type;

	if (proc_type == 0) {
		int	proc_fd;

		/*
		 * First test the newer feature as the older /proc/self/fd/%d
		 * feature is still available on newer systems.
		 */
		proc_fd = open("/proc/self/path", O_SEARCH);
		if (proc_fd >= 0) {
			proc_type = PT_SELF_PATH;
			close(proc_fd);
		} else {
			proc_fd = open("/proc/self/fd", O_SEARCH);
			if (proc_fd >= 0) {
				proc_type = PT_SELF_FD;
				close(proc_fd);
			} else {
				js_snprintf(buf, PATH_MAX,
					"/proc/%ld/fd", (long)getpid());
				proc_fd = open(buf, O_SEARCH);
				if (proc_fd >= 0) {
					proc_type = PT_PID_FD;
					close(proc_fd);
				} else {
					/*
					 * No /proc fs found
					 */
					proc_type = -1;
					seterrno(0);
					return ((char *)0);
				}
			}
		}
	} else if (proc_type < 0) {
		/*
		 * No /proc fs found
		 */
		seterrno(0);
		return ((char *)0);
	}
	if (proc_type == PT_PID_FD) {
		if (js_snprintf(buf, PATH_MAX,
				PROC_PID_FD_FORMAT,
				(long)getpid(), fd, name) >= PATH_MAX) {
			seterrno(ENAMETOOLONG);
			return (NULL);
		}
	} else if (js_snprintf(buf, PATH_MAX,
			proc_type == PT_SELF_PATH ?
				PROC_SELF_PATH_FORMAT :
				PROC_SELF_FD_FORMAT,
			fd, name) >= PATH_MAX) {
		seterrno(ENAMETOOLONG);
		return (NULL);
	}
	return (buf);
}
