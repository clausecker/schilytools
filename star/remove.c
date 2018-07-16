/* @(#)remove.c	1.58 18/07/15 Copyright 1985, 1991-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)remove.c	1.58 18/07/15 Copyright 1985, 1991-2018 J. Schilling";
#endif
/*
 *	remove files an file trees
 *
 *	Copyright (c) 1985, 1992-2018 J. Schilling
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
#include <schily/standard.h>
#include <schily/fcntl.h>	/* Wegen AT_REMOVEDIR */
#include "star.h"
#include "table.h"
#include <schily/dirent.h>	/* XXX Wegen S_IFLNK */
#include <schily/unistd.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/errno.h>
#include "starsubs.h"


extern	FILE	*tty;
extern	FILE	*vpr;
extern	BOOL	interactive;
extern	BOOL	force_remove;
extern	BOOL	ask_remove;
extern	BOOL	remove_first;
extern	BOOL	remove_recursive;
extern	BOOL	keep_nonempty_dirs;

EXPORT	BOOL	remove_file	__PR((char *name, BOOL isfirst));
LOCAL	BOOL	_remove_file	__PR((char *name, BOOL isfirst, int depth));
LOCAL	BOOL	remove_tree	__PR((char *name, BOOL isfirst, int depth));

/*
 * Remove a file or a directory tree.
 * If star has been called with -remove-first "isfirst" needs to be set.
 */
EXPORT BOOL
remove_file(name, isfirst)
	register char	*name;
		BOOL	isfirst;
{
	static	int	depth	= -10;
	static	int	dinit	= 0;

	if (!dinit) {
#ifdef	_SC_OPEN_MAX
		depth += sysconf(_SC_OPEN_MAX);
#else
		depth += getdtablesize();
#endif
		dinit = 1;
	}
	return (_remove_file(name, isfirst, depth));
}

LOCAL BOOL
_remove_file(name, isfirst, depth)
	register char	*name;
		BOOL	isfirst;
		int	depth;
{
	char	buf[32];
	char	ans = '\0';
	int	err = EX_BAD;
	BOOL	fr_save = force_remove;
	BOOL	rr_save = remove_recursive;
	BOOL	ret;

	if (remove_first && !isfirst)
		return (FALSE);
	if (!force_remove && (interactive || ask_remove)) {
		fgtprintf(vpr, "remove '%s' ? Y(es)/N(o) :", name); fflush(vpr);
		fgetline(tty, buf, 2);
	}
	if (force_remove ||
	    ((interactive || ask_remove) && (ans = toupper(buf[0])) == 'Y')) {

		/*
		 * only unlink non directories or empty directories
		 */
		if (lunlinkat(name, AT_REMOVEDIR) < 0) {	/* rmdir() */
			err = geterrno();
			if (err == ENOTDIR) {
				if (lunlinkat(name, 0) < 0) {
					err = geterrno();
#ifdef	RM_DEBUG
					errmsg("rmdir: Not a dir but cannot unlink file.\n");
#endif
					goto cannot;
				}
				return (TRUE);
			}
#if defined(ENOTEMPTY) && ENOTEMPTY != EEXIST
			if (err == EEXIST || err == ENOTEMPTY) {
#else
			if (err == EEXIST) {
#endif
				if (!remove_recursive) {
					if (ans == 'Y') {
						fgtprintf(vpr,
						"Recursive remove nonempty '%s' ? Y(es)/N(o) :",
							name);
						fflush(vpr);
						fgetline(tty, buf, 2);
						if (toupper(buf[0]) == 'Y') {
							force_remove = TRUE;
							remove_recursive = TRUE;
						} else {
							goto nonempty;
						}
					} else {
				nonempty:
						if (!keep_nonempty_dirs)
						errmsgno(err,
						"Nonempty directory '%s' not removed.\n",
						name);
						return (FALSE);
					}
				}
				ret = remove_tree(name, isfirst, depth);

				force_remove = fr_save;
				remove_recursive = rr_save;
				return (ret);
			}
#ifdef	RM_DEBUG
			else {
				errmsgno(err, "rmdir: err != EEXIST.\n");
			}
			errmsgno(err, "rmdir: unknown reason.\n");
#endif
			goto cannot;
		}
		return (TRUE);
	}
#ifdef	RM_DEBUG
	else {
		errmsgno(EX_BAD, "remove_file: if (force_remove || .... else part\n");
		errmsgno(EX_BAD, "force_remove %d interactive %d ask_remove %d ans '%c' (%d)\n",
			force_remove, interactive, ask_remove, ans, ans);
	}
#endif
cannot:
	errmsgno(err, "File '%s' not removed.\n", name);
	return (FALSE);
}

LOCAL BOOL
remove_tree(name, isfirst, depth)
	register char	*name;
		BOOL	isfirst;
		int	depth;
{
	DIR		*d;
	struct dirent	*dir;
	BOOL		ret = TRUE;
	char		xn[PATH_MAX];	/* XXX A bad idea for a final solution */
	char		*p;

	if ((d = lopendir(name)) == NULL) {
		return (FALSE);
	}
	depth--;

	strcpy(xn, name);
	p = &xn[strlen(name)];
	*p++ = '/';

	while ((dir = readdir(d)) != NULL) {

		if (streql(dir->d_name, ".") ||
				streql(dir->d_name, ".."))
			continue;

		strcpy(p, dir->d_name);

		if (depth <= 0) {
			closedir(d);
		}
		if (!_remove_file(xn, isfirst, depth))
			ret = FALSE;
		if (depth <= 0 && (d = lopendir(name)) == NULL) {
			return (FALSE);
		}
	}

	closedir(d);

	if (ret == FALSE)
		return (ret);

	if (lunlinkat(name, AT_REMOVEDIR) >= 0)	/* rmdir() */
		return (ret);

	errmsg("Directory '%s' not removed.\n", name);
	return (FALSE);
}
