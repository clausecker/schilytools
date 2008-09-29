/* @(#)append.c	1.25 08/09/26 Copyright 1992, 2001-2008 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)append.c	1.25 08/09/26 Copyright 1992, 2001-2008 J. Schilling";
#endif
/*
 *	Routines used to append files to an existing
 *	tape archive
 *
 *	Copyright (c) 1992, 2001-2008 J. Schilling
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
#include <schily/unistd.h>
#include <schily/standard.h>
#include "star.h"
#include <schily/schily.h>
#include "starsubs.h"

extern	FILE	*vpr;

extern	BOOL	debug;
extern	BOOL	cflag;
extern	BOOL	uflag;
extern	BOOL	rflag;

/*
 * XXX We should try to share the hash code with lhash.c
 */
static struct h_elem {
	struct h_elem *h_next;
	time_t		h_time;
	long		h_nsec;
	short		h_len;
	char		h_flags;
	char		h_data[1];			/* Variable size. */
} **h_tab;

#define	HF_NSEC		0x01				/* have nsecs	*/

static	unsigned	h_size;
LOCAL	int		cachesize;

EXPORT	void	skipall		__PR((void));
LOCAL	void	hash_new	__PR((size_t size));
LOCAL	struct h_elem *	uhash_lookup	__PR((FINFO *info));
LOCAL	void	hash_add	__PR((FINFO *info));
EXPORT	BOOL	update_newer	__PR((FINFO *info));
LOCAL	int	hashval		__PR((Uchar *str, Uint maxsize));

EXPORT void
skipall()
{
		FINFO	finfo;
		TCB	tb;
		char	name[PATH_MAX+1];
		char	lname[PATH_MAX+1];
	register TCB 	*ptb = &tb;

	if (uflag)
		hash_new(100);

	fillbytes((char *)&finfo, sizeof (finfo), '\0');

	finfo.f_tcb = ptb;
	for (;;) {
		if (get_tcb(ptb) == EOF)
			break;

		finfo.f_name = name;
		finfo.f_lname = lname;
		if (tcb_to_info(ptb, &finfo) == EOF)
			break;

		if (debug)
			fprintf(vpr, "R %s\n", finfo.f_name);
		if (uflag)
			hash_add(&finfo);

		void_file(&finfo);
	}
	if (debug)
		error("used %d bytes for update cache.\n", cachesize);
}

#include <schily/string.h>

LOCAL void
hash_new(size)
	size_t	size;
{
	register	int	i;

	h_size = size;
	h_tab = (struct h_elem **)___malloc(size * sizeof (struct h_elem *), "new hash");
	for (i = 0; i < size; i++) h_tab[i] = 0;

	cachesize += size * sizeof (struct h_elem *);
}

LOCAL struct h_elem *
uhash_lookup(info)
	FINFO	*info;
{
	register char		*name = info->f_name;
	register struct h_elem *hp;
	register int		hv;
		BOOL		slash = FALSE;

	if (info->f_namelen == 0)
		info->f_namelen = strlen(info->f_name);

	if (is_dir(info) && info->f_name[info->f_namelen-1] == '/') {
		info->f_name[--info->f_namelen] = '\0';
		slash = TRUE;
	}

	hv = hashval((unsigned char *)name, h_size);
	for (hp = h_tab[hv]; hp; hp = hp->h_next) {
		if (info->f_namelen == hp->h_len && *name == *hp->h_data) {
			if (streql(name, hp->h_data)) {
				if (slash)
					info->f_name[info->f_namelen++] = '/';
				return (hp);
			}
		}
	}
	if (slash)
		info->f_name[info->f_namelen++] = '/';
	return (0);
}

LOCAL void
hash_add(info)
	FINFO	*info;
{
	register	int	len;
	register struct h_elem	*hp;
	register	int	hv;
			BOOL	slash = FALSE;

	/*
	 * XXX nsec korrekt implementiert?
	 */
	if (info->f_namelen == 0)
		info->f_namelen = strlen(info->f_name);

	if (is_dir(info) && info->f_name[info->f_namelen-1] == '/') {
		info->f_name[--info->f_namelen] = '\0';
		slash = TRUE;
	}
	if ((hp = uhash_lookup(info)) != 0) {
		if (hp->h_time < info->f_mtime) {
			hp->h_time = info->f_mtime;
			hp->h_nsec = info->f_mnsec;
		} else if (hp->h_time == info->f_mtime) {
			/*
			 * If the current archive entry holds extended
			 * time imformation, honor it.
			 */
			if (info->f_xflags & XF_MTIME)
				hp->h_flags |= HF_NSEC;
			else
				hp->h_flags &= ~HF_NSEC;

			if ((hp->h_flags & HF_NSEC) &&
			    (hp->h_nsec < info->f_mnsec))
				hp->h_nsec = info->f_mnsec;
		}
		if (slash)
			info->f_name[info->f_namelen++] = '/';
		return;
	}

	len = info->f_namelen;
	hp = ___malloc((size_t)len + sizeof (struct h_elem), "add hash");
	cachesize += len + sizeof (struct h_elem);
	strcpy(hp->h_data, info->f_name);
	hp->h_time = info->f_mtime;
	hp->h_nsec = info->f_mnsec;
	hp->h_len = len;
	hp->h_flags = 0;
	if (info->f_xflags & XF_MTIME)
		hp->h_flags |= HF_NSEC;
	hv = hashval((unsigned char *)info->f_name, h_size);
	hp->h_next = h_tab[hv];
	h_tab[hv] = hp;
	if (slash)
		info->f_name[info->f_namelen++] = '/';
}

EXPORT BOOL
update_newer(info)
	FINFO	*info;
{
	register struct h_elem *hp;

	/*
	 * XXX nsec korrekt implementiert?
	 */
	if ((hp = uhash_lookup(info)) != 0) {
		if (info->f_mtime > hp->h_time)
			return (TRUE);
		if ((hp->h_flags & HF_NSEC) && (info->f_flags & F_NSECS) &&
		    info->f_mtime == hp->h_time && info->f_mnsec > hp->h_nsec)
			return (TRUE);
		return (FALSE);
	}
	return (TRUE);
}

LOCAL int
hashval(str, maxsize)
	register Uchar *str;
		Uint	maxsize;
{
	register int	sum = 0;
	register int	i;
	register int	c;

	for (i = 0; (c = *str++) != '\0'; i++)
		sum ^= (c << (i&7));
	return (sum % maxsize);
}
