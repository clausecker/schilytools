/* @(#)resolvepath.c	1.3 11/11/08 Copyright 2011 J. Schilling */
/*
 *	resolvepath() removes "./" and non-leading "/.." path components.
 *	It tries to do the same as the Solaris syscall with the same name.
 *
 *	Copyright (c) 2011 J. Schilling
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

#include <schily/types.h>
#include <schily/stat.h>
#include <schily/errno.h>
#include <schily/maxpath.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/schily.h>

#ifndef	HAVE_RESOLVEPATH
EXPORT	int	resolvepath	__PR((const char *path, char *buf,
					size_t bufsiz));
#endif
EXPORT	int	resolvenpath	__PR((const char *path, char *buf,
					size_t bufsiz));
LOCAL	int	pathresolve	__PR((const char *path, const char *p,
					char *buf, char *b,
					size_t bufsiz, int flags));
LOCAL	int	shorten		__PR((char *path));

#define	RF_EXIST	0x01	/* All path components must exist */

#ifndef	HAVE_RESOLVEPATH
EXPORT int
resolvepath(path, buf, bufsiz)
	register const char	*path;
			char	*buf;
			size_t	bufsiz;
{
	return (pathresolve(path, path, buf, buf, bufsiz, RF_EXIST));
}
#endif

EXPORT int
resolvenpath(path, buf, bufsiz)
	register const char	*path;
			char	*buf;
			size_t	bufsiz;
{
	return (pathresolve(path, path, buf, buf, bufsiz, 0));
}


LOCAL int
pathresolve(path, p, buf, b, bufsiz, flags)
	register const char	*path;
	register const char	*p;
			char	*buf;
	register	char	*b;
			size_t	bufsiz;
			int	flags;
{
register 	char	*e;
		int	len = 0;
		struct stat rsb;
		struct stat sb;

	if (path == NULL || buf == NULL) {
		seterrno(EFAULT);
		return (-1);
	}
	if (*path == '\0') {
		seterrno(EINVAL);
		return (-1);
	}

	if (bufsiz <= 0)
		return (0);
	if (p == path && p[0] == '/')
		*b++ = *p++;

	rsb.st_ino = 0;
	rsb.st_nlink = 0;

	while (*p) {
		if ((b - buf) >= bufsiz) {
			buf[bufsiz-1] = '\0';	/* bufziz > 0 tested before */
			return (bufsiz);
		}
		*b = '\0';

		if (p[0] == '/') {
			p += 1;
			continue;
		}
		if (p[0] == '.' &&
		    (p[1] == '/' || p[1] == '\0')) {
			if (p == path && p[1] == '\0')
				return (strlcpy(buf, ".", bufsiz));
			if (p[1] == '\0')
				p += 1;
			else
				p += 2;
			continue;
		}

		if (p[0] == '.' && p[1] == '.' &&
		    (p[2] == '/' || p[2] == '\0')) {
			if (p[2] == '\0')
				p += 2;
			else
				p += 3;
			if (!shorten(buf)) {
				if (strlcat(buf,
					buf[0]?"/..":"..", bufsiz) >= bufsiz) {
					return (bufsiz);
				}
			}
			b = buf + strlen(buf);
			if (stat(buf, &sb) < 0) {
				return (-1);
			}
			if (rsb.st_ino == 0 || rsb.st_nlink == 0) {
				if (stat("/", &rsb) < 0)
					return (-1);
			}
			if (sb.st_dev == rsb.st_dev &&
			    sb.st_ino == rsb.st_ino) {
				if (strlcpy(buf, "/", bufsiz) >= bufsiz) {
					return (bufsiz);
				}
			}
			continue;
		}

		if (b > &buf[1] || (b == &buf[1] && buf[0] != '/'))
			*b++ = '/';
		if ((b - buf) >= bufsiz) {
			buf[bufsiz-1] = '\0';	/* bufziz > 0 tested before */
			return (bufsiz);
		}
		e = strchr(p, '/');
		if (e)
			len = e - p;
		else
			len = strlen(p);
		if (++len > (bufsiz - (b - buf))) { /* Add one for strlcpy() */
			len = bufsiz - (b - buf);
			strlcpy(b, p, len);
			return (bufsiz);
		}

		strlcpy(b, p, len);
		p += len - 1;
		if (lstat(buf, &sb) < 0) {
			if (flags & RF_EXIST)
				return (-1);
			sb.st_mode = S_IFREG;
		}
		if (S_ISLNK(sb.st_mode)) {
			char	lname[PATH_MAX+1];

			len = readlink(buf, lname, sizeof (lname));
			if (len < 0) {
				return (-1);
			}
			if (len < sizeof (lname))
				lname[len] = '\0';
			else
				lname[sizeof (lname)-1] = '\0';
			*b = '\0';
			len += strlen(buf) + 1;
			{
#ifdef	HAVE_DYN_ARRAYS
			char bx[len];
#else
			char *bx = malloc(len);
			if (bx == NULL)
				return (-1);
#endif
			strcatl(bx, buf, lname, (char *)0);
			if (b > &buf[1] && b[-1] == '/')
				--b;
			len = pathresolve(bx, bx + (b - buf), buf, b,
								bufsiz, flags);
#ifndef	HAVE_DYN_ARRAYS
			free(bx);
#endif
			}
			if (len < 0) {
				return (-1);
			}
			b = buf + len;
		} else {
			b += len - 1;
		}
		if (e == NULL)
			break;
	}

	if ((b - buf) >= bufsiz) {
		buf[bufsiz-1] = '\0';		/* bufziz > 0 tested before */
		return (bufsiz);
	}
	buf[(b - buf)] = '\0';
	return (b - buf);
}

/*
 * Removes last path name component.
 * Returns FALSE if path could not be shortened.
 * Does not remove path components if already at root direcory.
 */
LOCAL int
shorten(path)
	register	char	*path;
{
	register	char	*p = path;


	if (p[0] == '\0')			/* Cannot shorten empty path */
		return (FALSE);			/* Leave empty to add ".."   */
	if (p[0] == '.' && p[1] == '\0') {	/* Path is just "."	    */
		*p = '\0';			/* Prepare to add ".."	    */
		return (FALSE);
	}

	for (p = path; *p++ != '\0'; );
	while (p > path) {
		if (*--p == '/')
			break;
	}

	if (p[0] == '.' && p[1] == '.' && p[2] == '\0') {
		return (FALSE);
	}
	if (p[0] == '/' && p[1] == '.' && p[2] == '.' && p[3] == '\0') {
		return (FALSE);
	}

	if (p == path && p[0] == '/')
		p++;
	else if (p == path)
		*p++ = '.';
	*p = '\0';
	return (TRUE);
}
