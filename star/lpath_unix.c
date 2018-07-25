/* @(#)lpath_unix.c	1.11 18/07/22 Copyright 2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)lpath_unix.c	1.11 18/07/22 Copyright 2018 J. Schilling";
#endif
/*
 *	Routines for long path names on unix like operating systems
 *
 *	Copyright (c) 2018 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/stdio.h>
#include <schily/errno.h>
#include "star.h"
#include <schily/standard.h>
#include <schily/unistd.h>
#include <schily/dirent.h>
#include <schily/fcntl.h>	/* For AT_FDCWD */
#include <schily/stat.h>
#include <schily/param.h>	/* For DEV_BSIZE */
#include <schily/string.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include "starsubs.h"

EXPORT	int	lchdir		__PR((char *name));
#if	defined(IS_SUN) && defined(__SVR4) && defined(DO_CHDIR_LONG)
LOCAL	void	sunos5_cwdfix	__PR((void));
#endif
EXPORT	char	*lgetcwd	__PR((void));
EXPORT	int	lmkdir		__PR((char *name, mode_t mode));
EXPORT	int	laccess		__PR((char *name, int amode));
EXPORT	int	lstatat		__PR((char *name, struct stat *buf, int flag));
EXPORT	int	lchmodat	__PR((char *name, mode_t mode, int flag));
EXPORT	int	lutimensat	__PR((char *name, struct timespec *ts, int flag));
EXPORT	int	lreadlink	__PR((char *name, char *buf, size_t bufsize));
EXPORT	int	lsymlink	__PR((char *name, char *name2));
EXPORT	int	llink		__PR((char *name, char *name2));
EXPORT	int	lrename		__PR((char *name, char *name2));
EXPORT	int	lmknod		__PR((char *name, mode_t mode, dev_t dev));
EXPORT	int	lmkfifo		__PR((char *name, mode_t mode));
EXPORT	int	lchownat	__PR((char *name, uid_t uid, gid_t gid, int flag));
EXPORT	int	lunlinkat	__PR((char *name, int flag));
EXPORT	char	*lmktemp	__PR((char *name));
EXPORT	FILE	*lfilemopen	__PR((char *name, char *mode, mode_t cmode));
LOCAL	int	_fileopenat	__PR((int fd, char *name, char *mode));
EXPORT	int	_lfileopen	__PR((char *name, char *mode));
EXPORT	DIR	*lopendir	__PR((char *name));
LOCAL	int	hop_dirs	__PR((char *name, char **np));

/*
 * A chdir() implementation that is able to deal with ENAMETOOLONG.
 */
EXPORT int
lchdir(path)
	char	*path;
{
	int	ret = chdir(path);
	char	*p;
	char	*p2;

	if (ret >= 0 || geterrno() != ENAMETOOLONG)
		return (ret);

	p = path;
	if (*p == '/') {
		if ((ret = chdir("/")) < 0)
			return (ret);
		while (*p == '/')
			p++;
	}
	while (*p) {
		if ((p2 =  strchr(p, '/')) != NULL)
			*p2 = '\0';
		if ((ret = chdir(p)) < 0) {
			*p2 = '/';
			break;
		}
		if (p2 == NULL)
			break;
		*p2++ = '/';
		while (*p2 == '/')
			p2++;
		p = p2;
	}
	return (ret);
}

#if	defined(IS_SUN) && defined(__SVR4) && defined(DO_CHDIR_LONG)
/*
 * Workaround a Solaris getcwd() bug that hits on newer Solaris releases with
 * getcwd() being a system call. Once a successful getcwd() call that returns
 * more than PATH_MAX did succeed, future calls to getcwd() always fail with
 * ERANGE until a successfull new chdir() call has been issued.
 */
LOCAL void
sunos5_cwdfix()
{
	int	f = open(".", O_SEARCH|O_DIRECTORY|O_NDELAY);

	if (f >= 0) {
		chdir("/");
		fchdir(f);
		close(f);
	}
}
#endif

/*
 * A getcwd() implementation that is able to deal with more than PATH_MAX.
 */
EXPORT char *
lgetcwd()
{
	char	*dir = NULL;
	char	*r;
	size_t	len = PATH_MAX;
#define	LWD_RETRY_MAX	8			/* stops at 128 * PATH_MAX */
	int	i = 0;

#if	defined(IS_SUN) && defined(__SVR4) && defined(DO_CHDIR_LONG)
retry:
#endif
	do {
		dir = ___realloc(dir, len, "working dir");
		*dir = '\0';

		r = getcwd(dir, len);
		if (++i >= LWD_RETRY_MAX)	/* stops at 128 * PATH_MAX */
			break;
		len *= 2;
	} while (r == NULL && errno == ERANGE);

#if	defined(IS_SUN) && defined(__SVR4) && defined(DO_CHDIR_LONG)
	if (r == NULL && errno == ERANGE && i == LWD_RETRY_MAX) {
		/*
		 * Work around a Solaris kernel bug.
		 */
		sunos5_cwdfix();
		goto retry;
	}
#endif
	return (r);
}

EXPORT int
#ifdef	PROTOTYPES
lmkdir(char *name, mode_t mode)
#else
lmkdir(name, mode)
	char		*name;
	mode_t		mode;
#endif
{
#ifdef	HAVE_FCHDIR
	char	*p;
	int	fd;
	int	err;
#endif
	int	ret;

	if ((ret = mkdir(name, mode)) < 0 &&
	    geterrno() != ENAMETOOLONG) {
		return (ret);
	}

#ifdef	HAVE_FCHDIR
	if (ret >= 0)
		return (ret);

	fd = hop_dirs(name, &p);
	ret = mkdirat(fd, p, mode);
	err = geterrno();
	close(fd);
	seterrno(err);
#endif
	return (ret);
}

EXPORT int
laccess(name, amode)
	char		*name;
	int		amode;
{
#ifdef	HAVE_FCHDIR
	char	*p;
	int	fd;
	int	err;
#endif
	int	ret;

	if ((ret = access(name, amode)) < 0 &&
	    geterrno() != ENAMETOOLONG) {
		return (ret);
	}

#ifdef	HAVE_FCHDIR
	if (ret >= 0)
		return (ret);

	fd = hop_dirs(name, &p);
	ret = faccessat(fd, p, amode, 0);
	err = geterrno();
	close(fd);
	seterrno(err);
#endif
	return (ret);
}

EXPORT int
lstatat(name, buf, flag)
	char		*name;
	struct stat	*buf;
	int		flag;
{
#ifdef	HAVE_FCHDIR
	char	*p;
	int	fd;
	int	err;
#endif
	int	ret;

	if ((ret = fstatat(AT_FDCWD, name, buf, flag)) < 0 &&
	    geterrno() != ENAMETOOLONG) {
		return (ret);
	}

#ifdef	HAVE_FCHDIR
	if (ret >= 0)
		return (ret);

	fd = hop_dirs(name, &p);
	ret = fstatat(fd, p, buf, flag);
	err = geterrno();
	close(fd);
	seterrno(err);
#endif
	return (ret);
}


EXPORT int
#ifdef	PROTOTYPES
lchmodat(char *name, mode_t mode, int flag)
#else
lchmodat(name, mode, flag)
	char		*name;
	mode_t		mode;
	int		flag;
#endif
{
#ifdef	HAVE_FCHDIR
	char	*p;
	int	fd;
	int	err;
#endif
	int	ret;

	if ((ret = fchmodat(AT_FDCWD, name, mode, flag)) < 0 &&
	    geterrno() != ENAMETOOLONG) {
		return (ret);
	}

#ifdef	HAVE_FCHDIR
	if (ret >= 0)
		return (ret);

	fd = hop_dirs(name, &p);
	ret = fchmodat(fd, p, mode, flag);
	err = geterrno();
	close(fd);
	seterrno(err);
#endif
	return (ret);
}


EXPORT int
lutimensat(name, ts, flag)
	char		*name;
	struct timespec	*ts;
	int		flag;
{
#ifdef	HAVE_FCHDIR
	char	*p;
	int	fd;
	int	err;
#endif
	int	ret;

	if ((ret = utimensat(AT_FDCWD, name, ts, flag)) < 0 &&
	    geterrno() != ENAMETOOLONG) {
		return (ret);
	}

#ifdef	HAVE_FCHDIR
	if (ret >= 0)
		return (ret);

	fd = hop_dirs(name, &p);
	ret = utimensat(fd, p, ts, flag);
	err = geterrno();
	close(fd);
	seterrno(err);
#endif
	return (ret);
}

#ifdef	HAVE_READLINK
EXPORT int
lreadlink(name, buf, bufsize)
	char		*name;
	char		*buf;
	size_t		bufsize;
{
#ifdef	HAVE_FCHDIR
	char	*p;
	int	fd;
	int	err;
#endif
	int	ret;

	if ((ret = readlink(name, buf, bufsize)) < 0 &&
	    geterrno() != ENAMETOOLONG) {
		return (ret);
	}

#ifdef	HAVE_FCHDIR
	if (ret >= 0)
		return (ret);

	fd = hop_dirs(name, &p);
	ret = readlinkat(fd, p, buf, bufsize);
	err = geterrno();
	close(fd);
	seterrno(err);
#endif
	return (ret);
}

EXPORT int
lsymlink(name, name2)
	char		*name;
	char		*name2;
{
#ifdef	HAVE_FCHDIR
	char	*p;
	int	fd;
	int	err;
#endif
	int	ret;

	if ((ret = symlink(name, name2)) < 0 &&
	    geterrno() != ENAMETOOLONG) {
		return (ret);
	}

#ifdef	HAVE_FCHDIR
	if (ret >= 0)
		return (ret);

	fd = hop_dirs(name2, &p);
	ret = symlinkat(name, fd, p);
	err = geterrno();
	close(fd);
	seterrno(err);
#endif
	return (ret);
}
#endif

#ifdef	HAVE_LINK
EXPORT int
llink(name, name2)
	char		*name;
	char		*name2;
{
#ifdef	HAVE_FCHDIR
	char	*p;
	char	*p2;
	int	fd;
	int	fd2;
	int	err;
#endif
	int	ret;

	if ((ret = link(name, name2)) < 0 &&
	    geterrno() != ENAMETOOLONG) {
		return (ret);
	}

#ifdef	HAVE_FCHDIR
	if (ret >= 0)
		return (ret);

	fd = hop_dirs(name, &p);
	fd2 = hop_dirs(name2, &p2);
	ret = linkat(fd, p, fd2, p2, 0); /* SYMLINK_FOLLOW not in emulation */
	err = geterrno();
	close(fd);
	close(fd2);
	seterrno(err);
#endif
	return (ret);
}
#endif

EXPORT int
lrename(name, name2)
	char		*name;
	char		*name2;
{
#ifdef	HAVE_FCHDIR
	char	*p;
	char	*p2;
	int	fd;
	int	fd2;
	int	err;
#endif
	int	ret;

	if ((ret = rename(name, name2)) < 0 &&
	    geterrno() != ENAMETOOLONG) {
		return (ret);
	}

#ifdef	HAVE_FCHDIR
	if (ret >= 0)
		return (ret);

	fd = hop_dirs(name, &p);
	fd2 = hop_dirs(name2, &p2);
	ret = renameat(fd, p, fd2, p2);
	err = geterrno();
	close(fd);
	close(fd2);
	seterrno(err);
#endif
	return (ret);
}

#ifdef	HAVE_MKNOD
EXPORT int
#ifdef	PROTOTYPES
lmknod(char *name, mode_t mode, dev_t dev)
#else
lmknod(name, mode, dev)
	char		*name;
	mode_t		mode;
	dev_t		dev;
#endif
{
#ifdef	HAVE_FCHDIR
	char	*p;
	int	fd;
	int	err;
#endif
	int	ret;

	if ((ret = mknod(name, mode, dev)) < 0 &&
	    geterrno() != ENAMETOOLONG) {
		return (ret);
	}

#ifdef	HAVE_FCHDIR
	if (ret >= 0)
		return (ret);

	fd = hop_dirs(name, &p);
	ret = mknodat(fd, p, mode, dev);
	err = geterrno();
	close(fd);
	seterrno(err);
#endif
	return (ret);
}
#endif

#ifdef	HAVE_MKFIFO
EXPORT int
#ifdef	PROTOTYPES
lmkfifo(char *name, mode_t mode)
#else
lmkfifo(name, mode)
	char		*name;
	mode_t		mode;
#endif
{
#ifdef	HAVE_FCHDIR
	char	*p;
	int	fd;
	int	err;
#endif
	int	ret;

	if ((ret = mkfifo(name, mode)) < 0 &&
	    geterrno() != ENAMETOOLONG) {
		return (ret);
	}

#ifdef	HAVE_FCHDIR
	if (ret >= 0)
		return (ret);

	fd = hop_dirs(name, &p);
	ret = mkfifoat(fd, p, mode);
	err = geterrno();
	close(fd);
	seterrno(err);
#endif
	return (ret);
}
#endif

EXPORT int
#ifdef	PROTOTYPES
lchownat(char *name, uid_t uid, gid_t gid, int flag)
#else
lchownat(name, uid, gid, flag)
	char		*name;
	uid_t		uid;
	gid_t		gid;
	int		flag;
#endif
{
#ifdef	HAVE_FCHDIR
	char	*p;
	int	fd;
	int	err;
#endif
	int	ret;

	if ((ret = fchownat(AT_FDCWD, name, uid, gid, flag)) < 0 &&
	    geterrno() != ENAMETOOLONG) {
		return (ret);
	}

#ifdef	HAVE_FCHDIR
	if (ret >= 0)
		return (ret);

	fd = hop_dirs(name, &p);
	ret = fchownat(fd, p, uid, gid, flag);
	err = geterrno();
	close(fd);
	seterrno(err);
#endif
	return (ret);
}

EXPORT int
lunlinkat(name, flag)
	char		*name;
	int		flag;
{
#ifdef	HAVE_FCHDIR
	char	*p;
	int	fd;
	int	err;
#endif
	int	ret;

	if ((ret = unlinkat(AT_FDCWD, name, flag)) < 0 &&
	    geterrno() != ENAMETOOLONG) {
		return (ret);
	}

#ifdef	HAVE_FCHDIR
	if (ret >= 0)
		return (ret);

	fd = hop_dirs(name, &p);
	ret = unlinkat(fd, p, flag);
	err = geterrno();
	close(fd);
	seterrno(err);
#endif
	return (ret);
}

EXPORT char *
lmktemp(name)
	char		*name;
{
#ifdef	HAVE_FCHDIR
	char	*p;
	int	fd;
	int	fdh;
	int	err = 0;
	char	spath[PATH_MAX+1];
	char	*oname = spath;
#endif
	char	*ret;

	if (name == NULL)
		return (name);

#ifdef	HAVE_FCHDIR
	/*
	 * mktemp() is destructive, so we need to save the name before
	 * as we always fail first with long path names.
	 */
	if (strlen(name) < PATH_MAX) {
		strcpy(oname, name);
	} else {
		oname = strdup(name);
		if (oname == NULL) {
			name[0] = '\0';
			return (name);
		}
	}
#endif
	ret = mktemp(name);
	if (ret[0] == '\0' &&
	    geterrno() != ENAMETOOLONG) {
#ifdef	HAVE_FCHDIR
		if (oname != spath)
			free(oname);
#endif
		return (ret);
	}

#ifdef	HAVE_FCHDIR
	if (ret[0] != '\0') {
		if (oname != spath)
			free(oname);
		return (ret);
	}

	strcpy(name, oname);
	if (oname != spath)
		free(oname);
	fd = hop_dirs(name, &p);
	if (fd >= 0) {
		fdh = open(".", O_SEARCH|O_DIRECTORY|O_NDELAY);
		if (fdh >= 0) {
			fchdir(fd);
			ret = mktemp(p);
			err = geterrno();
			fchdir(fdh);
			close(fdh);
		}
		close(fd);
	}
	if (err)
		seterrno(err);
#endif
	return (ret);
}

EXPORT FILE *
#ifdef	PROTOTYPES
lfilemopen(char *name, char *mode, mode_t cmode)
#else
lfilemopen(name, mode, cmode)
	char		*name;
	char		*mode;
	mode_t		cmode;
#endif
{
#ifdef	HAVE_FCHDIR
	char	*p;
	char	*p2;
	int	fd;
	int	fdf;
	int	err;
	char	lmode[10];
#endif
	FILE	*fp;
	int	omode = 0;
	int	flag = 0;

	if ((fp = filemopen(name, mode, cmode)) == NULL &&
	    geterrno() != ENAMETOOLONG) {
		return (fp);
	}

#ifdef	HAVE_FCHDIR
	if (fp != NULL)
		return (fp);

	fd = hop_dirs(name, &p);
	_cvmod(mode, &omode, &flag);
	fdf = openat(fd, p, omode, cmode);
	p = mode;
	p2 = lmode;
	while (*p) {
		if (*p == 'c' || *p == 't' || *p == 'e') {
			p++;
			continue;
		}
		*p2++ = *p++;
	}
	*p2 = '\0';
	if (fdf >= 0 && (fp = fileluopen(fdf, lmode)) == NULL)
		close(fdf);
	err = geterrno();
	close(fd);
	seterrno(err);
#endif
	return (fp);
}

LOCAL int
_fileopenat(fd, name, smode)
	int	fd;
	char	*name;
	char	*smode;
{
	int	ret;
	int	omode = 0;
	int	flag = 0;

	if (!_cvmod(smode, &omode, &flag))
		return (-1);

retry:
	if ((ret = openat(fd, name, omode, S_IRWALL)) < 0) {
		if (geterrno() == EINTR)
			goto retry;
		return (-1);
	}

	return (ret);
}

EXPORT int
_lfileopen(name, smode)
	char	*name;
	char	*smode;
{
#ifdef	HAVE_FCHDIR
	char	*p;
	int	fd;
	int	err;
#endif
	int	fdf;

	if ((fdf = _fileopenat(AT_FDCWD, name, smode)) < 0 &&
	    geterrno() != ENAMETOOLONG) {
		return (fdf);
	}

#ifdef	HAVE_FCHDIR
	if (fdf >= 0)
		return (fdf);

	fd = hop_dirs(name, &p);
	fdf = _fileopenat(fd, p, smode);
	err = geterrno();
	close(fd);
	seterrno(err);
#endif
	return (fdf);
}

EXPORT DIR *
lopendir(name)
	char	*name;
{
#ifdef	HAVE_FCHDIR
	char	*p;
	int	fd;
	int	dfd;
#endif
	DIR	*ret = NULL;

	if ((ret = opendir(name)) == NULL && geterrno() != ENAMETOOLONG)
		return ((DIR *)NULL);

#ifdef	HAVE_FCHDIR
	if (ret)
		return (ret);

	fd = hop_dirs(name, &p);
	if ((dfd = openat(fd, p, O_RDONLY|O_DIRECTORY|O_NDELAY)) < 0) {
		close(fd);
		return ((DIR *)NULL);
	}
	close(fd);
	ret = fdopendir(dfd);
#endif
	return (ret);
}

#ifdef	HAVE_FCHDIR
LOCAL int
hop_dirs(name, np)
	char	*name;
	char	**np;
{
	char	*p;
	char	*p2;
	int	fd;
	int	dfd;
	int	err;

	p = name;
	fd = AT_FDCWD;
	if (*p == '/') {
		fd = openat(fd, "/", O_SEARCH|O_DIRECTORY|O_NDELAY);
		while (*p == '/')
			p++;
	}
	while (*p) {
		if ((p2 = strchr(p, '/')) != NULL) {
			if (p2[1] == '\0')
				break;
			*p2 = '\0';
		} else {
			break;
		}
		if ((dfd = openat(fd, p, O_SEARCH|O_DIRECTORY|O_NDELAY)) < 0) {
			err = geterrno();

			close(fd);
			if (err == EMFILE)
				seterrno(err);
			else
				seterrno(ENAMETOOLONG);
			*p2 = '/';
			return (dfd);
		}
		close(fd);	/* Don't care about AT_FDCWD, it is negative */
		fd = dfd;
		if (p2 == NULL)
			break;
		*p2++ = '/';
		while (*p2 == '/')
			p2++;
		p = p2;
	}
	*np = p;
	return (fd);
}
#endif
