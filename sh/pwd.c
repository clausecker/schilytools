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
 * Copyright 2008-2016 J. Schilling
 *
 * @(#)pwd.c	1.25 16/05/19 2008-2016 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)pwd.c	1.25 16/05/19 2008-2016 J. Schilling";
#endif

/*
 *	UNIX shell
 */
#ifdef	SCHILY_INCLUDES
#include	<schily/errno.h>
#include	<schily/types.h>
#include	<schily/stat.h>
#include	<schily/getcwd.h>
#else
#include	"mac.h"
#include	<errno.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<limits.h>
#endif

#define	DOT		'.'
#ifndef	NULL
#define	NULL	0
#endif
#define	SLASH	'/'
#define	PARTLY	2		/* cwdname may contain symlinks */

	void	cwd		__PR((unsigned char *dir));
static	void	cwd2		__PR((int cdflg));
static	int	cwd3		__PR((struct stat *statp));
#ifdef	DO_POSIX_CD
	int	cwdrel2abs	__PR((void));
#endif
	unsigned char *cwdget	__PR((int cdflg));
	void	cwdprint	__PR((int cdflg));
static void	rmslash		__PR((unsigned char *string));
	void	ocwdnod		__PR((void));
static void	cwdnod		__PR((void));

#ifdef __STDC__
extern const char	longpwd[];
#else
extern char	longpwd[];
#endif

unsigned char *ocwdname;
unsigned char cwdname[PATH_MAX+1];

static int	didpwd = FALSE;

/*
 * Try to update cwdname.
 * Called by the "cd" builtin.
 *
 * Incorrect values will be detected by cwd2().
 */
void
cwd(dir)
	unsigned char	*dir;
{
	unsigned char *pcwd;
	unsigned char *pdir;

	/* First remove extra /'s */

	rmslash(dir);

	/* Now remove any .'s */

	pdir = dir;
	if (*dir == SLASH)
		pdir++;
	while (*pdir)			/* remove /./ by itself */
	{
		if ((*pdir == DOT) && (*(pdir+1) == SLASH))
		{
			movstr(pdir+2, pdir);
			continue;
		}
		pdir++;
		while ((*pdir) && (*pdir != SLASH))
			pdir++;
		if (*pdir)
			pdir++;
	}
	/* take care of trailing /. */
	if (*(--pdir) == DOT && pdir > dir && *(--pdir) == SLASH) {
		if (pdir > dir) {
			*pdir = '\0';
		} else {
			*(pdir+1) = '\0';
		}

	}

	/* Remove extra /'s */

	rmslash(dir);

	/* Now that the dir is canonicalized, process it */

	if (*dir == DOT && *(dir+1) == '\0')
	{
		return;
	}


	if (*dir == SLASH)
	{
		/* Absolute path */

		pcwd = cwdname;
		*pcwd++ = *dir++;
		didpwd = PARTLY;
	}
	else
	{
		/* Relative path */

		if (didpwd == FALSE)
			return;
		didpwd = PARTLY;
		pcwd = cwdname + length(cwdname) - 1;
		if (pcwd != cwdname+1)
			*pcwd++ = SLASH;
	}
	while (*dir)
	{
		if (*dir == DOT &&
		    *(dir+1) == DOT &&
		    (*(dir+2) == SLASH || *(dir+2) == '\0'))
		{
			/* Parent directory, so backup one */

			if (pcwd > cwdname+2)
				--pcwd;
			while (*(--pcwd) != SLASH)
				/* LINTED */
				;
			pcwd++;
			dir += 2;
			if (*dir == SLASH)
			{
				dir++;
			}
			continue;
		}
		if (pcwd >= &cwdname[PATH_MAX+1])
		{
			didpwd = FALSE;
			return;
		}
		*pcwd++ = *dir++;
		while ((*dir) && (*dir != SLASH))
		{
			if (pcwd >= &cwdname[PATH_MAX+1])
			{
				didpwd = FALSE;
				return;
			}
			*pcwd++ = *dir++;
		}
		if (*dir)
		{
			if (pcwd >= &cwdname[PATH_MAX+1])
			{
				didpwd = FALSE;
				return;
			}
			*pcwd++ = *dir++;
		}
	}
	if (pcwd >= &cwdname[PATH_MAX+1])
	{
		didpwd = FALSE;
		return;
	}
	*pcwd = '\0';

	--pcwd;
	if (pcwd > cwdname && *pcwd == SLASH)
	{
		/* Remove trailing / */

		*pcwd = '\0';
	}
}

/*
 * Verify that "cwdname" is identical to "."
 * Set didpwd = FALSE in case of problems.
 * If cdflg is not CHDIR_L, check for symlinks in "cwdname"
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
		} else if (stat((char *)cwdname, &stat1) == -1) {
			didpwd = FALSE;
			return;
		}
	} else if (stat((char *)cwdname, &stat1) == -1) {
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
	unsigned char *pcwd = cwdname + 1;

	while (*pcwd) {
		char c;
		while ((c = *pcwd++) != SLASH && c != '\0')
			/* LINTED */
			;
		*--pcwd = '\0';
		if (lstat((char *)cwdname, statp) == -1 ||
		    (statp->st_mode & S_IFMT) == S_IFLNK) {
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
 */
int
cwdrel2abs()
{
	unsigned char	cwdnm[PATH_MAX+1];
	unsigned char	*p;
	unsigned char	*p2;
	int		odidpwd = didpwd;
	int		ret;

	/*
	 * Check whether there is no way to make it "right".
	 */
	if (didpwd == FALSE || cwdname[0] == '\0')
		return (FALSE);

	/*
	 * cwdname is granted to be null terminated after the above test.
	 */
	movstr(cwdname, cwdnm);			/* save old value */

	/*
	 * Create space for $PWD + "/" + curstak()
	 */
	ret = length(curstak());
	p = curstak() + length(cwdname) + ret;
	GROWSTAK(p);

	/*
	 * Now concat $PWD + "/" + curstak() above current curstak().
	 */
	p = curstak() + ret;
	p2 = movstr(cwdname, p);
	*p2++ = '/';
	movstr(curstak(), p2);

	/*
	 * Canonicalize new absolute path.
	 * The input value at curstak() is still valid.
	 */
	cwd(p);
	if (didpwd == FALSE) {
		/*
		 * TODO: Check whether a long path could be converted into
		 * an absolute path that is shorter.
		 */
		ret = FALSE;
		goto out;
	}
	ret = TRUE;
	movstr(cwdname, curstak());		/* move result to curstak() */
out:
	movstr(cwdnm, cwdname);			/* restore old cwdname	*/
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
	cwd2(cdflg);
	if (didpwd == FALSE) {
		if (getcwd((char *)cwdname, PATH_MAX+1) == NULL)
			*cwdname = 0;
		didpwd = TRUE;

	}
	cwdnod();
	return (cwdname);
}

/*
 * Set cwdname (call cwdget()) if needed.
 *
 * We use cwdget(CHDIR_L) as this does not modify PWD
 * unless it is definitely invalid.
 */
unsigned char *
cwdset()
{
	if (cwdname[0] == '\0')
		cwdget(CHDIR_L);
	return (cwdname);
}

/*
 *	Print the current working directory.
 *	Used by the "pwd" builtin.
 */
void
cwdprint(cdflg)
	int	cdflg;
{
	unsigned char	*cwdptr = cwdname;
#ifdef	DO_POSIX_CD
	unsigned char	cwdnm[PATH_MAX+1];

	/*
	 * As cwdprint() is only used to print the current working directory,
	 * it should only update cwdname[] in case it is definitely invalid.
	 * For this reason, we call cwd2(CHDIR_L) to just check whether the
	 * content does not point to the current working directory.
	 */
	cwd2(CHDIR_L);
	if (didpwd == FALSE || (didpwd == PARTLY && !(cdflg & CHDIR_L))) {

		if (getcwd((char *)cwdnm, PATH_MAX+1) == NULL) {
			if (errno && errno != ERANGE)
				Error(badpwd);
			else
				Error(longpwd);
			return;
		}
		if (didpwd == FALSE) {
			movstr(cwdnm, cwdname);
			didpwd = TRUE;
		} else {
			cwdptr = cwdnm;
		}
	}
#else
	cwd2(cdflg);
	if (didpwd == FALSE) {
		if (getcwd((char *)cwdname, PATH_MAX+1) == NULL) {
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
	cwdnod();
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
	while (*pstring)
	{
		if (*pstring == SLASH && *(pstring+1) == SLASH)
		{
			/* Remove repeated SLASH's */

			movstr(pstring+1, pstring);
			continue;
		}
		pstring++;
	}

	--pstring;
	if (pstring > string && *pstring == SLASH)
	{
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
	if (opwdnod.namenv != ocwdname)
		free(opwdnod.namenv);
	free(ocwdname);
	if (cwdname[0] != '\0')
		ocwdname = make(cwdname);
	else
		ocwdname = NULL;		/* Makes OLDPWD= disappear */
	opwdnod.namval = opwdnod.namenv = ocwdname;
#endif
}

/*
 * Update PWD= node
 */
static void
cwdnod()
{
	extern struct namnod pwdnod;

	if (pwdnod.namval != cwdname)
		free(pwdnod.namval);
	if (pwdnod.namenv != cwdname)
		free(pwdnod.namenv);
	pwdnod.namval = pwdnod.namenv = cwdname;
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
	push_dir(cwdname);
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
