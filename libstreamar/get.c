/* @(#)get.c	1.4 21/07/26 Copyright 2011-2021 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)get.c	1.4 21/07/26 Copyright 2011-2021 J. Schilling";
#endif
/*
 *	Get a file from a StreamArchive and store it in the filesystem
 *
 *	Copyright (c) 2011-2021 J. Schilling
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

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/utypes.h>
#include <schily/fcntl.h>
#include <schily/device.h>
#include <schily/errno.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/maxpath.h>
#include <schily/strar.h>
#include "table.h"

#define	my_uid		strar_my_uid
#define	mode_mask	strar_mode_mask

#define	ROOT_UID	0

#if defined(ENOTEMPTY) && ENOTEMPTY != EEXIST
#define	is_eexist(err)	((err) == EEXIST || (err) == ENOTEMPTY)
#else
#define	is_eexist(err)	((err) == EEXIST)
#endif
#if defined(EMISSDIR)
#define	is_enoent(err)	((err) == ENOENT || (err) == EMISSDIR)
#else
#define	is_enoent(err)	((err) == ENOENT)
#endif

#define	remove_file(n, b)	(0)

extern	uid_t	my_uid;
extern	mode_t	mode_mask;

LOCAL	int	strar_getfile	__PR((FINFO *info, const char *name));
LOCAL	BOOL	strar_tmpname	__PR((FINFO *info, char	*xname));
LOCAL	BOOL	_create_dirs	__PR((char *name));
LOCAL	BOOL	create_dirs	__PR((char *name));

int
strar_get(info)
	register FINFO	*info;
{
	char	nbuf[4096];
	int	n;

	if (info->f_name[0] == '.' && info->f_name[1] == '\0') {
		nbuf[0] = '\0';
	} else {
		strar_tmpname(info, nbuf);
		create_dirs(nbuf);
	}

	switch (info->f_xftype) {

	case XT_FILE:
	case XT_CONT:
			if (strar_getfile(info, nbuf) < 0) {
				goto err;
			}
			break;

	case XT_LINK:
			if (link(info->f_lname, nbuf) < 0) {
				errmsg("Cannot make link '%s'\n", nbuf);
				goto err;
			}
			break;

	case XT_SLINK:
			if (link(info->f_lname, nbuf) < 0) {
				errmsg("Cannot make symlink '%s'\n", nbuf);
				goto err;
			}
			break;

	case XT_DIR:
			if (nbuf[0] &&
			    mkdir(nbuf, info->f_mode | S_IWUSR) < 0) {
				errmsg("Cannot make directory '%s'\n", nbuf);
				goto err;
			}
			break;
	case XT_FIFO:
			if (mkfifo(nbuf, info->f_mode) < 0) {
				errmsg("Cannot make fifo '%s'\n", nbuf);
				goto err;
			}
			break;

	default:
			if (mknod(nbuf, info->f_mode,
			    makedev(info->f_rdevmaj, info->f_rdevmin)) < 0) {
				errmsg("Cannot make special '%s'\n", nbuf);
				goto err;
			}
	}
	/*
	 * Read status
	 */
	n = strar_hparse(info);
	if (!n || info->f_status != 0) {
		if (info->f_xftype != XT_DIR)
			unlink(nbuf);
		else
			rmdir(nbuf);
		goto err;
	}
	if (nbuf[0] == '\0')
		return (0);
	if (rename(nbuf, info->f_name) >= 0)
		return (0);
	errmsgno(EX_BAD, "Could not extract '%s'.\n", info->f_name);
err:
	return (-1);
}

LOCAL int
strar_getfile(info, name)
	register FINFO	*info;
	const	char	*name;
{
	char	buf[32*1024];
	off_t	n;
	off_t	amt;
	FILE	*f = info->f_fp;
	int	fd;

	fd = creat(name, info->f_mode | S_IWUSR);
	if (fd < 0) {
		errmsg("Cannot create '%s'.\n", name);
		strar_skip(info);
		return (-1);
	}
	for (n = 0; n < info->f_size; n += amt) {
		amt = info->f_size - n;
		if (amt > sizeof (buf))
			amt = sizeof (buf);

		amt = fileread(f, buf, amt);
		if (amt < 0) {
			errmsg("Cannot read '%s'.\n", info->f_fpname);
			close(fd);
			return (-1);
		}
		if (write(fd, buf, amt) != amt) {
			errmsg("Cannot write '%s'.\n", name);
			close(fd);
			n += amt;
			goto err;
		}
	}
	close(fd);
	return (0);
err:
	for (; n < info->f_size; n++)
		getc(f);
	return (-1);
}

/*
 * Create a temporary path name for the extraction in -install mode.
 */
LOCAL BOOL
strar_tmpname(info, xname)
	FINFO	*info;
	char	*xname;
{
	register char	*xp = xname;
	register char	*np;
	register char	*dp;

	np = info->f_name;
	dp = xp;
	do {
		if ((*xp++ = *np) == '/')
			dp = xp;
	} while (*np++);
	if ((dp - xname) >= (PATH_MAX-6)) {
		errmsgno(ENAMETOOLONG,
				"Cannot make temporary name for '%s'.\n",
				info->f_name);
		return (FALSE);
	}
	strcpy(dp, "XXXXXX");
	seterrno(0);
	mktemp(xname);
	if (xname[0] == '\0') {
		errmsg("Cannot make temporary name for '%s'.\n",
				info->f_name);
		return (FALSE);
	}
	return (TRUE);
}

/*
 * Create intermediate directories.
 * If the user is not root and the umask is degenerated or read-only,
 * we add 0700 to the granted permissions. For this reason, we may need
 * to correct the permissins of intermediate directories later from the
 * directory stack.
 */
LOCAL BOOL
_create_dirs(name)
	register char	*name;
{
	mode_t	mode;

	mode = mode_mask;				/* used to be 0777 */
	if (my_uid != ROOT_UID)
		mode |= S_IRWXU;	/* Make sure we will be able write */

	if (mkdir(name, mode) < 0) {
		if (create_dirs(name) &&
		    mkdir(name, mode) >= 0) {
#ifdef	__needed__
			_dir_setownwer(name);
			if (mode != mode_mask)
				sdirmode(name, mode_mask); /* Push umask */
#endif
			return (TRUE);
		}
		return (FALSE);
	}
#ifdef	__needed__
	_dir_setownwer(name);
	if (mode != mode_mask)
		sdirmode(name, mode_mask);	/* Push umask on dirstack */
#endif
	return (TRUE);
}

#ifdef	__needed__
/*
 * Set the owner/group of intermedia directories.
 * Be very careful not to overwrite sgid directory semantics.
 */
LOCAL void
_dir_setownwer(name)
	char	*name;
{
	FINFO	dinfo;

	if (my_uid != ROOT_UID)
		return;

	if (dir_uid == _BAD_UID && dir_gid == _BAD_GID)
		return;

	if (!_getinfo(name, &dinfo) || !is_dir(&dinfo))
		return;

	if (dir_uid != _BAD_UID)
		dinfo.f_uid = dir_uid;
	if (dir_gid != _BAD_GID)
		dinfo.f_gid = dir_gid;

	chown(name, dinfo.f_uid, dinfo.f_gid);
}
#endif

EXPORT BOOL
create_dirs(name)
	register char	*name;
{
	register char	*np;
	register char	*dp;
		int	err;
		int	olderr = 0;

#ifdef	__needed__
	if (noxdir) {
		errmsgno(EX_BAD, "Directories not created.\n");
		return (FALSE);
	}
#endif
	np = dp = name;
	do {
		if (*np == '/')
			dp = np;
	} while (*np++);
	if (dp == name) {
		/*
		 * Do not create the very last directory
		 */
		return (TRUE);
	}
	*dp = '\0';
	if (access(name, F_OK) < 0) {
		if (_create_dirs(name)) {
			*dp = '/';
			return (TRUE);
		}
		err = geterrno();
		if ((err == EACCES || is_eexist(err))) {
			olderr = err;
			goto exists;
		}
		*dp = '/';
		return (FALSE);
	} else {
		struct stat dinfo;

	exists:
		if (lstat(name, &dinfo)) {
			if (S_ISDIR(dinfo.st_mode)) {
				*dp = '/';
				return (TRUE);
			}

			if (remove_file(name, FALSE)) {
				if (_create_dirs(name)) {
					*dp = '/';
					return (TRUE);
				}
				*dp = '/';
				return (FALSE);
			} else {
				*dp = '/';
				return (FALSE);
			}
		}
		*dp = '/';
		if (olderr == EACCES)
			seterrno(olderr);
		return (FALSE);
	}
}
