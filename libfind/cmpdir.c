/* @(#)cmpdir.c	1.28 19/11/28 Copyright 2002-2019 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)cmpdir.c	1.28 19/11/28 Copyright 2002-2019 J. Schilling";
#endif
/*
 *	Blocked directory sort/compare.
 *
 *	Copyright (c) 2002-2019 J. Schilling
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
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/types.h>
#include <schily/utypes.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/fetchdir.h>

EXPORT	int	fdircomp	__PR((const void *p1, const void *p2));
EXPORT	char	**sortdir	__PR((char *dir, size_t *entp));
EXPORT	int	cmpdir		__PR((size_t ents1, size_t ents2,
					char **ep1, char **ep2,
					char **oa, char **od,
					size_t *alenp, size_t *dlenp));

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
 * If "ents" == (size_t)-1, start with counting exisiting entries.
 *
 * Return allocated arry with string pointers.
 */
EXPORT char **
sortdir(dir, entp)
	char	*dir;
	size_t	*entp;
{
	size_t	ents = (size_t)-1;
	char	**ea;
	char	*d;
	char	*p;
	size_t	i;

	if (entp)
		ents = *entp;
	if (ents == (size_t)-1) {
		d = dir;
		ents = 0;
		while (*d) {
			ents++;
			p = strchr(d, '\0');
			d = &p[1];
		}
	}
	ea = ___malloc((ents+1)*sizeof (char *), "sortdir");
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
	register size_t	ents1;	/* # of directory entries in arch	*/
	register size_t	ents2;	/* # of directory entries on disk	*/
	register char	**ep1;	/* Directory entry pointer array (arch)	*/
	register char	**ep2;	/* Directory entry pointer array (disk)	*/
	register char	**oa;	/* Only in arch pointer array		*/
	register char	**od;	/* Only on disk pointer array		*/
		size_t	*alenp;	/* Len pointer for "only in arch" array	*/
		size_t	*dlenp;	/* Len pointer for "only on disk" array	*/
{
	register size_t	i1;	/* Index for 'only in archive'		*/
	register size_t	i2;	/* Index for 'only on disk'		*/
	register int	d;	/* 'diff' amount (== 0 means equal)	*/
	register size_t	alen = 0; /* Number of ents only in archive	*/
	register size_t	dlen = 0; /* Number of ents only on disk	*/

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
