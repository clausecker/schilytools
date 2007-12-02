/* @(#)isort.h	1.11 06/05/06 J. Schilling from cdparanoia-III-alpha9.8 */
/*
 * CopyPolicy: GNU Lesser General Public License v2.1 applies
 * Copyright (C) 1997-2001 by Monty (xiphmont@mit.edu)
 * Copyright (C) 2002-2006 by J. Schilling
 */

#ifndef	_ISORT_H
#define	_ISORT_H

typedef struct sort_link {
	struct sort_link *next;
} sort_link;

typedef struct sort_info {
	Int16_t		*vector;	/* vector */
					/* vec storage doesn't belong to us */

	long		*abspos;	/* pointer for side effects */
	long		size;		/* vector size */

	long		maxsize;	/* maximum vector size */

	long		sortbegin;	/* range of contiguous sorted area */
	long		lo;
	long		hi;		/* current post, overlap range */
	int		val;		/* ...and val */

	/*
	 * sort structs
	 */
	sort_link	**head;		/* sort buckets (65536) */

	long		*bucketusage;	/* of used buckets (65536) */
	long		lastbucket;
	sort_link	*revindex;

} sort_info;

extern sort_info	*sort_alloc	__PR((long size));
extern void		sort_unsortall	__PR((sort_info * i));
extern void		sort_setup	__PR((sort_info * i, Int16_t * vector,
						long *abspos, long size,
						long sortlo, long sorthi));
extern void		sort_free	__PR((sort_info * i));
extern sort_link	*sort_getmatch	__PR((sort_info * i, long post,
						long overlap, int value));
extern sort_link	*sort_nextmatch	__PR((sort_info * i, sort_link * prev));

#define	is(i)		((i)->size)
#define	ib(i)		(*(i)->abspos)
#define	ie(i)		((i)->size + *(i)->abspos)
#define	iv(i)		((i)->vector)
#define	ipos(i, l)	((l) - (i)->revindex)

#endif	/* _ISORT_H */
