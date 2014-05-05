/* @(#)fchmodat.c	1.1 14/05/01 Copyright 2014 J. Schilling */
/*
 *	Emulate the behavior of fchmodat(int fd, const char *name, mode_t mode, int flag)
 *
 *	Note that emulation methods that do not use the /proc filesystem are
 *	not MT safe. In the non-MT-safe case, we do:
 *
 *		savewd()/fchdir()/open(name)/restorewd()
 *
 *	Errors may force us to abort the program as our caller is not expected
 *	to know that we do more than a simple open() here and that the
 *	working directory may be changed by us.
 *
 *	Copyright (c) 2014 J. Schilling
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
#include <schily/stat.h>
#include <schily/maxpath.h>
#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include "at-defs.h"

#ifndef	HAVE_FCHMODAT
#ifndef	HAVE_CHMOD
/* ARGSUSED */
#ifdef	PROTOTYPES
EXPORT int
fchmodat(int fd, const char *name, mode_t mode)
#else
EXPORT int
fchmodat(fd, name, mode)
	int		fd;
	const char	*name;
	mode_t		mode;
#endif
{
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (-1);
}
#else	/* HAVE_CHMOD */

#ifndef	HAVE_LCHMOD
#ifdef	PROTOTYPES
EXPORT int
lchmod(const char *name, mode_t mode)
#else
EXPORT int
lchmod(name, mode)
	const char	*name;
	mode_t		mode;
#endif
{
	struct stat	statbuf;

	statbuf.st_mode = 0;
	fstatat(AT_FDCWD, name, &statbuf, AT_SYMLINK_NOFOLLOW);

	if (!S_ISLNK(statbuf.st_mode))
		return (chmod(name, mode));
#ifdef	EOPNOTSUPP
	seterrno(EOPNOTSUPP);
#else
	seterrno(EINVAL);
#endif
	return (-1);
}
#endif	/* HAVE_LCHMOD */

/* CSTYLED */
#define	PROTO_DECL	, mode_t mode, int flag
#define	KR_DECL		mode_t mode; int flag;
/* CSTYLED */
#define	KR_ARGS		, mode, flag
#define	FUNC_CALL(n)	(flag & AT_SYMLINK_NOFOLLOW ? \
				lchmod(n, mode) : chmod(n, mode))
#define	FLAG_CHECK()	if (flag & ~(AT_SYMLINK_NOFOLLOW)) { \
				seterrno(EINVAL);			 \
				return (-1);				 \
			}

#define	FUNC_NAME	fchmodat
#define	FUNC_RESULT	int

#include "at-base.c"

#endif	/* HAVE_CHMOD */
#endif	/* HAVE_FCHMODDAT */
