/* @(#)match.c	1.12 08/12/22 Copyright 1985, 88-90, 92-96, 98, 99, 2000-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)match.c	1.12 08/12/22 Copyright 1985, 88-90, 92-96, 98, 99, 2000-2008 J. Schilling";
#endif
/*
 *	Pattern matching routines for star
 *
 *	Copyright (c) 1985, 88-90, 92-96, 98, 99, 2000-2008 J. Schilling
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
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include <schily/patmatch.h>
#include "starsubs.h"

EXPORT	const char *filename	__PR((const char *name));
LOCAL	BOOL	nameprefix	__PR((int i, const char *name));
LOCAL	BOOL	patprefix	__PR((int i, const char *name, int namelen));
LOCAL	int	namefound	__PR((const char *name));
EXPORT	BOOL	match		__PR((const char *name));
EXPORT	int	addpattern	__PR((const char *pattern));
EXPORT	int	addarg		__PR((const char *pattern));
EXPORT	void	closepattern	__PR((void));
EXPORT	void	prpatstats	__PR((void));
LOCAL	void	printpattern	__PR((void));
LOCAL	BOOL	issimple	__PR((const char *pattern));

extern	BOOL	debug;			/* -debug has been specified	*/
extern	BOOL	cflag;			/* -c has been specified	*/
extern	BOOL	xflag;			/* -x has been specified	*/
extern	BOOL	diff_flag;		/* -diff has been specified	*/
extern	BOOL	nodesc;			/* -D do not descenc dirs	*/
extern	BOOL	notpat;			/* -not invert pattern matcher	*/
extern	BOOL	notarg;			/* PAX -c invert match		*/
extern	BOOL	paxmatch;		/* Do PAX like matching		*/
extern	BOOL	paxnflag;		/* PAX -n one match only	*/

extern	const	char	*wdir;		/* current working dir name	*/
extern	const	char	*dir_flags;	/* -C dir from getallargs()	*/
extern	const	char	*currdir;	/* current -C dir argument	*/

#define	NPAT	100			/* Max. number of patterns	*/

EXPORT	BOOL		havepat = FALSE; /* Pattern matching in use	*/
LOCAL	int		npat	= 0;	/* Number of pat= patterns	*/
LOCAL	int		narg	= 0;	/* Number of file arg patterns	*/
LOCAL	int		maxplen	= 0;	/* Max pat len for state array	*/
LOCAL	int		*aux[NPAT+1];	/* Aux array for pat matcher	*/
LOCAL	int		alt[NPAT+1];	/* Alt values for pat matcher	*/
LOCAL	int		*state;		/* State array for matching	*/
LOCAL	const	Uchar	*pat[NPAT+1];	/* The plain text patterns	*/
LOCAL	const	char	*dirs[NPAT+1];	/* The coresponding directories	*/
LOCAL	char		didm[NPAT+1];	/* If a match did happen	*/

EXPORT const char *
filename(name)
	const char	*name;
{
	char	*p;

	if ((p = strrchr(name, '/')) == NULL)
		return (name);
	return (++p);
}

LOCAL BOOL
nameprefix(i, name)
			int	i;
	register const char	*name;
{
	register const char	*patp;

	patp = (const char *)pat[i];
	while (*patp) {
		if (*patp++ != *name++)
			return (FALSE);
	}
	if (*name) {
		if (nodesc)
			return (FALSE);
		if (*name != '/')	/* Directory tree match	*/
			return (FALSE);
	}
	if (paxnflag && didm[i])	/* Only one exact match */
		return (FALSE);
	didm[i] = TRUE;
	return (TRUE);			/* Names are equal	*/
}

/*
 * XXX POSIX PAX requires that '/' characters need to be specified explicitly
 * XXX as in shell pattern matching. We do not yet support this.
 */
LOCAL BOOL
patprefix(i, name, namelen)
	register int	i;
	register const char	*name;
	register int	namelen;
{
	char	*ret;

	ret = (char *)patmatch(pat[i], aux[i],
			(const Uchar *)name, 0,
			namelen, alt[i], state);
	if (ret != NULL) {
		if (*ret == '\0' || *ret == '/') {
			if (*ret == '\0') {
				if (paxnflag && didm[i])
					return (FALSE);
				didm[i] = TRUE;
			} else {
				if (nodesc)
					return (FALSE);
			}
			return (TRUE);
		}
	}
	return (FALSE);
}

/*
 * Match 'dirname' against 'dirname' and 'dirname/any'
 * Match 'dirname/' against 'dirname' only.
 *
 * Return index in pattern list for a match. The index is used
 * for indexing in the matching directory array later.
 */
LOCAL int
namefound(name)
	const	char	*name;
{
	register int	i;
	register int	namelen = -1;

	for (i = npat; i < narg; i++) {
		if (aux[i] != NULL) {
			if (namelen < 0)
				namelen = strlen(name);
			if (patprefix(i, name, namelen)) {
				if (notarg)
					return (-1);
				return (i);
			}
		} else if (nameprefix(i, name)) {
			if (notarg)
				return (-1);
			return (i);
		}
	}
	if (notarg)
		return (narg);
	return (-1);
}

EXPORT BOOL
match(name)
	const	char	*name;
{
	register int	i;
	register int	namelen;
		char	*ret = NULL;

	if (!cflag && narg > 0) {
		if ((i = namefound(name)) < 0)
			return (FALSE);
		if (npat == 0)
			goto found;
	}

	namelen = strlen(name);

	for (i = 0; i < npat; i++) {
		if (paxnflag && didm[i])
			continue;
		ret = (char *)patmatch(pat[i], aux[i],
					(const Uchar *)name, 0,
					namelen, alt[i], state);
		if (ret != NULL && *ret == '\0') {
			didm[i] = TRUE;
			break;
		}
	}
	if (notpat ^ (ret != NULL && *ret == '\0')) {
found:
		if (!(xflag || diff_flag))	/* Chdir only on -x or -diff */
			return (TRUE);
		if (dirs[i] != NULL && currdir != dirs[i]) {
			currdir = dirs[i];
			dochdir(wdir, TRUE);
			dochdir(currdir, TRUE);
		}
		return (TRUE);
	}
	return (FALSE);
}

EXPORT int
addpattern(pattern)
	const char	*pattern;
{
	int	plen;

/*	if (debug)*/
/*		error("Add pattern: '%s' currdir: '%s'.\n", pattern, currdir==NULL?dir_flags:currdir);*/

	if (npat >= NPAT)
		comerrno(EX_BAD, "Too many patterns (max is %d).\n", NPAT);
	plen = strlen(pattern);
	pat[npat] = (const Uchar *)pattern;

	if (plen > maxplen)
		maxplen = plen;

	aux[npat] = ___malloc(plen*sizeof (int), "compiled pattern");
	if ((alt[npat] = patcompile((const Uchar *)pattern,
							plen, aux[npat])) == 0)
		comerrno(EX_BAD, "Bad pattern: '%s'.\n", pattern);

	if (currdir == NULL)
		dirs[npat] = dir_flags;
	else
		dirs[npat] = currdir;
	npat++;
	return (TRUE);
}

EXPORT int
addarg(pattern)
	const char	*pattern;
{
	int	plen;

	/*
	 * Patterns from 'file type args' start after pat= patterns in the
	 * pat[] array.
	 */
	if (narg == 0)
		narg = npat;

/*	if (debug)*/
/*		error("Add arg '%s'.\n", pattern);*/

	if (narg >= NPAT)
		comerrno(EX_BAD, "Too many patterns (max is %d).\n", NPAT);

	plen = strlen(pattern);
	pat[narg] = (const Uchar *)pattern;

	if (!paxmatch || issimple(pattern)) {
		aux[narg] = NULL;
		alt[narg] = 0;
	} else {
		if (plen > maxplen)
			maxplen = plen;
		aux[npat] = ___malloc(plen*sizeof (int), "compiled pattern");
		if ((alt[npat] = patcompile((const Uchar *)pattern,
						plen, aux[npat])) == 0) {
			comerrno(EX_BAD, "Bad pattern: '%s'.\n", pattern);
		}
	}
	dirs[narg] = currdir;
	narg++;
	return (TRUE);
}

/*
 * Close pattern list: insert useful default directories.
 */
EXPORT void
closepattern()
{
	register int	i;

	if (debug) {	/* temporary */
		error("closepattern(), maxplen %d\n", maxplen);
		printpattern();
	}

	for (i = 0; i < npat; i++) {
		if (dirs[i] != NULL)
			break;
	}
	while (--i >= 0)
		dirs[i] = wdir;

	if (debug) /* temporary */
		printpattern();

	if (npat > 0 || narg > 0)
		havepat = TRUE;

	if (maxplen > 0) {
		state = ___malloc((maxplen+1)*sizeof (int), "pattern state");
	}
}

EXPORT void
prpatstats()
{
	register int	i;

	for (i = 0; i < narg; i++) {
		if (didm[i])
			continue;
		errmsgno(EX_BAD, "'%s' did not match\n", pat[i]);
	}
}

LOCAL void
printpattern()
{
	register int	i;

	error("npat: %d narg: %d\n", npat, narg);
	for (i = 0; i < npat; i++) {
		error("pat %s dir %s\n", pat[i], dirs[i]);
	}
	for (i = npat; i < narg; i++) {
		error("arg %s dir %s\n", pat[i], dirs[i]);
	}
}

/*
 * Check if the pattern contains special characters of the pattern matcher.
 * Do not look for characters that may be used only inside of a character set.
 */
LOCAL BOOL
issimple(pattern)
	register const char	*pattern;
{
	while (*pattern != '\0') {
		switch (*pattern++) {

		casePAT
			return (FALSE);
		}
	}
	return (TRUE);
}
