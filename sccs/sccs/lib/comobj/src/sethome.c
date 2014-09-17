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
/*
 *	Find the "home" directory for a "File Set"
 *
 *	Search for $SET_HOME/.sccs
 *
 * @(#)sethome.c	1.3 14/08/05 Copyright 2011-2014 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)sethome.c	1.3 14/08/05 Copyright 2011-2014 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)sethome.c"
#pragma ident	"@(#)sccs:lib/mpwlib/sethome.c"
#endif
#include	<defines.h>

/*
 * If it is not possible to retrieve "setahome", it may be a NULL pointer.
 * The variables "setrhome" and "cwdprefix" are always set from sethome().
 */
char	*setrhome;	/* Relative path to the project set home directory */
char	*setahome;	/* Absolute path to the project set home directory */
char	*cwdprefix;	/* Prefix from project set home directory to cwd */
int	homedist;	/* The # of directories to the project set home dir */

LOCAL	int	searchabs	__PR((char *path));
LOCAL	int	dorel		__PR((char *bp, size_t len));
LOCAL	void	mkrelhome	__PR((char *bp, size_t len, int i));
LOCAL	int	mkprefix	__PR((char *bp, size_t len, int i));

/*
 * Try to find the project set home directory and a path from that directory
 * to the current directory.
 */
EXPORT int
sethome()
{
	char	buf[max(8192, PATH_MAX+1)];
	int	len;
static	int	shinit;

	if (shinit != 0)
		return (0);
	shinit = 1;

	if (abspath(".", buf, sizeof (buf)) == NULL)
		return (dorel(buf, sizeof (buf)));

	errno = 0;
	len = searchabs(buf);
	if (len < 0) {
		if (errno != 0)
			return (-1);
		return (0);
	}

	buf[len] = '\0';
	if (len == 0)
		setahome = strdup("/");
	else
		setahome = strdup(buf);
	cwdprefix = strdup(&buf[len+1]);
	if (setahome == NULL || cwdprefix == NULL)	/* no mem */
		return (-1);
	mkrelhome(buf, sizeof (buf), homedist);
	if (setrhome == NULL)				/* no mem */
		return (-1);
	return (1);
}

/*
 * Search for the directory $SET_HOME/.sccs using the absolute path name to the
 * the current directory.
 */
LOCAL int
searchabs(path)
	char	*path;
{
	struct stat sb;
	char	buf[max(8192, PATH_MAX+1)];
	char	*p;
	int	len;
	int	err = 0;
	int	i = 0;
	int	found = 0;

	strlcpy(buf, path, sizeof (buf));
	len = strlen(buf);
	p = &buf[len];

	while (p >= buf) {
		strlcpy(p, "/.sccs", sizeof (buf) - (p - buf));
		found = 0;
		if (stat(buf, &sb) >= 0) {
			found++;
			break;
		} else if (errno != ENOENT) {
			err = errno;
		}
		if (p == buf)
			break;
		while (p > buf) {
			if (*--p == '/')
				break;
		}
		i++;
	}
	if (!found) {
		if (err)
			errno = err;
		else
			errno = 0;
		return (-1);
	}

	homedist = i;

	return (p - buf);
}

/*
 * Search for the directory $SET_HOME/.sccs using the relative path names from
 * the current directory.
 */
LOCAL int
dorel(bp, len)
	char	*bp;
	size_t	len;
{
	struct stat sb;
	struct stat sb2;
	char	*p;
	char	*s;
	int	err = 0;
	int	i;
	int	found = 0;

	bp[0] = '\0';
	for (i = (len - 6) / 3; --i >= 0; ) {
		strlcat(bp, "../", len);
	}
	strlcat(bp, ".sccs", len);
	p = bp + strlen(bp);
	p -= 5;
	s = p + 1;
	i = 0;
	while (p >= bp) {
		found = 0;
		if (stat(p, &sb) >= 0) {
			found++;
			break;
		} else if (errno != ENOENT) {
			err = errno;
			break;
		}
		*s = '\0';
		sb.st_ino = (ino_t)-1;
		sb2.st_ino = (ino_t)-2;
		stat(p, &sb);		/* "." */
		p -= 3;
		stat(p, &sb2);		/* ".." */
		if (sb.st_ino == sb2.st_ino &&
		    sb.st_dev == sb2.st_dev) {
			break;
		}
		*s = 's';
		i++;
	}
	if (!found) {
		if (err) {
			errno = err;
			return (-1);
		}
		return (0);
	}
	errno = 0;
	if (!mkprefix(bp, len, i))
		return (-1);
	if (cwdprefix == NULL)				/* no mem */
		return (-1);
	homedist = i;
	mkrelhome(bp, len, homedist);
	if (setrhome == NULL)				/* no mem */
		return (-1);
	return (1);
}

/*
 * Create a relative path from the current directory to the
 * "project set home" directory. The input is the number of directories
 * betweem the current dir and the project set home directory.
 */
LOCAL void
mkrelhome(bp, len, n)
	char	*bp;
	size_t	len;
	int	n;
{
	bp[0] = '\0';
	if (n == 0) {
		strlcat(bp, ".", len);
	} else {
		while (--n > 0) {
			strlcat(bp, "../", len);
		}
		strlcat(bp, "..", len);
	}
	setrhome = strdup(bp);
}

/*
 * Create the path prefix from the "project set home" directory to the current
 * directory. The input is the number of directories betweem the current dir
 * and the project set home directory. We implement a partial getcwd() by
 * scanning directories and matching inode numbers for ".".
 */
LOCAL int
mkprefix(bp, len, n)
	char	*bp;
	size_t	len;
	int	n;
{
	struct stat sb;
	struct stat sb2;
	char	buf[max(8192, PATH_MAX+1)];
	char	pfx[max(8192, PATH_MAX+1)];
	char	*p;
	int	i;
	int	found;
	DIR	*dp;
	struct dirent *de;

	bp[0] = '\0';
	i = n;
	for (; --i >= 0; ) {
		strlcat(bp, "../", len);
	}
	strlcat(bp, ".", len);

	i = n;
	pfx[0] = '\0';
	while (--i >= 0) {

		stat(&bp[(n-i)*3], &sb);	/* Dir to match */
		dp = opendir(&bp[(n-i-1)*3]);	/* one level above that dir */

		strlcpy(buf, &bp[(n-i-1)*3], sizeof (buf));
		p = buf + strlen(buf)-1;
		found = 0;
		while ((de = readdir(dp)) != NULL) {
			if (strcmp(de->d_name, ".") == 0 ||
			    strcmp(de->d_name, "..") == 0)
				continue;
			strlcpy(p, de->d_name, sizeof (buf) - (p - buf));
			stat(buf, &sb2);
			if (sb.st_ino == sb2.st_ino &&
			    sb.st_dev == sb2.st_dev) {
				found++;
				break;
			}
		}
		if (!found)
			return (0);
		if (pfx[0])
			strlcat(pfx, "/", sizeof (pfx));
		strlcat(pfx, sname(buf), sizeof (pfx));
	}
	cwdprefix = strdup(pfx);
	return (1);
}
