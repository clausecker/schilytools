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
 * @(#)sethome.c	1.13 20/05/17 Copyright 2011-2020 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)sethome.c	1.13 20/05/17 Copyright 2011-2020 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)sethome.c"
#pragma ident	"@(#)sccs:lib/comobj/sethome.c"
#endif
#include	<defines.h>

LOCAL	int	shinit;	/* sethome was initialized */
/*
 * If it is not possible to retrieve "setahome", it may be a NULL pointer.
 * The variables "setrhome" and "cwdprefix" are always set from sethome().
 *
 * Note that if we switch to shared libraries, these variables will not work
 * on Mac OS since Mac OS only supports a broken linker.
 */
char	*setphome;	/* Best path to the project set home directory */
char	*setrhome;	/* Relative path to the project set home directory */
char	*setahome;	/* Absolute path to the project set home directory */
char	*cwdprefix;	/* Prefix from project set home directory to cwd */
char	*changesetfile;	/* The path to the changeset history file */
int	homedist;	/* The # of directories to the project set home dir */
int	setphomelen;	/* strlen(setphome) */
int	setrhomelen;	/* strlen(setrhome) */
int	setahomelen;	/* strlen(setahome) */
int	cwdprefixlen;	/* strlen(cwdprefix) */
int	sethomestat;	/* sethome() status flags */

EXPORT	void	unsethome	__PR((void));
EXPORT	int	xsethome	__PR((char *path));
EXPORT	int	sethome		__PR((char *path));
LOCAL	int	searchabs	__PR((char *path));
LOCAL	int	dorel		__PR((char *bp, size_t len));
LOCAL	void	mkrelhome	__PR((char *bp, size_t len, int i));
LOCAL	int	mkprefix	__PR((char *bp, size_t len, int i));
LOCAL	int	checkdotsccs	__PR((char *path));
EXPORT	int	checkhome	__PR((char *path));
LOCAL	void	gchangeset	__PR((void));

/*
 * Undo the effect of a previous sethome() call.
 * Free all data and change the state tu "uninitialized".
 */
EXPORT void
unsethome()
{
	if (setrhome != NULL) {
		free(setrhome);
		setrhome = NULL;
	}
	if (setahome != NULL) {
		free(setahome);
		setahome = NULL;
	}
	if (cwdprefix != NULL) {
		free(cwdprefix);
		cwdprefix = NULL;
	}
	if (changesetfile != NULL) {
		free(changesetfile);
		changesetfile = NULL;
	}
	setphome = NULL;
	homedist = setphomelen = setrhomelen = setahomelen =
			cwdprefixlen = sethomestat = 0;
	shinit = 0;
}

/*
 * A variant of sethome() that includes error handling.
 */
EXPORT int
xsethome(path)
	char	*path;
{
	int	ret;

	errno = 0;
	ret = sethome(path);
	if (ret < 0)
		efatal(gettext("cannot get project home directory (co32)"));
	errno = 0;
	return (ret);
}

/*
 * Try to find the project set home directory and a path from that directory
 * to the current directory.
 *
 * This is the central function for sccs when it tries to support whole
 * projects instead of just unrelated single files.
 *
 * Returns:
 *
 *	-1	Error, cannot scan directories for ".sccs"
 *	 0	$SET_HOME/.sccs not found
 *	 1	$SET_HOME/.sccs found
 */
EXPORT int
sethome(path)
	char	*path;
{
	char	buf[max(8192, PATH_MAX+1)];
	int	len;

	if (shinit != 0)
		return (sethomestat & SETHOME_OK);
	shinit = 1;

	if (path == NULL)
		path = ".";
	if (abspath(path, buf, sizeof (buf)) == NULL) {	/* Can't get abspath */
		/*
		 * Resolve symlinks and .. in path
		 */
		resolvenpath(path, buf, sizeof (buf));	/* Rationalize path */
		return (dorel(buf, sizeof (buf)));
	}

	errno = 0;
	len = searchabs(buf);				/* Index after home */
	if (len < 0) {
		if (errno != 0)				/* Other than ENOENT */
			return (-1);
		return (0);				/* No $SET_HOME/.sccs */
	}

	/*
	 * $SET_HOME/.sccs was found and len is the offset
	 * in buf where we may append "/.sccs".
	 */
	buf[len] = '\0';
	if (len == 0) {
		setahome = strdup("/");
		setahomelen = 1;
	} else {
		setahome = strdup(buf);
		setahomelen = len;
	}
	cwdprefix = strdup(&buf[len+1]);
	cwdprefixlen = strlen(&buf[len+1]);
	if (setahome == NULL || cwdprefix == NULL) {	/* no mem */
		setahomelen = 0;
		cwdprefixlen = 0;
		return (-1);				/* Return error */
	}
	resolvenpath(path, buf, sizeof (buf));		/* Rationalize path */
	mkrelhome(buf, sizeof (buf), homedist);
	if (setrhome == NULL)				/* no mem */
		return (-1);				/* return error */
	if (setahomelen > 0 && setahomelen < setrhomelen) {
		sethomestat |= SETHOME_ABS;
		setphome = setahome;
		setphomelen = setahomelen;
	} else {
		setphome = setrhome;
		setphomelen = setrhomelen;
	}
	sethomestat |= SETHOME_OK;
	(void) checkdotsccs(setrhome);
	gchangeset();
	return (1);					/* $SET_HOME/.sccs OK */
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
			if (S_ISDIR(sb.st_mode))
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
 * the current directory. This function is only called in case that it did not
 * work to retrieve the absolute pathname.
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

	p = bp + strlen(bp);
	if (bp[0] == '.' && bp[1] == '\0') {
		p = bp;
		s = &bp[1];
		strlcpy(bp, ".sccs", len);
	} else {
		if (stat(bp, &sb) < 0) {
			/*
			 * If we cannot stat() the given initial path for
			 * whatever reason, we will not be able to scan the
			 * filesystem tree to search for ".sccs".
			 */
			return (0);
		}
		if (strlcat(bp, "/.sccs", len) >= len) {
			return (-1);
		} else {
			s = p + 2;
		}
	}
	i = 0;
	for (;;) {
		found = 0;
		if (stat(bp, &sb) >= 0) {
			if (S_ISDIR(sb.st_mode))
				found++;
			break;
		} else if (errno != ENOENT) {
			err = errno;
			break;
		}
		*s = '\0';
		sb.st_ino = (ino_t)-1;
		sb2.st_ino = (ino_t)-2;
		stat(bp, &sb);		/* "." */
		s[0] = '.';		/* Make it ".." */
		s[1] = '\0';
		stat(bp, &sb2);		/* ".." */
		if (sb.st_ino == sb2.st_ino &&
		    sb.st_dev == sb2.st_dev) {
			break;		/* We found the root directory. */
		}
		if (strlcat(bp, "/.sccs", len) >= len)
			return (-1);
		s += 3;
		i++;
	}
	if (!found) {
		if (err) {
			errno = err;
			return (-1);			/* Other than ENOENT */
		}
		return (0);				/* No $SET_HOME/.sccs */
	}
	s -= 2;
	if (s > bp)
		s[0] = '\0';
	if (p == bp) {
		setrhome = strdup(".");
	} else
		setrhome = strdup(bp);
	if (setrhome != NULL)
		setrhomelen = strlen(bp);
	homedist = i;
	if (setrhome == NULL)				/* no mem */
		return (-1);
	setphome = setrhome;
	setphomelen = setrhomelen;
	sethomestat |= SETHOME_OK;
	(void) checkdotsccs(setrhome);
	*p = '\0';					/* Cut off new text */
	errno = 0;
	if (!mkprefix(bp, len, i))			/* Not found */
		return (-1);
	if (cwdprefix == NULL)				/* no mem */
		return (-1);
	gchangeset();
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
	int	isdot = bp[0] == '.' && bp[1] == '\0';

	if (isdot)
		bp[0] = '\0';
	if (n == 0 && isdot) {
		strlcat(bp, ".", len);
	} else {
		if (isdot) {
			bp[1] = '.';
			bp[2] = '\0';
		}
		if (n > 0 && bp[0] == '\0') {
			strlcat(bp, "..", len);
			n--;
		}
		while (--n >= 0) {
			strlcat(bp, "/..", len);
		}
	}
	setrhome = strdup(bp);
	if (setrhome != NULL)
		setrhomelen = strlen(bp);
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
	char	*s;
	int	i;
	int	found;
	DIR	*dp;
	struct dirent *de;

	if (bp[0]) {
		if (stat(bp, &sb) < 0) {
			/*
			 * If we cannot stat() the given initial path for
			 * whatever reason, we will not be able to scan the
			 * filesystem tree to search for ".sccs".
			 */
			return (0);
		}
		strlcat(bp, "/", len);
	}
	i = n;
	for (; --i >= 0; ) {
		strlcat(bp, "../", len);
	}
	strlcat(bp, ".", len);

	s = bp + strlen(bp);
	i = n;
	pfx[0] = '\0';
	while (--i >= 0) {		/* Enter only if n > 0 */

		s[-3] = '\0';
		stat(bp, &sb);		/* Dir to match */
		s[-3] = '.';
		dp = opendir(bp);	/* one level above that dir */
		
		strlcpy(buf, bp, sizeof (buf));
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
		s -= 3;
		*s = '\0';
	}
	cwdprefix = strdup(pfx);
	if (cwdprefix != NULL)
		cwdprefixlen = strlen(pfx);
	return (1);				/* cwdprefix found */
}

/*
 * Check $SET_HOME/.sccs for specific sub-directories and flag the
 * search results.
 */
LOCAL int
checkdotsccs(path)
	char	*path;
{
	struct stat sb;
	char	buf[max(8192, PATH_MAX+1)];
	char	*p;
	int	len;
	int	err = 0;

	strlcpy(buf, path, sizeof (buf));
	len = strlen(buf);
	p = &buf[len];

	strlcpy(p, "/.sccs/data", sizeof (buf) - (p - buf));
	if (stat(buf, &sb) >= 0) {
		if (S_ISDIR(sb.st_mode)) {
			sethomestat &= ~SETHOME_INTREE;
			sethomestat |=  SETHOME_OFFTREE;
		}
	} else if (errno != ENOENT) {
		err = errno;
	} else {
		sethomestat |=  SETHOME_INTREE;
		sethomestat &= ~SETHOME_OFFTREE;
	}
	p += 7;					/* Skip "/.sccs/" */
	strlcpy(p, "dels", sizeof (buf) - (p - buf));
	if (stat(buf, &sb) >= 0) {
		if (S_ISDIR(sb.st_mode))
			sethomestat |= SETHOME_DELS_OK;
	} else if (errno != ENOENT) {
		err = errno;
	} else {
		sethomestat &= ~SETHOME_DELS_OK;
	}
	if (err) {
		errno = err;
		return (-1);			/* Other than ENOENT */
	}
	return (0);
}

/*
 * Check whether the change set home directory could be detected and
 * abort in case it is missing.
 */
EXPORT int
checkhome(path)
	char	*path;
{
	if (shinit != 0)
		xsethome(path);
	if (!SETHOME_INIT()) {
		efatal(gettext("no SCCS project home directory (co33)"));
		return (0);			/* (Fflags & FTLACT= == 0) */
	}
	return (1);
}

LOCAL void
gchangeset()
{
	char		buf[max(8192, PATH_MAX+1)];
	struct stat	_Statbuf;

	if (setphome == NULL)
		return;
	strlcpy(buf, setphome, sizeof (buf));
	strlcat(buf, "/.sccs/SCCS/s.changeset", sizeof (buf));
	if (_exists(buf) && S_ISREG(_Statbuf.st_mode))
		sethomestat |= SETHOME_CHSET_OK;
	changesetfile = strdup(buf);
}
