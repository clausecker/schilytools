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
 * This file contains modifications Copyright 2008-2014 J. Schilling
 *
 * @(#)pwd.c	1.19 15/09/14 2008-2014 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)pwd.c	1.19 15/09/14 2008-2014 J. Schilling";
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
#define	PARTLY	2

	void	cwd		__PR((unsigned char *dir));
static	void	cwd2		__PR((void));
	unsigned char *cwdget	__PR((void));
	void	cwdprint	__PR((void));
static void	rmslash		__PR((unsigned char *string));
static void	ocwdnod		__PR((void));
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
 * Incorrect values will be detected by cwd2().
 */
void
cwd(dir)
	unsigned char	*dir;
{
	unsigned char *pcwd;
	unsigned char *pdir;

	/* Update OLDPWD= to point to previous dir */

	ocwdnod();

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
 */
static void
cwd2()
{
	struct stat stat1, stat2;
	unsigned char *pcwd;
	/* check if there are any symbolic links in pathname */

	if (didpwd == FALSE)
		return;
	pcwd = cwdname + 1;
	if (didpwd == PARTLY) {
		while (*pcwd) {
			char c;
			while ((c = *pcwd++) != SLASH && c != '\0')
				/* LINTED */
				;
			*--pcwd = '\0';
			if (lstat((char *)cwdname, &stat1) == -1 ||
			    (stat1.st_mode & S_IFMT) == S_IFLNK) {
				didpwd = FALSE;
				*pcwd = c;
				return;
			}
			*pcwd = c;
			if (c)
				pcwd++;
		}
		didpwd = TRUE;
	} else
		if (stat((char *)cwdname, &stat1) == -1) {
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
 * Get the current working directory.
 * Mark didpwd that we have a real value.
 */
unsigned char *
cwdget()
{
	cwd2();
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
 */
unsigned char *
cwdset()
{
	if (cwdname[0] == '\0')
		cwdget();
	return (cwdname);
}

/*
 *	Print the current working directory.
 *	Used by the "pwd" builtin.
 */
void
cwdprint()
{
	unsigned char *cp;

	cwd2();
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

	for (cp = cwdname; *cp; cp++) {
		prc_buff(*cp);
	}

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
static void
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
pr_dirs(minlen)
	int	minlen;
{
	struct argnod	*d;

	if (!dirs)
		init_dirs();

	d = dirs;
	if (d->argnxt == NULL) {
		if (minlen == 0)
			cwdprint();
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
