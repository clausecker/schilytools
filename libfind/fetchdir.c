/* @(#)fetchdir.c	1.21 07/04/06 Copyright 2002-2007 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)fetchdir.c	1.21 07/04/06 Copyright 2002-2007 J. Schilling";
#endif
/*
 *	Blocked directory handling.
 *
 *	Copyright (c) 2002-2007 J. Schilling
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

#include <schily/mconfig.h>
#include <stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/dirent.h>
#include <schily/stat.h>	/* needed in case we have no dirent->d_ino */
#include <schily/string.h>
#include <schily/libport.h>
#include <schily/schily.h>
#ifdef	__STAR__
#include "star.h"
#include "starsubs.h"
#endif

#ifndef	HAVE_LSTAT
#define	lstat	stat
#endif

EXPORT	char	*fetchdir	__PR((char *dir, int *entp, int *lenp, ino_t **inop));
EXPORT	char	*dfetchdir	__PR((DIR *dir, char *dirname, int *entp, int *lenp, ino_t **inop));
EXPORT	int	fdircomp	__PR((const void *p1, const void *p2));
EXPORT	char	**sortdir	__PR((char *dir, int *entp));
EXPORT	int	cmpdir		__PR((int ents1, int ents2,
					char **ep1, char **ep2,
					char **oa, char **od,
					int *alenp, int *dlenp));

EXPORT char *
fetchdir(dir, entp, lenp, inop)
	char	*dir;			/* The name of the directory	  */
	int	*entp;			/* Pointer to # of entries found  */
	int	*lenp;			/* Pointer to len of returned str */
	ino_t	**inop;
{
	char	*ret;
	DIR	*d = opendir(dir);

	if (d == NULL)
		return (NULL);
	ret = dfetchdir(d, dir, entp, lenp, inop);
	closedir(d);
	return (ret);
}

/*
 * Fetch content of a directory and return all entries (except '.' & '..')
 * concatenated in one memory chunk.
 *
 * Each name is prepended by a binary 1 ('^A') that is used by star to flag
 * additional information for this entry.
 * The end of the returned string contains two additional null character.
 */
EXPORT char *
dfetchdir(d, dirname, entp, lenp, inop)
	DIR	*d;
	char	*dirname;		/* The name of the directory	  */
	int	*entp;			/* Pointer to # of entries found  */
	int	*lenp;			/* Pointer to len of returned str */
	ino_t	**inop;
{
		char	*erg = NULL;
		int	esize = 2;
		int	msize = getpagesize();
		int	off = 0;
		ino_t	*ino = NULL;
		int	mino = 0;
	struct dirent	*dp;
	register char	*name;
	register int	nlen;
	register int	nents = 0;
#ifndef	HAVE_DIRENT_D_INO
	struct stat	sbuf;
		char	sname[PATH_MAX+1];
#endif

	if ((erg = __malloc(esize, "fetchdir")) == NULL)
		return (NULL);
	erg[0] = '\0';
	erg[1] = '\0';

	while ((dp = readdir(d)) != NULL) {
		name = dp->d_name;
		/*
		 * Skip the following names: "", ".", "..".
		 */
		if (name[name[0] != '.' ? 0 : name[1] != '.' ? 1 : 2] == '\0')
			continue;
		if (inop) {
			if (mino <= nents) {
				if (mino == 0)
					mino = 32;
				else if (mino < (msize / sizeof (ino_t)))
					mino *= 2;
				else
					mino += msize / sizeof (ino_t);
				if ((ino = __realloc(ino, mino * sizeof (ino_t), "fetchdir")) == NULL)
					return (NULL);
			}
#ifdef	HAVE_DIRENT_D_INO
			ino[nents] = dp->d_ino;
#else
			/*
			 * d_ino is currently missing on __DJGPP__ & __CYGWIN__
			 * We need to call lstat(2) for every file
			 * in order to get the needed information.
			 * Do not care about speed, this should be a rare
			 * exception.
			 */
			if (dirname != NULL) {
				snprintf(sname, sizeof (sname), "%s/%s",
								dirname, name);
				sbuf.st_ino = (ino_t)0;
				lstat(sname, &sbuf);
				ino[nents] = sbuf.st_ino;
			} else {
				ino[nents] = (ino_t)-1;
			}
#endif
		}
		nents++;
		nlen = strlen(name);
		nlen += 4;		/* ^A name ^@ + ^@^@ Platz fuer Ende */
		while (esize < (off + nlen)) {
			if (esize < 64)
				esize = 32;
			if (esize < msize)
				esize *= 2;
			else
				esize += msize;
			if (esize < (off + nlen))
				continue;

			if ((erg = __realloc(erg, esize, "fetchdir")) == NULL)
				return (NULL);
		}
#ifdef	DEBUG
		if (off > 0)
			erg[off-1] = 2;	/* Hack: ^B statt ^@ zwischen Namen */
#endif
		erg[off++] = 1;		/* Platzhalter: ^A vor jeden Namen  */

		strcpy(&erg[off], name);
		off += nlen -3;		/* ^A  + ^@^@ Platz fuer Ende	    */
	}
#ifdef	DEBUG
	erg[off-1] = 2;			/* Hack: ^B st. ^@ am letzten Namen */
#endif
	erg[off] = 0;
	erg[off+1] = 0;

#ifdef	DEBUG
	erg[off] = 1;			/* Platzhalter: ^A n. letztem Namen */
	erg[off+1] = 0;			/* Letztes Null Byte		    */
#endif
	off++;				/* List terminator null Byte zaehlt */
	if (lenp)
		*lenp = &erg[off] - erg; /* Alloziert ist 1 Byte mehr	    */

	if (entp)
		*entp = nents;
	if (inop)
		*inop = ino;

	return (erg);
}

/*
 * XXX These functions should better go into cmpdir.c
 */
#ifdef	__STAR__
/*
 * Compare directory entries from fetchdir().
 * Ignore first byte, it does not belong to the directory data.
 */
EXPORT int
fdircomp(p1, p2)
	const void	*p1;
	const void	*p2;
{
	register Uchar	*s1;
	register Uchar	*s2;

	s1 = *(Uchar **)p1;
	s2 = *(Uchar **)p2;
	s1++;
	s2++;
	while (*s1 == *s2) {
		if (*s1 == '\0')
			return (0);
		s1++;
		s2++;
	}
	return (*s1 - *s2);
}

/*
 * Sort a directory string as returned by fetchdir().
 *
 * Return allocated arry with string pointers.
 */
EXPORT char **
sortdir(dir, entp)
	char	*dir;
	int	*entp;
{
	int	ents = -1;
	char	**ea;
	char	*d;
	char	*p;
	int	i;

	if (entp)
		ents = *entp;
	if (ents < 0) {
		d = dir;
		ents = 0;
		while (*d) {
			ents++;
			p = strchr(d, '\0');
			d = &p[1];
		}
	}
	ea = __malloc((ents+1)*sizeof (char *), "sortdir");
	for (i = 0; i < ents; i++) {
		ea[i] = NULL;
	}
	for (i = 0; i < ents; i++) {
		ea[i] = dir;
		p = strchr(dir, '\0');
		if (p == NULL)
			break;
		dir = ++p;
	}
	ea[ents] = NULL;
	qsort(ea, ents, sizeof (char *), fdircomp);
	if (entp)
		*entp = ents;
	return (ea);
}

EXPORT int
cmpdir(ents1, ents2, ep1, ep2, oa, od, alenp, dlenp)
	register int	ents1;	/* # of directory entries in arch	*/
	register int	ents2;	/* # of directory entries on disk	*/
	register char	**ep1;	/* Directory entry pointer array (arch)	*/
	register char	**ep2;	/* Directory entry pointer array (disk)	*/
	register char	**oa;	/* Only in arch pointer array		*/
	register char	**od;	/* Only on disk pointer array		*/
		int	*alenp;	/* Len pointer for "only in arch" array	*/
		int	*dlenp;	/* Len pointer for "only on disk" array	*/
{
	register int	i1;	/* Index for 'only in archive'		*/
	register int	i2;	/* Index for 'only on disk'		*/
	register int	d;	/* 'diff' amount (== 0 means equal)	*/
	register int	alen = 0; /* Number of ents only in archive	*/
	register int	dlen = 0; /* Number of ents only on disk	*/

	for (i1 = i2 = 0; i1 < ents1 && i2 < ents2; i1++, i2++) {
		d = fdircomp(&ep1[i1], &ep2[i2]);
		retry:

		if (d > 0) {
			do {
				d = fdircomp(&ep1[i1], &ep2[i2]);
				if (d <= 0)
					goto retry;
				if (od)
					od[dlen] = ep2[i2];
				dlen++;
				i2++;
			} while (i1 < ents1 && i2 < ents2);
		}
		if (d < 0) {
			do {
				d = fdircomp(&ep1[i1], &ep2[i2]);
				if (d >= 0)
					goto retry;
				if (oa)
					oa[alen] = ep1[i1];
				alen++;
				i1++;
			} while (i1 < ents1 && i2 < ents2);
		}
		/*
		 * Do not increment i1 & i2 in case that the last elements are
		 * not equal and need to be treaten as longer list elements in
		 * the code below.
		 */
		if (i1 >= ents1 || i2 >= ents2)
			break;
	}
	/*
	 * One of both lists is longer after some amount.
	 */
	if (od == NULL) {
		if (i2 < ents2)
			dlen += ents2 - i2;
	} else {
		for (; i2 < ents2; i2++) {
			od[dlen] = ep2[i2];
			dlen++;
		}
	}
	if (oa == NULL) {
		if (i1 < ents1)
			alen += ents1 - i1;
	} else {
		for (; i1 < ents1; i1++) {
			oa[alen] = ep1[i1];
			alen++;
		}
	}
	if (alenp)
		*alenp = alen;
	if (dlenp)
		*dlenp = dlen;

	return (alen + dlen);
}
#endif	/* __STAR__ */
