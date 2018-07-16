/* @(#)xattr.c	1.20 18/07/12 Copyright 2003-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)xattr.c	1.20 18/07/12 Copyright 2003-2018 J. Schilling";
#endif
/*
 *	Handle Extended File Attributes on Linux
 *
 *	Copyright (c) 2003-2018 J. Schilling
 *	Thanks to Anreas Grünbacher <agruen@suse.de> for the
 *	first implemenation.
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
#include <schily/string.h>
#if defined(HAVE_SYS_XATTR_H)
#include <sys/xattr.h>
#else
#if defined(HAVE_ATTR_XATTR_H)
#include <attr/xattr.h>
#endif	/* HAVE_ATTR_XATTR_H */
#endif	/* HAVE_SYS_XATTR_H */
#include "star.h"
#include <schily/standard.h>
#include <schily/unistd.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include "starsubs.h"
#include "checkerr.h"

#if defined(USE_XATTR) && defined(HAVE_LISTXATTR) && defined(HAVE_GETXATTR)
/*
 * Use a global list of extended attributes -- FINFO structs have no
 * constructors and destructors.
 */
/*
 * It bad to see new global variables while we are working on a star library.
 */
LOCAL star_xattr_t	*static_xattr;
#endif

#ifdef	USE_SELINUX
extern	BOOL		selinux_enabled;
#endif

#if defined(HAVE_GETXATTR) && !defined(HAVE_LGETXATTR)
#define	lgetxattr	getxattr
#endif
#if defined(HAVE_SETXATTR) && !defined(HAVE_LSETXATTR)
#define	lsetxattr	setxattr
#endif
#if defined(HAVE_LISTXATTR) && !defined(HAVE_LLISTXATTR)
#define	llistxattr	listxattr
#endif

EXPORT void
opt_xattr()
{
#if defined(USE_XATTR) && \
	(defined(HAVE_LISTXATTR) || defined(HAVE_LLISTXATTR)) && \
	(defined(HAVE_GETXATTR) || defined(HAVE_LGETXATTR))
#if defined(HAVE_SETXATTR) || defined(HAVE_LSETXATTR)
	printf(" Linux-xattr");
#endif
#endif
}

EXPORT void
opt_selinux()
{
#ifdef	USE_SELINUX
	printf(" SELinux");
#endif
}

/* ARGSUSED */
EXPORT BOOL
get_xattr(info)
	register FINFO	*info;
{
#if defined(USE_XATTR) && \
	(defined(HAVE_LISTXATTR) || defined(HAVE_LLISTXATTR)) && \
	(defined(HAVE_GETXATTR) || defined(HAVE_LGETXATTR))
	ssize_t	list_len;
	size_t	size;
	char	*alist;
	char	*lp;
	int	count;
	int	i;

	free_xattr(&static_xattr);
	info->f_xflags &= ~XF_XATTR;
	info->f_xattr = NULL;

	list_len = llistxattr(info->f_sname, NULL, 0);
	if (list_len < 0) {
		if (!errhidden(E_GETXATTR, info->f_name)) {
			if (!errwarnonly(E_GETXATTR, info->f_name))
				xstats.s_getxattr++;
			errmsg("Cannot listxattr for '%s'.\n", info->f_name);
			(void) errabort(E_GETXATTR, info->f_name, TRUE);
		}
		return (FALSE);
	} else if (list_len == 0) {
		return (FALSE);
	}
	alist = ___malloc(list_len+2, "extended attribute");
	list_len = llistxattr(info->f_sname, alist, list_len);
	if (list_len < 0) {
		if (!errhidden(E_GETXATTR, info->f_name)) {
			if (!errwarnonly(E_GETXATTR, info->f_name))
				xstats.s_getxattr++;
			errmsg("Cannot listxattr for '%s'.\n", info->f_name);
			(void) errabort(E_GETXATTR, info->f_name, TRUE);
		}
		goto fail;
	}
	alist[list_len] = alist[list_len+1] = '\0';

	/*
	 * Count the number of attributes
	 */
	for (lp = alist, count = 0; lp - alist < list_len;
						lp = strchr(lp, '\0')+1) {
		if (*lp == '\0' ||
		    strncmp(lp, "system.", 7) == 0 ||
		    strncmp(lp, "xfsroot.", 8) == 0)
			continue;
		count++;
	}

	if (count == 0)
		goto fail;  /* not really a failure, but... */

	size = (count+1) * sizeof (star_xattr_t);
	static_xattr = ___malloc(size, "extended attribute");
	fillbytes(static_xattr, size, '\0');

	for (lp = alist, i = 0; lp - alist < list_len;
						lp = strchr(lp, '\0')+1) {
		ssize_t	len;

		if (*lp == '\0' ||
		    strncmp(lp, "system.", 7) == 0 ||
		    strncmp(lp, "xfsroot.", 8) == 0)
			continue;

		static_xattr[i].name = ___malloc(strlen(lp)+1,
						"extended attribute");
		static_xattr[i].value = NULL;
		strcpy(static_xattr[i].name, lp);

		len = lgetxattr(info->f_sname, lp, NULL, 0);
		if (len < 0) {
			if (!errhidden(E_GETXATTR, info->f_name)) {
				if (!errwarnonly(E_GETXATTR, info->f_name))
					xstats.s_getxattr++;
				errmsg("Cannot getxattr for '%s'.\n",
							info->f_name);
				(void) errabort(E_GETXATTR, info->f_name,
								TRUE);
			}
			goto fail2;
		}
		static_xattr[i].value_len = len;
		static_xattr[i].value = ___malloc(len, "extended attribute");
		len = lgetxattr(info->f_sname, lp, static_xattr[i].value, len);
		if (len < 0) {
			if (!errhidden(E_GETXATTR, info->f_name)) {
				if (!errwarnonly(E_GETXATTR, info->f_name))
					xstats.s_getxattr++;
				errmsg("Cannot getxattr for '%s'.\n",
							info->f_name);
				(void) errabort(E_GETXATTR, info->f_name,
								TRUE);
			}
			goto fail2;
		}
		i++;
	}

	free(alist);
	info->f_xflags |= XF_XATTR;
	info->f_xattr = static_xattr;
	return (TRUE);

fail2:
	for (; i >= 0; i--) {
		free(static_xattr[i].name);
		if (static_xattr[i].value != NULL)
			free(static_xattr[i].value);
	}
	free(static_xattr);
	static_xattr = NULL;
fail:
	free(alist);
	return (FALSE);
#else  /* USE_XATTR */
	return (TRUE);
#endif  /* USE_XATTR */
}

/* ARGSUSED */
EXPORT BOOL
set_xattr(info)
	register FINFO	*info;
{
#if defined(USE_XATTR) && \
	(defined(HAVE_SETXATTR) || defined(HAVE_LSETXATTR))
	star_xattr_t	*xap;

	if (info->f_xattr == NULL || (info->f_xflags & XF_XATTR) == 0)
		return (TRUE);

	for (xap = info->f_xattr; xap->name != NULL; xap++) {
#ifdef	USE_SELINUX
		if (selinux_enabled &&
		    (strcmp(xap->name, "security.selinux") == 0))
			continue;
#endif
		if (lsetxattr(info->f_name, xap->name, xap->value,
		    xap->value_len, 0) != 0) {
			if (!errhidden(E_SETXATTR, info->f_name)) {
				if (!errwarnonly(E_SETXATTR, info->f_name))
					xstats.s_setxattr++;
				errmsg("Cannot setxattr for '%s'.\n",
							info->f_name);
				(void) errabort(E_SETXATTR, info->f_name,
								TRUE);
			}
			return (FALSE);
		}
	}
#endif  /* USE_XATTR */
	return (TRUE);
}

#ifdef	USE_SELINUX
EXPORT BOOL
setselinux(info)
	register FINFO	*info;
{
	star_xattr_t	*xap;

	if (info->f_xattr == NULL || (info->f_xflags & XF_XATTR) == 0) {
		if (setfscreatecon(NULL) < 0)
			goto err;
		return (TRUE);
	}

	for (xap = info->f_xattr; xap->name != NULL; xap++) {
		if (strcmp(xap->name, "security.selinux") == 0) {
			if (setfscreatecon(xap->value) < 0)
				goto err;
			return (TRUE);
		}
	}
	/*
	 * There was no "security.selinux" label, so we need to clear
	 * the context.
	 */
	if (setfscreatecon(NULL) < 0)
		goto err;
	return (TRUE);
err:
	errmsg("Cannot setup security context for '%s'.\n", info->f_name);
	return (FALSE);
}
#endif	/* USE_SELINUX */

EXPORT void
free_xattr(xattr)
	star_xattr_t	**xattr;
{
#ifdef USE_XATTR
	star_xattr_t	*xap;

	if (*xattr == NULL)
		return;

	for (xap = *xattr; xap->name != NULL; xap++) {
		free(xap->name);
		free(xap->value);
	}
	free(*xattr);
	*xattr = NULL;

#endif  /* USE_XATTR */
}
