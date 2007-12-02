/* @(#)filesubs.c	1.61 06/09/26 Copyright 1984-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)filesubs.c	1.61 06/09/26 Copyright 1984-2004 J. Schilling";
#endif
/*
 *	Commands that deal with non i/o file subroutines
 *
 *	Copyright (c) 1984-2004 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include "ved.h"
#include <schily/unistd.h>
#include <schily/fcntl.h>
#include <schily/errno.h>

EXPORT	void	lockfile	__PR((ewin_t *wp, char *filename));
EXPORT	void	writelockmsg	__PR((ewin_t *wp));
LOCAL	int	check_lock	__PR((void));
EXPORT	int	lockfd		__PR((int f));
LOCAL	int	lock_fcntl	__PR((int f));
EXPORT	int	getfmodes	__PR((Uchar* name));
EXPORT	long	gftime		__PR((char *file));
EXPORT	BOOL	readable	__PR((Uchar* name));
EXPORT	BOOL	writable	__PR((Uchar* name));
EXPORT	BOOL	wrtcheck	__PR((ewin_t *wp, BOOL  err));
EXPORT	BOOL	modcheck	__PR((ewin_t *wp));

/*
 * Used when no locking interface is available.
 */
#ifdef	ENOSYS
#	define	NO_LOCK	ENOSYS
#else
#	define	NO_LOCK	EINVAL
#endif
#define	no_lock()	(seterrno(NO_LOCK))

/*
 * Lock the current default file.
 */
EXPORT void
lockfile(wp, filename)
	ewin_t	*wp;
	char	*filename;
{
	int	f = open(filename, O_RDWR);
	int	ret;

	if (f < 0) {	/* New file ??? */
		if (geterrno() != ENOENT)
			wp->eflags |= FNOLOCK;
		return;
	}

	ret = lockfd(f);

	switch (ret) {

	case LOCK_LOCAL:
		wp->eflags |= FLOCKLOCAL;
	case LOCK_OK:
		wp->curfd = f;
		break;
	case LOCK_ALREADY:
		close(f);
		wp->eflags |= FREADONLY;
		wp->eflags |= FNOLOCK;
		break;
	case LOCK_CANNOT:
		close(f);
		wp->eflags |= FNOLOCK;
		break;
	}
}

/*
 * Write an error message according to the current lock status.
 * We cannot do this in lockfile() because after the file is read,
 * the system line is cleared.
 */
EXPORT void
writelockmsg(wp)
	ewin_t	*wp;
{
	if ((wp->eflags & (FREADONLY|FNOLOCK)) == (FREADONLY|FNOLOCK))
		writeerr(wp, "FILE ALREADY LOCKED");
	else if ((wp->eflags & FNOLOCK) != 0)
		writeerr(wp, "CANNOT LOCK FILE");
	else if ((wp->eflags & FLOCKLOCAL) != 0)
		writeserr(wp, "LOCK IS LOCAL ONLY");
}

/*
 * Check whether the lock was successful.
 */
LOCAL int
check_lock()
{
	/*
	 * We need to distinguish a lock not being available for the file
	 * from the file system not supporting locking.
	 * Fcntl is documented to return EACCESS and EAGAIN if file is already
	 * locked. We need to add EWOULDBLOCK as *BSD returns this undocumented
	 * value.
	 */
	return (geterrno() == EACCES || geterrno() == EAGAIN
#ifdef EWOULDBLOCK
						|| geterrno() == EWOULDBLOCK
#endif
	?  LOCK_ALREADY : LOCK_CANNOT);
}

/*
 * Do the actual file locking.
 */
EXPORT int
lockfd(f)
	int	f;
{
	int	ret	= LOCK_CANNOT;
	BOOL	didlock	= FALSE;

	if (nolock)
		return (LOCK_OK);

#ifdef	HAVE_LOCKF
#undef	FOUND_LOCK
#define	FOUND_LOCK
	/*
	 * First try the most recent standard interface.
	 * It is most likely working correctly on all systems.
	 * It also supports locking for all machines that
	 * use the file via NFS.
	 */
	seterrno(0);
	if (lockf(f, F_TLOCK, 0) < 0) {
		ret = check_lock();
	} else {
		ret = LOCK_OK;
		didlock = TRUE;
	}
#endif

#ifdef	HAVE_FCNTL_LOCKF
#undef	FOUND_LOCK
#define	FOUND_LOCK
	/*
	 * Now try the SVSvr4 interface.
	 * It also supports locking for all machines that
	 * use the file via NFS.
	 */
	if (ret == LOCK_CANNOT) {
		seterrno(0);
		if (lock_fcntl(f) < 0) {
			ret = check_lock();
		} else {
			ret = LOCK_OK;
			didlock = TRUE;
		}
	}
#endif

#ifdef	HAVE_FLOCK
#undef	FOUND_LOCK
#define	FOUND_LOCK
	/*
	 * If no other lock method is supported or if the lock
	 * was signalled successful we call this outdated BSD
	 * interface.
	 * This is needed because on some old systems the
	 * standard compliant locking may not work correctly.
	 * There are implementations that implent the old BSD
	 * method completely disjunct from the standard method
	 * and thus force us to apply for both locks.
	 *
	 * As this old BSD method does most likely not work over NFS
	 * it would be a bad idea to only use this locking call as
	 * 'nvi' does. Co-operative work in a non-networked environment
	 * looks like a method from the past.
	 */
	if (ret != LOCK_ALREADY) {
		seterrno(0);
		if (flock(f, LOCK_EX | LOCK_NB) < 0) {
			if (!didlock)
				ret = check_lock();
		} else {
			if (!didlock)
				ret = LOCK_LOCAL;
			didlock = TRUE;
		}
	}
#endif
#ifndef	FOUND_LOCK
	no_lock();
	return (check_lock());
#else
	return (ret);
#endif
}

LOCAL int
lock_fcntl(f)
	int	f;
{
#ifdef	HAVE_FCNTL_LOCKF
	struct flock fl;
	int	ret;

	fl.l_whence = 0;
	fl.l_start = 0;
	fl.l_len = 0;
	fl.l_type = 0;

	fl.l_type |= F_WRLCK;

	ret = fcntl(f, F_SETLK, &fl);
	return (ret);
#else
	no_lock();
	return (-1);
#endif
}

#ifdef	JOS
#	include <sys/stypes.h>
#	include <sys/access.h>
#	include <sys/filedesc.h>
#	include <error.h>

#	define	stat		status
#	define	st_mtime	f_tmodified
#	define	st_ino		f_fileno
#	define	st_dev		f_fromfs
#	define	STATBUF		FILEDESC

/*
 * Get the file permissions
 */
EXPORT int
getfmodes(name)
	Uchar	*name;
{
	FILEDESC	fd;

	if (status(C name, &fd) < 0)
		return (-1);
	return (fd.f_modes);
}

/*
 * Set the file permissions in UNIX flavor
 */
int
chmod(name, modes)
	char	*name;
	int	modes;
{
	FILE	*f;

	if (f = fileopen(name, "")) {
		chaccess(fdown(f), modes);
		fclose(f);
	}
}

#else
#	include <schily/stat.h>
#	define	STATBUF		struct stat

/*
 * Get the file permissions
 */
EXPORT int
getfmodes(name)
	Uchar	*name;
{
	struct stat	fd;

	if (stat(C name, &fd) < 0)
		return (-1);

	/*
	 * We are only interested in the file permissions which will fit into
	 * an 'int' as long as it hold >= 12 bits.
	 */
	return ((int)fd.st_mode);
}

#endif

/*
 * Get the time of last modification for a file.
 */
EXPORT long
gftime(file)
	char	*file;
{
	STATBUF	stbuf;

	stbuf.st_mtime = 0;
	if (stat(file, &stbuf) < 0) {
		/*
		 * GNU libc.6 destroys st_mtime
		 */
		stbuf.st_mtime = 0;
	}
	return ((long)stbuf.st_mtime);
}

/*
 * Return if the file is readable
 */
EXPORT BOOL
readable(name)
	Uchar	*name;
{
	int	ret;

	ret = access((char *)name, R_OK); /* XXX nur OK wenn uid == euid !!! */

	if (ret < 0)
		return (FALSE);
	return (TRUE);
}

/*
 * Return if the file is writable
 */
EXPORT BOOL
writable(name)
	Uchar	*name;
{
	int	ret;

	ret = access((char *)name, W_OK); /* XXX nur OK wenn uid == euid !!! */

	if (ret < 0)
		return (geterrno() == ENOENT);

#ifdef	JOS
	if (getuid() == 0 && (getfmodes(name) & 0x222) == 0)
		return (FALSE);
#else
	if (getuid() == 0 && (getfmodes(name) & 0222) == 0)
		return (FALSE);
#endif
	return (TRUE);
}

/*
 * Check if we are in read only mode or if the file is read only.
 * write the appropriate message and return if we would be allowed to write.
 */
EXPORT BOOL
wrtcheck(wp, err)
	ewin_t	*wp;
	BOOL	err;
{
	int	ronly = ReadOnly;

	if (ronly == 0 && (wp->eflags & FREADONLY) != 0)
		ronly = 1;

	if (ronly > 0) {
		(err ? writeerr : writemsg)(wp, "READ ONLY MODE");
		return (FALSE);
	}
	if (!writable(wp->curfile)) {
		(err ? writeerr : writemsg)(wp, "READ ONLY FILE");
		return (FALSE);
	}
	return (TRUE);
}

/*
 * Check if the file has been modified more recent than our copy.
 */
EXPORT BOOL
modcheck(wp)
	ewin_t	*wp;
{
	long	ftime;

	ftime = gftime(C wp->curfile);
	if (ftime != 0 && wp->curftime != ftime) {
		writemsg(wp, "FILE MODIFIED BY ANOTHER USER");
		return (FALSE);
	}
	return (TRUE);
}
