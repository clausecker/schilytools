/* @(#)memory.c	1.21 09/10/16 Copyright 1985-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)memory.c	1.21 09/10/16 Copyright 1985-2009 J. Schilling";
#endif
/*
 *	Make program
 *	Memory allocation routines
 *
 *	Copyright (c) 1985-2009 by J. Schilling
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
#include <schily/standard.h>
#include <schily/types.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/schily.h>
#include "make.h"

EXPORT	char	*gbuf;
EXPORT	char	*gbufend;
EXPORT	int	gbufsize;

EXPORT	void	*___realloc	__PR((void *ptr, size_t size, char *msg));
LOCAL	char	*checkalloc	__PR((unsigned int size));
#ifdef	DEBUG
EXPORT	void	prmem		__PR((void));
#endif
EXPORT	char	*fastalloc	__PR((unsigned int size));
EXPORT	void	freelist	__PR((list_t *l));
EXPORT	char	*strsave	__PR((char *p));
EXPORT	char	*initgbuf	__PR((int size));
EXPORT	char	*growgbuf	__PR((char *p));

#ifdef	DEBUG
char	*heapanfang;
char	*heapende;
int	sumfastfree;
int	countfastfree;
#endif

EXPORT void *
___realloc(ptr, size, msg)
	void	*ptr;
	size_t	size;
	char	*msg;
{
	void	*ret;

	if (ptr == NULL)
		ret = malloc(size);
	else
		ret = realloc(ptr, size);
	if (ret == NULL) {
		comerr("Cannot realloc memory for %s.\n", msg);
		/* NOTREACHED */
	}
#ifdef	DEBUG
	if (heapanfang == 0)
		heapanfang = ret;
	if (heapende < (ret + size))
		heapende = ret + size;
#endif
	return (ret);
}

/*
 * Memory allocation with error handling
 */
LOCAL char *
checkalloc(size)
	unsigned	size;
{
	char	*result;

	if ((result = malloc(size)) == NULL)
		comerr("No memory\n");
#ifdef	DEBUG
	if (heapanfang == 0)
		heapanfang = result;
	if (heapende < (result + size))
		heapende = result + size;
#endif
	return (result);
}


LOCAL	char	**ffstack;

#ifdef	DEBUG
/*
 * Print memory usage. Used to debug 'smake' size at end of execution.
 */
EXPORT void
prmem()
{
	int	i = 0;
	char	**p = ffstack;

	while (p) {
		i++;
		p = (char **)*p;
	}
	printf("start: %lX end: %lX size: %d freed: %d, freecnt: %d, faststacksize: %d\n",
		(long)heapanfang, (long)heapende, heapende - heapanfang,
		sumfastfree, countfastfree, i);
	printf("gbufsize: %d\n", gbufsize);
}
#endif


/*
 * malloc() needs 16 bytes for each allocated chunk.
 * We need to allocate 8 bytes in most cases (hundereds or thousands of
 * times) and therefore try to optimize things.
 */
EXPORT char *
fastalloc(size)
	unsigned	size;
{
	static	char	*spc	= NULL;
	static	char	*spcend = NULL;
	register char	*result	= spc;

	/*
	 * Round things up to sizeof (ptr)
	 * XXX This may not work for double * or 64 Bit architectures.
	 * XXX Should rather include schily/align.h and use the right
	 * XXX macros.
	 */
/*#define	round_up(x)		((x) + (4 - ((x) & 3)))*/
#define	ALIGN_PTR		(sizeof (char *))
#define	ALIGN_PMASK		(sizeof (char *)-1)
/*#define	round_up(x)		((x) + (ALIGN_PTR - ((x) & ALIGN_PMASK)))*/
#define	round_up(x)		((x) + (ALIGN_PTR - 1 - (((x)-1) & ALIGN_PMASK)))

	size = round_up(size);

	if (ffstack && size == sizeof (list_t)) {
		result = (char *)ffstack;
		ffstack = (char **)*ffstack;
		return (result);
	}

	if (result + size > spcend) {

		if (size >= 4*sizeof (long))
			return (checkalloc(size)); /* keep rest of space */

		spc =    result = checkalloc(512);
		spcend = result + 512;
	}
	spc += size;
	return (result);
}

/*
 * Return a chunk of storage to the fast allocation stack for re-usage
 * by fastalloc().
 */
EXPORT void
fastfree(p, size)
	char	*p;
	unsigned int size;
{
	char	**pp;

	if (size != sizeof (list_t))
		comerrno(EX_BAD, "Panic: fastfree size.\n");
#ifdef	DEBUG
	sumfastfree += size;
	countfastfree++;
#endif
	pp = (char **)p;
	*pp = (char *)ffstack;
	ffstack = pp;
}

/*
 * Free a list chain. Don't free the objects behind as they might
 * be still in use.
 */
EXPORT void
freelist(l)
	register list_t	*l;
{
	register list_t	*next;

	while (l) {
		next = l->l_next;
		fastfree((char *)l, sizeof (*l));
		l = next;
	}
}

/*
 * Copy a string into allocated memory and return a pointer to the allocated
 * memory.
 */
EXPORT char *
strsave(p)
	char	*p;
{
	unsigned i	= strlen(p) + 1;
	char	 *res	= fastalloc(i);

	if (i > 16) {
		movebytes(p, res, (int) i);
	} else {
		char	*p2 = res;

		while ((*p2++ = *p++) != '\0') {
			;
			/* LINTED */
		}
	}
	return (res);
}

/*
 * Create growable general purpose buffer.
 */
EXPORT char *
initgbuf(size)
	int	size;
{
	if (gbuf) {
		if (size <= gbufsize)
			return (gbuf);
		free(gbuf);
	}
	gbuf = checkalloc((unsigned) (gbufsize = size));
	gbufend = gbuf + size;
	return (gbuf);
}

/*
 * INCR_GSIZE must be a multiple of INIT_GSIZE and INCR_GSIZE/INIT_GSIZE
 * must be a value that is a power of two.
 */
#define	INIT_GSIZE	512	/* Initial size of the gbuf		*/
#define	INCR_GSIZE	8192	/* Size to grow gbuf in incrementals	*/

#ifdef	GROWTEST
#define	INIT_GSIZE	4	/* Initial size of the gbuf		*/
#define	INCR_GSIZE	4	/* Size to grow gbuf in incrementals	*/
#endif

/*
 * Grow growable general purpose buffer.
 */
EXPORT char *
growgbuf(p)
	char	*p;
{
	register char	*new;
	register int	newsize = gbufsize;

	if (gbufsize == 0)
		return (initgbuf(INIT_GSIZE));
	if (newsize < INCR_GSIZE)
		newsize *= 2;
	else
		newsize += INCR_GSIZE;

	new = checkalloc((unsigned)newsize);
	movebytes(gbuf, new, gbufsize);
	free(gbuf);
	p += new - gbuf;	/* Re-adjust pointer into callers buffer. */
	gbuf = new;
	gbufend = new + newsize;
	gbufsize = newsize;
	return (p);
}
