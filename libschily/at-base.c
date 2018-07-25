/* @(#)at-base.c	1.2 18/07/16 Copyright 2011-2018 J. Schilling */
/*
 *	Emulate the behavior of openat(fd, name, flag, mode)
 *	Replace "openat" by "FUNC_CALL".
 *
 *	Note that emulation methods that do not use the /proc filesystem are
 *	not MT safe. In the non-MT-safe case, we do:
 *
 *		savewd()/fchdir()/open(name)/restorewd()
 *
 *	Since the /proc method may fail with ENAMETOOLONG, we need to fall back
 *	to the fchdir() method in case of long path names and as a result are
 *	not MT-safe with long path names.
 *
 *	Errors may force us to abort the program as our caller is not expected
 *	to know that we do more than a simple open() here and that the
 *	working directory may be changed by us.
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

#ifndef	ENAMETOOLONG
#define	ENAMETOOLONG	EINVAL
#endif

#ifndef	_AT_TRIGGER
#define	_AT_TRIGGER	0
#endif

#ifdef  PROTOTYPES
EXPORT FUNC_RESULT
FUNC_NAME(int fd, const char *name PROTO_DECL)
#else
EXPORT FUNC_RESULT
FUNC_NAME(fd, name KR_ARGS)
	int		fd;
	const char	*name;
	KR_DECL
#endif
{
	int	n;
	FUNC_RESULT	ret = 0;
	char	buf[PATH_MAX];
	char	*proc_name;
	struct save_wd save_wd;

	FLAG_CHECK();
	if (fd == AT_FDCWD || ABS_NAME(name))
		return (FUNC_CALL(name));

	if ((proc_name = proc_fd2name(buf, fd, name)) != NULL) {
		/*
		 * The next FUNC_CALL() frequently results in ENAMETOOLONG
		 */
		ret = FUNC_CALL(proc_name);
		if (ret >= 0 || NON_PROCFS_ERRNO(errno))
			return (ret);
	}

	/*
	 * /proc open failed or /proc not available on this platform.
	 * Give it a chance using fchdir(). But the following code is
	 * not MT-safe!
	 */

	if (savewd(&save_wd) < 0) {
		/*
		 * We abort here as the caller may not know that we are forced
		 * to savewd/fchdir/restorewd here and misinterpret errno.
		 */
		savewd_abort(geterrno());
		/* NOTREACHED */
		return (-1);
	}
	if (fd >= 0 && save_wd.fd == fd) {
		/*
		 * If we just opened "fd" with the same number in savewd(), fd
		 * must have been illegal when calling openat();
		 */
		closewd(&save_wd);
		seterrno(EBADF);
		return (-1);
	}
	if ((n = fchdir(fd)) < 0) {
		int	err = geterrno();
		/*
		 * In case that fchdir() is emulated via chdir() and we use a
		 * multi hop chdir(), we may be on an undefined intermediate
		 * directory. Try to return to last working directory and if
		 * this fails, abort for security.
		 */
		if (n == -2 && restorewd(&save_wd) < 0) {
			restorewd_abort(geterrno());
			/* NOTREACHED */
		}
		closewd(&save_wd);
		seterrno(err);
		return (-1);
	}

	ret = FUNC_CALL(name);		/* The actual call() */

	if (restorewd(&save_wd) < 0) {
		int	err = geterrno();

		closewd(&save_wd);
		restorewd_abort(err);
		/* NOTREACHED */
		seterrno(err);
		return (-1);
	}
	closewd(&save_wd);

	return (ret);
}
