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
 * @(#)lhash.c	1.2 17/02/27 Copyright 1988-2017 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)lhash.c	1.2 17/02/27 Copyright 1988-2017 J. Schilling";
#endif

#if defined(sun)
#pragma ident	"@(#)lhash.c"
#pragma ident	"@(#)sccs:lib/comobj/lhash.c"
#endif
#include	<defines.h>


#define	HASH_DFLT_SIZE	128

static struct h_elem {
	struct h_elem	*h_next;
	char		h_data[1];			/* Variable size. */
} **h_tab;

static size_t	h_size;

EXPORT	size_t	lhash_size	__PR((size_t size));
EXPORT	void	lhash_destroy	__PR((void));
EXPORT	char	*lhash_add	__PR((char *str));
LOCAL	char	*_lhash_add	__PR((char *str, struct h_elem **htab));
EXPORT	char	*lhash_lookup	__PR((char *str));
LOCAL	int	lhashval	__PR((unsigned char *str, unsigned int maxsize));

EXPORT size_t
lhash_size(size)
	size_t	size;
{
	if (h_size == 0)
		h_size = size;
	return (h_size);
}

/*
 * Warning: we use fmalloc() and thus our memory is freed by ffreeall(), but
 * the variables h_size and h_tab keep their values unless we clear them.
 */
EXPORT void
lhash_destroy()
{
	h_size = 0;
	h_tab = NULL;
}

EXPORT char *
lhash_add(str)
	char	*str;
{
	if (h_tab == NULL) {
		register	int	i;
		register	size_t	size = lhash_size(HASH_DFLT_SIZE);

		h_tab = fmalloc(size * sizeof (struct h_elem *));
		for (i = 0; i < size; i++) {
			h_tab[i] = NULL;
		}
	}
	return (_lhash_add(str, h_tab));
}

LOCAL char *
_lhash_add(str, htab)
	char			*str;
	register struct h_elem	**htab;
{
	register struct h_elem	*hp;
	register	int	len;
	register	int	hv;
	register	size_t	size;

	size = lhash_size(HASH_DFLT_SIZE);
	len = strlen(str);
	if (len == 0)
		return ("");

	hp = fmalloc((size_t)len + 1 + sizeof (struct h_elem *));
	strcpy(hp->h_data, str);
	hv = lhashval((unsigned char *)str, size);
	hp->h_next = htab[hv];
	htab[hv] = hp;
	return (hp->h_data);
}

EXPORT char *
lhash_lookup(str)
	char	*str;
{
	register struct h_elem *hp;
	register int		hv;

	if (h_tab == NULL)
		return (lhash_add(str));

	hv = lhashval((unsigned char *)str, h_size);
	for (hp = h_tab[hv]; hp; hp = hp->h_next)
	    if (equal(str, hp->h_data))
		return (hp->h_data);
	return (lhash_add(str));
}

LOCAL int
lhashval(str, maxsize)
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
