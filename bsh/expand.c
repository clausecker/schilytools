/* @(#)expand.c	1.43 11/08/04 Copyright 1985-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)expand.c	1.43 11/08/04 Copyright 1985-2009 J. Schilling";
#endif
/*
 *	Expand a pattern (do shell name globbing)
 *
 *	Copyright (c) 1985-2009 J. Schilling
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

#include <schily/stdio.h>
#include "bsh.h"
#include "node.h"
#include "str.h"
#include "strsubs.h"
#include <schily/string.h>
#include <schily/stdlib.h>
#include <schily/dirent.h>
#include <schily/patmatch.h>
/*#define	DEBUG*/

#ifdef	DEBUG
#define	EDEBUG(a)	printf a
#else
#define	EDEBUG(a)
#endif

static char mchars[] = "!#%*{}[]?\\";

#define	exp	_exp	/* Some compilers do not like exp() */

LOCAL	int	dncmp		__PR((char *s1, char *s2));
LOCAL	char	*save_base	__PR((char *s, char *endptr));
EXPORT	BOOL	any_match	__PR((char *s));
LOCAL	Tnode	*mklist		__PR((Tnode *l));
LOCAL	int	xcmp		__PR((char *s1, char *s2));
LOCAL	void	xsort		__PR((char **from, char **to));
LOCAL	Tnode	*exp		__PR((char *n, int i, Tnode * l));
EXPORT	Tnode	*expand		__PR((char *s));

LOCAL int
dncmp(s1, s2)
	register char	*s1;
	register char	*s2;
{
	for (; *s1 == *s2; s1++, s2++) {
		if (*s1 == '\0')
			return (0);
	}
	if (*s1 == '\0' && *s2 == '/')
		return (0);
	return (*s1 - *s2);
}

LOCAL char *
save_base(s, endptr)
	register char	*s;
	register char	*endptr;
{
	register char	*tmp;
	register char	*r;

	tmp = malloc((size_t)(endptr-s+1));
	for (r = tmp; s < endptr; )
		*r++ = *s++;
	*r = '\0';
	return (tmp);
}

EXPORT BOOL
any_match(s)
	register char	*s;
{
	register char	*rm = mchars;

	while (*s && !strchr(rm, *s))
		s++;
	return ((BOOL) *s);
}

LOCAL Tnode *
mklist(l)
		Tnode	*l;
{
	register int	ac;
	register char	**p;
	register Tnode	*l1;
	register Argvec	*vp;

	if (l == (Tnode *)NULL)
		return ((Tnode *)NULL);

	ac = listlen(l);
	vp = allocvec(ac);
	vp->av_ac = ac;
	for (l1 = l, p = &vp->av_av[0]; --ac >= 0; ) {
		*p++ = l1->tn_left.tn_str;
		l1->tn_type = STRING;	/* Type LSTRING -> STRING */
		l1 = l1->tn_right.tn_node;
	}
	xsort(&vp->av_av[0], &vp->av_av[vp->av_ac]);

	ac = vp->av_ac;

	for (l1 = l, p = &vp->av_av[0]; --ac >= 0; ) {
		l1->tn_left.tn_str = *p++;
		l1 = l1->tn_right.tn_node;
	}
	free((char *)vp);
	return (l);
}

LOCAL int
xcmp(s1, s2)
	register char	*s1;
	register char	*s2;
{
	while (*s1++ == *s2)
		if (*s2++ == 0)
			return (0);
	return (*--s1 - *s2);
}

#define	USE_QSORT
#ifdef	USE_QSORT
/*
 *	quicksort algorithm
 *	on array of elsize elements from lowp to hip-1
 */
#define	exchange(a, b)	{ register char *p = *(a); *(a) = *(b); *(b) = p; }

LOCAL void
xsort(lowp, hip)
	register char	*lowp[];
	register char	*hip[];
{
	register char **olp;
	register char **ohp;
	register char **pivp;

	hip--;

	while (hip > lowp) {
		if (hip == (lowp + 1)) {	/* two elements */
			if (xcmp(*hip, *lowp) < 0)
				exchange(lowp, hip);
			return;
		}
		olp = lowp;
		ohp = hip;

		pivp = lowp+((((unsigned)(hip-lowp))+1)/2);
		exchange(pivp, hip);
		pivp = hip;
		hip--;				/* point past pivot element */

		while (lowp <= hip) {
			while ((hip >= lowp) && (xcmp(*hip, *pivp) >= 0))
				hip--;
			while ((lowp <= hip) && (xcmp(*lowp, *pivp) < 0))
				lowp++;
			if (lowp < hip) {
				exchange(lowp, hip);
				hip--;
				lowp++;
			}
		}
		/*
		 *	now lowp points at first member not in low set
		 *	and hip points at first member not in high set.
		 *	Since high set contains the pivot element (by
		 *	definition) it has at least one element.
		 *	low set might not.  check for this and make the
		 *	pivot element part of the low set if its is empty.
		 *	sort smaller of remaining pieces
		 *	then larger, to cut down on recursion
		 */
		if (lowp == olp) {			/* its empty */
			exchange(lowp, pivp);
			lowp++;		 /* point past sigle element(pivot) */
			hip = ohp;
		} else if ((olp - lowp-1) > (hip+1 - ohp)) {
			xsort(hip+1, ohp+1);
			/* hip = lowp - elsize; set right alread */
			lowp = olp;
		} else {
			xsort(olp, lowp);
			/* lowp = hip+elsize;  set right already */
			hip = ohp;
		}

	}
}
#else
/*
 *	shellsort algorithm
 *	on array of elsize elements from lowp to hip-1
 */
LOCAL void
xsort(from, to)
	char	*from[];
	char	*to[];
{
	register int	i;
	register int	j;
		int	k;
		int	m;
		int	n;

	if ((n = to - from) <= 1)	/* Nothing to sort. */
		return;

	for (j = 1; j <= n; j *= 2)
		;

	for (m = 2 * j - 1; m /= 2; ) {
		k = n - m;
		for (j = 0; j < k; j++) {
			for (i = j; i >= 0; i -= m) {
				register char **fromi;
/*#define	cmplocal*/
#ifdef	cmplocal
				register char	*s1;
				register char	*s2;
#endif

				fromi = &from[i];
#ifdef	cmplocal
				s1 = fromi[m];
				s2 = fromi[0];

				/* schneller als strcmp() ??? */

				while (*s1++ == *s2) {
					if (*s2++ == 0) {
						--s2;
						break;
					}
				}
				if ((*--s1 - *s2) > 0) {
#else
				if (xcmp(fromi[m], fromi[0]) > 0) {
#endif
					break;
				} else {
					char *s;

					s = fromi[m];
					fromi[m] = fromi[0];
					fromi[0] = s;
				}
			}
		}
	}
}
#endif

LOCAL Tnode *
exp(n, i, l)
		char	*n;		/* name to rescan */
		int	i;		/* index in name to start rescan */
		Tnode	*l;		/* list of Tnodes already found */
{
	register char	*cp;
	register char	*dp;		/* pointer past end of current dir */
		char	*dir;
		char	*tmp;
	register char	*cname;		/* concatenated name dir+d_ent */
		DIR	*dirp;
	struct dirent	*dent;
		int	*aux;
		int	*state;
	register int	patlen;
	register int	alt;
		int	rescan	= 0;
		Tnode	*l1	= l;

	cp = dp = &n[i];

	EDEBUG(("name: '%s' i: %d dp: '%s'\n", n, i, dp));

	while (*cp && !strchr(mchars, *cp)) /* go past non glob parts */
		if (*cp++ == '/')
			dp = cp;

	while (*cp && *cp != '/')	/* find end of name component */
		cp++;

	patlen = cp-dp;
	i = dp - n;			/* make &n[i] == dp (pattern start) */

	/*
	 * Prepare to use the pattern matcher.
	 */
	EDEBUG(("patlen: %d pattern: '%.*s'\n", patlen, patlen, dp));

	aux = malloc((size_t) patlen*(sizeof (int)));
	state = malloc((size_t) (patlen+1)*(sizeof (int)));
	if ((alt = patcompile((unsigned char *)dp, patlen, aux)) == 0 && patlen != 0) {
		EDEBUG(("Bad pattern\n"));
		free((char *) aux);
		free((char *) state);
		return (l1);
	}

	dir = save_base(n, dp);		/* get dirname part */
	if ((dirp = opendir(dp == n ? "." : dir)) == (DIR *) NULL)
		goto cannot;

	EDEBUG(("dir: '%s' match: '%.*s'\n", dp == n?".":dir, patlen, dp));
	if (patlen == 0) {
		/*
		 * match auf /pattern/ Daher kein Match wenn keine
		 * Directory! opendir() Test ist daher notwendig.
		 */
		l1 = allocnode(STRING, (Tnode *)makestr(dir), l1);

	} else while ((dent = readdir(dirp)) != 0 && !ctlc) {
		int	namlen;

		/*
		 * Are we interested in files starting with '.'?
		 */
		if (dent->d_name[0] == '.' && *dp != '.')
			continue;
		namlen = DIR_NAMELEN(dent);
		tmp = (char *)patmatch((unsigned char *)dp, aux,
			(unsigned char *)dent->d_name, 0, namlen, alt, state);

#ifdef	DEBUG
		if (tmp != NULL || (dent->d_name[0] == dp[0] && patlen == namlen))
			EDEBUG(("match? '%s' end: '%s'\n", dent->d_name, tmp));
#endif
		/*
		 * *tmp == '\0' is a result of an exact pattern match.
		 *
		 * dncmp(dent->d_name, dp) == 0 happens when
		 * a pattern contains a pattern matcher special
		 * character (e.g. "foo#1"), but the pattern would not
		 * match itself using regular expressions. We use a
		 * literal compare in this case.
		 */

		if ((tmp != NULL && *tmp == '\0') ||
		    (patlen == namlen &&
		    dent->d_name[0] == dp[0] &&
		    dncmp(dent->d_name, dp) == 0)) {
			EDEBUG(("found: '%s'\n", dent->d_name));

			cname = concat(dir, dent->d_name, cp, (char *)NULL);
			if (*cp == '/') {
				EDEBUG(("rescan: '%s'\n", cname));
				rescan++;
			}
			l1 = allocnode(sxnlen(i+namlen), (Tnode *)cname, l1);
		}
	}
	closedir(dirp);
cannot:
	free(dir);
	free((char *) aux);
	free((char *) state);

	if (rescan > 0) {
		for (alt = rescan; --alt >= 0; ) {
			register Tnode	*l2;

			l = exp(l1->tn_left.tn_str, nlen(l1), l);
			free(l1->tn_left.tn_str);
			l2 = l1->tn_right.tn_node;
			free(l1);
			l1 = l2;
		}
		return (l);
	}

	return (l1);
}

EXPORT Tnode *
expand(s)
	char	*s;
{
	if (!any_match(s))
		return ((Tnode *) NULL);
	else
		return (mklist(exp(s, 0, (Tnode *)NULL)));
}
