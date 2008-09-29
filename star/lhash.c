/* @(#)lhash.c	1.17 08/09/26 Copyright 1988, 1993-2008 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)lhash.c	1.17 08/09/26 Copyright 1988, 1993-2008 J. Schilling";
#endif
/*
 *	Copyright (c) 1988, 1993-2008 J. Schilling
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

/*
 * Hash table name lookup.
 *
 *	Implemented 1988 with help from B. Mueller-Zimmermann
 *
 *	hash_size(nqueue) size_t nqueue;
 *		`nqueue' ist ein tuning parameter und gibt die Zahl der
 *		Hash-queues an. Pro Hashqueue werden 4 Bytes benoetigt.
 *
 *	hash_build(fp) FILE *fp;
 *
 *		Liest das File `fp' zeilenweise. Jede Zeile enthaelt genau
 *		einen Namen. Blanks werden nicht entfernt. Alle so
 *		gefundenen Namen werden in der Hashtabelle gespeichert.
 *
 *	hash_lookup(str) char *str;
 *
 *		Liefert TRUE, wenn der angegebene String in der
 *		Hashtabelle vorkommt.
 *
 * Scheitert malloc(), gibt es eine Fehlermeldung und exit().
 */

#include <schily/mconfig.h>
#include <stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include "star.h"
#include <schily/string.h>
#include <schily/schily.h>
#include "starsubs.h"

extern	BOOL	notpat;
extern	BOOL	readnull;

#define	HASH_DFLT_SIZE	1024

static struct h_elem {
	struct h_elem	*h_next;
	char		h_data[1];			/* Variable size. */
} **h_tab, **h_xtab;

static size_t	h_size;

EXPORT	size_t	hash_size	__PR((size_t size));
EXPORT	void	hash_build	__PR((FILE *fp));
EXPORT	void	hash_xbuild	__PR((FILE *fp));
LOCAL	void	_hash_build	__PR((FILE *fp, struct h_elem **htab));
EXPORT	BOOL	hash_lookup	__PR((char *str));
EXPORT	BOOL	hash_xlookup	__PR((char *str));
LOCAL	int	hashval		__PR((unsigned char *str, unsigned int maxsize));

EXPORT size_t
hash_size(size)
	size_t	size;
{
	if (h_size == 0)
		h_size = size;
	return (h_size);
}

EXPORT void
hash_build(fp)
	FILE	*fp;
{
	if (h_tab == NULL) {
		register	int	i;
		register	size_t	size = hash_size(HASH_DFLT_SIZE);

		h_tab = ___malloc(size * sizeof (struct h_elem *), "list option");
		for (i = 0; i < size; i++) h_tab[i] = 0;
	}
	_hash_build(fp, h_tab);
}

EXPORT void
hash_xbuild(fp)
	FILE	*fp;
{

	if (h_xtab == NULL) {
		register	int	i;
		register	size_t	size = hash_size(HASH_DFLT_SIZE);

		h_xtab = ___malloc(size * sizeof (struct h_elem *), "exclude option");
		for (i = 0; i < size; i++) h_xtab[i] = 0;
	}
	_hash_build(fp, h_xtab);
}

LOCAL void
_hash_build(fp, htab)
	FILE			*fp;
	register struct h_elem	**htab;
{
	register struct h_elem	*hp;
	register	int	len;
	register	int	hv;
			char	buf[PATH_MAX+1];
	register	size_t	size;

	size = hash_size(HASH_DFLT_SIZE);
	while ((len = readnull ? ngetline(fp, buf, sizeof (buf)) :
				fgetline(fp, buf, sizeof (buf))) >= 0) {
		if (len == 0)
			continue;
		if (len >= PATH_MAX) {
			errmsgno(EX_BAD, "%s: Name too long (%d > %d).\n",
							buf, len, PATH_MAX);
			continue;
		}
		hp = ___malloc((size_t)len + 1 + sizeof (struct h_elem *), "list option");
		strcpy(hp->h_data, buf);
		hv = hashval((unsigned char *)buf, size);
		hp->h_next = htab[hv];
		htab[hv] = hp;
	}
}

/*
 * Hash Include lookup
 * may be modfied via "notpat".
 */
EXPORT BOOL
hash_lookup(str)
	char	*str;
{
	register struct h_elem *hp;
	register int		hv;

	/*
	 * If no include list exists, all files are included.
	 */
	if (h_tab == NULL)
		return (TRUE);

	hv = hashval((unsigned char *)str, h_size);
	for (hp = h_tab[hv]; hp; hp = hp->h_next)
	    if (streql(str, hp->h_data))
		return (!notpat);
	return (notpat);
}

/*
 * Hash eXclude lookup.
 */
EXPORT BOOL
hash_xlookup(str)
	char	*str;
{
	register struct h_elem *hp;
	register int		hv;

	/*
	 * If no exclude list exists, no files are excluded.
	 */
	if (h_tab == NULL)
		return (FALSE);

	hv = hashval((unsigned char *)str, h_size);
	for (hp = h_xtab[hv]; hp; hp = hp->h_next)
	    if (streql(str, hp->h_data))
		return (TRUE);
	return (FALSE);
}

LOCAL int
hashval(str, maxsize)
	register unsigned char *str;
		unsigned	maxsize;
{
	register int	sum = 0;
	register int	i;
	register int	c;

	for (i = 0; (c = *str++) != '\0'; i++)
		sum ^= (c << (i&7));
	return (sum % maxsize);
}
