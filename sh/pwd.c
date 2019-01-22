/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * A copy of the CDDL is also available via the Internet at
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
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


#if defined(sun)
#pragma ident	"@(#)pwd.c	1.12	06/06/16 SMI"
#endif

#include "defs.h"

/*
 * Copyright 2008-2019 J. Schilling
 *
 * @(#)pwd.c	1.39 19/01/14 2008-2019 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)pwd.c	1.39 19/01/14 2008-2019 J. Schilling";
#endif

/*
 *	UNIX shell
 */
#ifdef	SCHILY_INCLUDES
#include	<schily/errno.h>
#include	<schily/types.h>
#include	<schily/fcntl.h>	/* For O_XXX and AT_FDCWD */
#include	<schily/stat.h>
#include	<schily/getcwd.h>
#else
#include	"mac.h"
#include	<errno.h>
#include	<sys/types.h>
#include	<fcntl.h>
#include	<sys/stat.h>
#include	<limits.h>
#endif

#define	DOT	'.'
#ifndef	NULL
#define	NULL	0
#endif
#define	SLASH	'/'
#define	PARTLY	2		/* ncwdname may contain symlinks */

	int	lchdir		__PR((char *path));
#if	defined(IS_SUN) && defined(__SVR4) && defined(DO_CHDIR_LONG)
static	void	sunos5_cwdfix	__PR((void));
#endif
static	Uchar	*lgetcwd	__PR((void));
	int	lstatat		__PR((char *name, struct stat *buf, int flag));
	void	cwd		__PR((unsigned char *dir,
					unsigned char *cwdbase));
static	void	cwd2		__PR((int cdflg));
static	int	cwd3		__PR((struct stat *statp));
#ifdef	DO_POSIX_CD
	int	cwdrel2abs	__PR((void));
#endif
	unsigned char *cwdget	__PR((int cdflg));
	void	cwdprint	__PR((int cdflg));
static void	rmslash		__PR((unsigned char *string));
	void	ocwdnod		__PR((void));
static void	cwdnod		__PR((unsigned char *wdname));

#ifdef __STDC__
extern const char	longpwd[];
#else
extern char	longpwd[];
#endif

unsigned char *ocwdname;
unsigned char cwdname[2];		/* Just a place holder for a Nul byte */
unsigned char *ncwdname = cwdname;	/* The official handle for $PWD	*/

static int	didpwd = FALSE;

/*
 * A chdir() implementation that is able to deal with ENAMETOOLONG.
 */
int
lchdir(path)
	char	*path;
{
	int	ret = chdir(path);
#if	defined(ENAMETOOLONG) && defined(DO_CHDIR_LONG)
	char	*p;
	char	*p2;

	if (ret >= 0 || errno != ENAMETOOLONG)
		return (ret);

	p = path;
	if (*p == SLASH) {
		if ((ret = chdir("/")) < 0)
			return (ret);
		while (*p == SLASH)
			p++;
	}
	while (*p) {
		if ((p2 =  strchr(p, SLASH)) != NULL)
			*p2 = '\0';
		if ((ret = chdir(p)) < 0) {
			if (p2)
				*p2 = SLASH;
			break;
		}
		if (p2 == NULL)
			break;
		*p2++ = SLASH;
		while (*p2 == SLASH)
			p2++;
		p = p2;
	}
#endif	/* defined(ENAMETOOLONG) && defined(DO_CHDIR_LONG) */
	return (ret);
}

#if	defined(IS_SUN) && defined(__SVR4) && defined(DO_CHDIR_LONG)
/*
 * Workaround a Solaris getcwd() bug that hits on newer Solaris releases with
 * getcwd() being a system call. Once a successful getcwd() call that returns
 * more than PATH_MAX did succeed, future calls to getcwd() always fail with
 * ERANGE until a successfull new chdir() call has been issued.
 */
static void
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
 *
 * Since lgetcwd() overwrites possible data at "staktop", it cannot be called
 * while ephemeral string processing on the string stack is under way.
 */
static unsigned char *
lgetcwd()
{
	unsigned char	*dir = staktop;
		char	*r;
	size_t		len = PATH_MAX;
#if	defined(DO_CHDIR_LONG)
#define	LWD_RETRY_MAX	8			/* stops at 128 * PATH_MAX */
	int		i = 0;
#endif

#if	defined(IS_SUN) && defined(__SVR4) && defined(DO_CHDIR_LONG)
retry:
#endif
	do {
		GROWSTAKL(dir, len);
		*dir = '\0';

		r = getcwd((char *)dir, (brkend - dir)-1);
#if	defined(DO_CHDIR_LONG)
		if (++i >= LWD_RETRY_MAX)	/* stops at 128 * PATH_MAX */
			break;
		len *= 2;
	} while (r == NULL && errno == ERANGE);
#else
	} while (0);
#endif

#if	defined(IS_SUN) && defined(__SVR4) && defined(DO_CHDIR_LONG)
	if (r == NULL && errno == ERANGE && i == LWD_RETRY_MAX) {
		/*
		 * Work around a Solaris kernel bug.
		 */
		sunos5_cwdfix();
		goto retry;
	}
#endif
	return ((unsigned char *)r);
}

#if	defined(HAVE_FCHDIR) && \
	(defined(DO_EXPAND_LONG) || defined(DO_CHDIR_LONG))
int
sh_hop_dirs(name, np)
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
			break;	/* No slash remains, return the prefix fd */
		}
		if ((dfd = openat(fd, p, O_SEARCH|O_DIRECTORY|O_NDELAY)) < 0) {
			err = errno;

			close(fd);
			if (err == EMFILE)
				errno = err;
			else
				errno = ENAMETOOLONG;
			*p2 = '/';
			return (dfd);
		}
		close(fd);	/* Don't care about AT_FDCWD, it is negative */
		fd = dfd;
		*p2++ = '/';	/* p2 is always != NULL here */
		while (*p2 == '/')
			p2++;
		p = p2;
	}
	*np = p;
	return (fd);
}
#endif

/*
 * A stat() implementation that is able to deal with ENAMETOOLONG.
 */
int
lstatat(name, buf, flag)
	char		*name;
	struct stat	*buf;
	int		flag;
{
#if	defined(HAVE_FCHDIR) && defined(ENAMETOOLONG) && defined(DO_CHDIR_LONG)
	char	*p;
	int	fd;
	int	err;
#endif
	int	ret;

	if ((ret = fstatat(AT_FDCWD, name, buf, flag)) < 0 &&
	    errno != ENAMETOOLONG) {
		return (ret);
	}

#if	defined(HAVE_FCHDIR) && defined(ENAMETOOLONG) && defined(DO_CHDIR_LONG)
	if (ret >= 0)
		return (ret);

	fd = sh_hop_dirs(name, &p);
	ret = fstatat(fd, p, buf, flag);
	err = errno;
	close(fd);		/* Don't care about AT_FDCWD, it is negative */
	errno = err;
#endif	/* HAVE_FCHDIR && ENAMETOOLONG && DO_CHDIR_LONG */
	return (ret);
}


/*
 * Try to update ncwdname.
 * Called by the "cd" builtin, in this case with "cwdbase" == NULL.
 *
 * Incorrect values will be detected by cwd2().
 *
 * If "cwdbase" == NULL, we replace $PWD.
 *
 * Otherwise we assume "dir" == "cwdbase" and normalize in place without
 * updating $PWD. This is used by cwdrel2abs().
 *
 * This function is based on string manipulation only.
 */
void
cwd(dir, cwdbase)
	unsigned char	*dir;		/* The new relative directory	   */
	unsigned char	*cwdbase;	/* Absolute directory base or NULL */
{
	unsigned char *pcwd;
	unsigned char *pdir;
	unsigned char *base = cwdbase;

	if (base == NULL) {		/* Compute new value on string stack */
		int	len;		/* and later install result as $PWD  */

		/*
		 * Create space for $PWD + "/" + length(dir)
		 * First step over curstak().
		 */
		len = length(curstak());
		pcwd = curstak() + len;

		len = length(ncwdname) + length(dir);
		pcwd += len;
		GROWSTAK(pcwd);
		pcwd -= len;
		cwdbase = pcwd;
		movstr(ncwdname, cwdbase);	/* Copy old $PWD to base */

	} else if (dir == cwdbase && *dir != '/') {
		/*
		 * In place operation only for absolute path names.
		 * In case of a relative path name, we need to call getcwd().
		 */
		didpwd = FALSE;
		return;
	}

	/*
	 * First remove extra /'s by squeezing the string.
	 */
	rmslash(dir);

	/*
	 * Now remove any .'s
	 */
	pdir = dir;
	if (*dir == SLASH)		/* skip initial slash */
		pdir++;
	while (*pdir) {			/* remove /./ by itself */
		if ((*pdir == DOT) && (*(pdir+1) == SLASH)) {
			movstr(pdir+2, pdir);
			continue;
		}
		pdir++;
		while ((*pdir) && (*pdir != SLASH))
			pdir++;
		if (*pdir)		/* skip next slash */
			pdir++;
	}
	/*
	 * Take care of trailing /.
	 */
	if (*(--pdir) == DOT && pdir > dir && *(--pdir) == SLASH) {
		if (pdir > dir) {
			*pdir = '\0';
		} else {
			*(pdir+1) = '\0';
		}

	}

	/*
	 * Remove extra /'s by squeezing the string.
	 */
	rmslash(dir);

	/*
	 * Now that the dir is canonicalized, process it
	 */
	if (*dir == DOT && *(dir+1) == '\0') {
		/*
		 * This was a chdir("."), so we do not need to change
		 * the $PWD variable.
		 */
		return;
	}


	if (*dir == SLASH) {
		/*
		 * Absolute path, overwrite cwdbase.
		 *
		 * The resulting string length will be less or equal to the
		 * string length of the "dir" argument.
		 */
		pcwd = cwdbase;
		*pcwd++ = *dir++;	/* Copy over initial '/' */
		didpwd = PARTLY;
	} else {
		/*
		 * Relative path, append to cwdbase (usually on string stack).
		 *
		 * This may cause an overflow in case there is not enough space
		 * but the resulting string length will be less or equal to the
		 * string length in "cwdbase" and "dir" plus 1 ('/').
		 */
		if (didpwd == FALSE)
			return;
		didpwd = PARTLY;
		pcwd = cwdbase + length(cwdbase) - 1;
		if (pcwd != cwdbase+1)
			*pcwd++ = SLASH;
	}
	while (*dir) {
		if (*dir == DOT &&
		    *(dir+1) == DOT &&
		    (*(dir+2) == SLASH || *(dir+2) == '\0')) {
			/* Parent directory, so backup one */

			if (pcwd > cwdbase+2)
				--pcwd;
			while (*(--pcwd) != SLASH)
				/* LINTED */
				;
			pcwd++;
			dir += 2;
			if (*dir == SLASH) {
				dir++;
			}
			continue;
		}
		*pcwd++ = *dir++;
		while ((*dir) && (*dir != SLASH)) {
			*pcwd++ = *dir++;
		}
		if (*dir) {
			*pcwd++ = *dir++;
		}
	}
	*pcwd = '\0';

	--pcwd;
	if (pcwd > cwdbase && *pcwd == SLASH) {
		/* Remove trailing / */

		*pcwd = '\0';
	}
	if (base == NULL)
		cwdnod(cwdbase);	/* Update $PWD */
}

/*
 * Verify that "ncwdname" is identical to "."
 * Set didpwd = FALSE in case of problems.
 * If cdflg is not CHDIR_L, check for symlinks in "ncwdname"
 * and set didpwd = FALSE in case a symlink was seen.
 */
static void
cwd2(cdflg)
	int	cdflg;
{
	struct stat stat1, stat2;
	/* check if there are any symbolic links in pathname */

	if (didpwd == FALSE)
		return;

	if (didpwd == PARTLY) {
		if (!(cdflg & CHDIR_L)) {
			if (!cwd3(&stat1))
				return;
		} else if (lstatat((char *)ncwdname, &stat1, 0) == -1) {
			didpwd = FALSE;
			return;
		}
	} else if (lstatat((char *)ncwdname, &stat1, 0) == -1) {
			didpwd = FALSE;
			return;
	}

	/*
	 * check if ino's and dev's match; pathname could
	 * consist of symbolic links with ".."
	 */
	if (stat(".", &stat2) == -1 ||
	    stat1.st_dev != stat2.st_dev ||
	    stat1.st_ino != stat2.st_ino)
		didpwd = FALSE;
}

/*
 * Check all pathname components for symlinks.
 * Set didpwd = FALSE in case a symlink was seen.
 */
static int
cwd3(statp)
	struct stat	*statp;
{
	unsigned char *pcwd = ncwdname + 1;

	while (*pcwd) {
		char c;
		while ((c = *pcwd++) != SLASH && c != '\0')
			/* LINTED */
			;
		*--pcwd = '\0';
		if (lstatat((char *)ncwdname, statp,
			    AT_SYMLINK_NOFOLLOW) == -1 ||
		    S_ISLNK(statp->st_mode)) {
			didpwd = FALSE;
			*pcwd = c;
			return (FALSE);
		}
		*pcwd = c;
		if (c)
			pcwd++;
	}
	didpwd = TRUE;
	return (TRUE);
}

#ifdef	DO_POSIX_CD
/*
 * Convert a relative pathname into an absolute patname when in -L mode.
 * The relative pathname is expected in curstak() and the result is
 * written to the same location.
 *
 * We are _always_ called with relative path names in curstak().
 *
 * This function is based only on string manipulation.
 */
int
cwdrel2abs()
{
	unsigned char	*p;
	unsigned char	*p2;
	int		odidpwd = didpwd;
	int		ret;

	/*
	 * Check whether there is no way to make it "right".
	 */
	if (didpwd == FALSE || ncwdname[0] != '/')
		return (FALSE);

	/*
	 * Create space for $PWD + "/" + curstak()
	 * Since we first step over curstak(), we need 2*ret.
	 */
	ret = length(curstak());
	p = curstak() + length(ncwdname) + 2*ret;
	GROWSTAK(p);

	/*
	 * Now concat $PWD + "/" + curstak() above current curstak().
	 */
	p = curstak() + ret;	/* Step over old curstak() content  */
	p2 = movstr(ncwdname, p); /* Copy current $PWD		    */
	*p2++ = '/';		/* Add '/'			    */
	movstr(curstak(), p2);	/* Add relative path from curstak() */

	/*
	 * Canonicalize new absolute path in place.
	 * The input value at curstak() is still valid.
	 */
	cwd(p, p);
	if (didpwd == FALSE) {
		/*
		 * TODO: Check whether a long path could be converted into
		 * an absolute path that is shorter.
		 */
		ret = FALSE;
		goto out;
	}
	ret = TRUE;
	movstr(p, curstak());			/* move result to curstak() */
out:
	didpwd = odidpwd;
	return (ret);
}
#endif

/*
 * Get the current working directory.
 * Mark didpwd that we have a real value.
 *
 * Externally used by the "cd" builtin to update PWD= and by xec.c when
 * setting up a new job slot.
 * Internally used by cwdset() below.
 */
unsigned char *
cwdget(cdflg)
	int	cdflg;
{
	unsigned char	*cwdptr = NULL;

	cwd2(cdflg);
	if (didpwd == FALSE) {
		if ((cwdptr = lgetcwd()) == NULL) {
			/*
			 * HP-UX-10.20 returns ENAMETOOLONG even though POSIX
			 * requires it to work with long path names.
			 * We check whether the value in "ncwdname" refers
			 * to "." and contains no symlinks. Since we normalize
			 * the path name before, this is as secure as a
			 * successful getcwd() call.
			 */
			didpwd = PARTLY;
			cwd2(CHDIR_P);
			if (didpwd == FALSE)
				*ncwdname = 0;
		} else {
			didpwd = TRUE;
		}
	}
	if (didpwd && cwdptr)
		cwdnod(cwdptr);
	else
		cwdnod(ncwdname);
	return (ncwdname);
}

/*
 * Set ncwdname (call cwdget()) if needed.
 *
 * Externally used by the "cd" builtin to update PWD=
 *
 * We use cwdget(CHDIR_L) as this does not modify PWD
 * unless it is definitely invalid.
 */
unsigned char *
cwdset()
{
	if (ncwdname[0] == '\0')
		cwdget(CHDIR_L);
	if (didpwd == FALSE)
		return (UC "");
	return (ncwdname);
}

/*
 *	Print the current working directory.
 *	Used by the "pwd" builtin.
 */
void
cwdprint(cdflg)
	int	cdflg;
{
	unsigned char	*cwdptr = ncwdname;
#ifdef	DO_POSIX_CD

	/*
	 * As cwdprint() is only used to print the current working directory,
	 * it should only update ncwdname[] in case it is definitely invalid.
	 * For this reason, we call cwd2(CHDIR_L) to just check whether the
	 * content does not point to the current working directory.
	 */
	cwd2(CHDIR_L);
	if (didpwd == FALSE || (didpwd == PARTLY && !(cdflg & CHDIR_L))) {

		if ((cwdptr = lgetcwd()) == NULL) {
			if (errno && errno != ERANGE)
				Error(badpwd);
			else
				Error(longpwd);
			return;
		}
		didpwd = TRUE;
	}
#else
	cwd2(cdflg);
	if (didpwd == FALSE) {
		if ((cwdptr = lgetcwd()) == NULL) {
			if (errno && errno != ERANGE)
				Error(badpwd);
			else
				Error(longpwd);
			return;
		}
		didpwd = TRUE;
	}
#endif

	prs_buff(cwdptr);
	prc_buff(NL);
	cwdnod(cwdptr);
}

/*
 *	This routine will remove repeated slashes from string.
 */
static void
rmslash(string)
	unsigned char *string;
{
	unsigned char *pstring;

	pstring = string;
	while (*pstring) {
		if (*pstring == SLASH && *(pstring+1) == SLASH) {
			/* Remove repeated SLASH's */

			movstr(pstring+1, pstring);
			continue;
		}
		pstring++;
	}

	--pstring;
	if (pstring > string && *pstring == SLASH) {
		/* Remove trailing / */

		*pstring = '\0';
	}
}

/*
 * Update OLDPWD= node
 */
void
ocwdnod()
{
#ifdef	DO_SYSPUSHD
	extern struct namnod opwdnod;

	if (opwdnod.namval != ocwdname)
		free(opwdnod.namval);
	if (opwdnod.namval != opwdnod.namenv && opwdnod.namenv != ocwdname)
		free(opwdnod.namenv);
	free(ocwdname);
	if (ncwdname[0] != '\0')
		ocwdname = make(ncwdname);
	else
		ocwdname = NULL;		/* Makes OLDPWD= disappear */
	opwdnod.namval = opwdnod.namenv = ocwdname;

	attrib(&opwdnod, N_ENVCHG);		/* Mark as changed */
#endif
}

/*
 * Update PWD= node
 */
static void
cwdnod(wdname)
	unsigned char	*wdname;
{
	extern struct namnod pwdnod;

	if (wdname == pwdnod.namval)		/* $PWD still intact */
		return;

	if (pwdnod.namval != ncwdname)
		free(pwdnod.namval);
	if (pwdnod.namval != pwdnod.namenv && pwdnod.namenv != ncwdname)
		free(pwdnod.namenv);

	if (wdname == ncwdname) {
		pwdnod.namval = pwdnod.namenv = ncwdname;
	} else {
		free(ncwdname);
		ncwdname = make(wdname);	/* Never returns NULL */
		pwdnod.namval = pwdnod.namenv = ncwdname;
	}
	attrib(&pwdnod, N_ENVCHG);		/* Mark as changed */
}

#ifdef	DO_SYSPUSHD
static	struct argnod *dirs;

struct argnod *
push_dir(name)
	unsigned char	*name;
{
	struct argnod	*ret;

	ret = (struct argnod *)alloc(length(name) + BYTESPERWORD);
	movstr(name, ret->argval);
	ret->argnxt = dirs;
	dirs = ret;
	return (ret);
}

struct argnod *
pop_dir(offset)
	int	offset;
{
	int		i = 0;
	struct argnod	*d = dirs;
	struct argnod	*prev = dirs;

	while (d && i++ != offset) {
		prev = d;
		d = d->argnxt;
	}
	if (!d)
		return (NULL);
	if (prev == d)		/* d == dirs */
		dirs = d->argnxt;
	else
		prev->argnxt = d->argnxt;
	return (d);
}

void
init_dirs()
{
	if (dirs)
		return;
	cwdset();
	push_dir(ncwdname);
}

int
pr_dirs(minlen, cdflg)
	int	minlen;
	int	cdflg;
{
	struct argnod	*d;

	if (!dirs)
		init_dirs();

	d = dirs;
	if (d->argnxt == NULL) {
		if (minlen == 0)
			cwdprint(cdflg);
		return (minlen == 0);
	}
	while (d) {
		prs_buff(d->argval);
		d = d->argnxt;
		if (d)
			prc_buff(' ');
	}
	prc_buff(NL);
	return (dirs->argnxt != NULL);
}
#endif	/* DO_SYSPUSHD */
