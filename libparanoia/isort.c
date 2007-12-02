/* @(#)isort.c	1.16 06/09/13 J. Schilling from cdparanoia-III-alpha9.8 */
#ifndef lint
static	char sccsid[] =
"@(#)isort.c	1.16 06/09/13 J. Schilling from cdparanoia-III-alpha9.8";

#endif
/*
 * CopyPolicy: GNU Lesser General Public License v2.1 applies
 * Copyright (C) 1997-2001 by Monty (xiphmont@mit.edu)
 * Copyright (C) 2002-2006 by J. Schilling
 *
 * sorted vector abstraction for paranoia
 *
 */

/*
 * Old isort got a bit complex.  This re-constrains complexity to
 * give a go at speed through a more alpha-6-like mechanism.
 */

#include <schily/mconfig.h>
#include <schily/stdlib.h>
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/string.h>
#include "p_block.h"
#include "isort.h"
#include "pmalloc.h"

EXPORT	sort_info	*sort_alloc	__PR((long size));
EXPORT	void		sort_unsortall	__PR((sort_info * i));
EXPORT	void		sort_free	__PR((sort_info * i));
LOCAL	void		sort_sort	__PR((sort_info * i,
						long sortlo, long sorthi));
EXPORT	void		sort_setup	__PR((sort_info * i,
						Int16_t * vector,
						long *abspos, long size,
						long sortlo, long sorthi));
EXPORT	sort_link	*sort_getmatch	__PR((sort_info * i,
						long post, long overlap,
						int value));
EXPORT	sort_link	*sort_nextmatch	__PR((sort_info * i, sort_link * prev));


EXPORT sort_info *
sort_alloc(size)
	long	size;
{
	sort_info	*ret = _pcalloc(1, sizeof (sort_info));

	ret->vector = NULL;
	ret->sortbegin = -1;
	ret->size = -1;
	ret->maxsize = size;

	ret->head = _pcalloc(65536, sizeof (sort_link *));
	ret->bucketusage = _pmalloc(65536 * sizeof (long));
	ret->revindex = _pcalloc(size, sizeof (sort_link));
	ret->lastbucket = 0;

	return (ret);
}

EXPORT void
sort_unsortall(i)
	sort_info	*i;
{
	if (i->lastbucket > 2000) {	/* a guess */
		memset(i->head, 0, 65536 * sizeof (sort_link *));
	} else {
		long	b;

		for (b = 0; b < i->lastbucket; b++)
			i->head[i->bucketusage[b]] = NULL;
	}

	i->lastbucket = 0;
	i->sortbegin = -1;
}

EXPORT void
sort_free(i)
	sort_info	*i;
{
	_pfree(i->revindex);
	_pfree(i->head);
	_pfree(i->bucketusage);
	_pfree(i);
}

LOCAL void
sort_sort(i, sortlo, sorthi)
	sort_info	*i;
	long		sortlo;
	long		sorthi;
{
	long	j;

	for (j = sorthi - 1; j >= sortlo; j--) {
		sort_link	**hv = i->head + i->vector[j] + 32768;
		sort_link	 *l = i->revindex + j;

		if (*hv == NULL) {
			i->bucketusage[i->lastbucket] = i->vector[j] + 32768;
			i->lastbucket++;
		}
		l->next = *hv;
		*hv = l;
	}
	i->sortbegin = 0;
}

/*
 * size *must* be less than i->maxsize
 */
EXPORT void
sort_setup(i, vector, abspos, size, sortlo, sorthi)
	sort_info	*i;
	Int16_t		*vector;
	long		*abspos;
	long		size;
	long		sortlo;
	long		sorthi;
{
	if (i->sortbegin != -1)
		sort_unsortall(i);

	i->vector = vector;
	i->size = size;
	i->abspos = abspos;

	i->lo = min(size, max(sortlo - *abspos, 0));
	i->hi = max(0, min(sorthi - *abspos, size));
}

EXPORT sort_link *
sort_getmatch(i, post, overlap, value)
	sort_info	*i;
	long		post;
	long		overlap;
	int		value;
{
	sort_link	*ret;

	if (i->sortbegin == -1)
		sort_sort(i, i->lo, i->hi);
	/*
	 * Now we reuse lo and hi
	 */
	post = max(0, min(i->size, post));
	i->val = value + 32768;
	i->lo = max(0, post - overlap);		/* absolute position */
	i->hi = min(i->size, post + overlap);	/* absolute position */

	ret = i->head[i->val];
	while (ret) {
		if (ipos(i, ret) < i->lo) {
			ret = ret->next;
		} else {
			if (ipos(i, ret) >= i->hi)
				ret = NULL;
			break;
		}
	}
/*	i->head[i->val]=ret; */
	return (ret);
}

EXPORT sort_link *
sort_nextmatch(i, prev)
	sort_info	*i;
	sort_link	*prev;
{
	sort_link	*ret = prev->next;

	if (!ret || ipos(i, ret) >= i->hi)
		return (NULL);
	return (ret);
}
