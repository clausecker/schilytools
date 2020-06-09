/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * This file contains modifications Copyright 2006-2020 J. Schilling
 *
 * @(#)fmalloc.c	1.7 20/06/07 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)fmalloc.c 1.7 20/06/07 J. Schilling"
#endif
/*
 * @(#)fmalloc.c 1.5 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)fmalloc.c"
#pragma ident	"@(#)sccs:lib/mpwlib/fmalloc.c"
#endif

/*
	The functions is this file replace xalloc-xfree-xfreeall from
	the PW library.

	Xalloc allocated words, not bytes, so this adjustment is made
	here.  This inconsistency should eventually be cleaned-up in the
	other source, i.e. each request for memory should be in bytes.

	These functions are complicated by the fact that libc has no
	equivalent to ffreeall.  This requires that pointers to allocated
	arrays be stored here.  If malloc ever has a freeall associated with
	it, most of this code can be discarded.
*/

#include <defines.h>

#define LCHUNK	100

static unsigned	ptrcnt = 0;
static unsigned	listsize = 0;
static void	**ptrlist = NULL;

#ifdef	DBG_MALLOC
void *
dbg_fmalloc(asize, file, line)
unsigned asize;
char	*file;
int	line;
{
	void *ptr;
	char dfile[100];

	if (listsize == 0) {
		listsize = LCHUNK;
		snprintf(dfile, sizeof (dfile), "%s/%s", __FILE__, file);
		if ((ptrlist = (void **)dbg_malloc(sizeof(void *)*listsize, dfile, line)) == NULL)
			fatal(gettext("OUT OF SPACE (ut9)"));
	}
	if (ptrcnt >= listsize) {
		listsize += LCHUNK;
		snprintf(dfile, sizeof (dfile), "%s/%s", __FILE__, file);
		if ((ptrlist = (void **)dbg_realloc((void *) ptrlist,
					sizeof(void *)*listsize, dfile, line)) == NULL)
			fatal(gettext("OUT OF SPACE (ut9)"));
	}

	if ((ptr = dbg_malloc(asize, file, line)) == NULL)
		fatal(gettext("OUT OF SPACE (ut9)"));
	else
		ptrlist[ptrcnt++] = ptr;
	return(ptr);
}
#undef	fmalloc
#endif

void *
fmalloc(asize)
unsigned asize;
{
	void *ptr;

	if (listsize == 0) {
		listsize = LCHUNK;
		if ((ptrlist = (void **)malloc(sizeof(void *)*listsize)) == NULL)
			fatal(gettext("OUT OF SPACE (ut9)"));
	}
	if (ptrcnt >= listsize) {
		listsize += LCHUNK;
		if ((ptrlist = (void **)realloc((void *) ptrlist,
					sizeof(void *)*listsize)) == NULL)
			fatal(gettext("OUT OF SPACE (ut9)"));
	}

	if ((ptr = malloc(asize)) == NULL)
		fatal(gettext("OUT OF SPACE (ut9)"));
	else
		ptrlist[ptrcnt++] = ptr;
	return(ptr);
}

void
ffree(aptr)
void *aptr;
{
	register unsigned cnt;

	cnt = ptrcnt;
	while (cnt)
		if (aptr == ptrlist[--cnt]) {
			free(aptr);
			ptrlist[cnt] = NULL;
			if (cnt == ptrcnt - 1)
				--ptrcnt;
			return;
		}
	fatal(gettext("ffree: Pointer not pointing to allocated area"));
	/*NOTREACHED*/
}

void
ffreeall()
{
	while(ptrcnt)
		if (ptrlist[--ptrcnt] != NULL)
			free(ptrlist[ptrcnt]);
	if (ptrlist != NULL)
		free((void *)ptrlist);
	ptrlist = NULL;
	listsize = 0;
}
