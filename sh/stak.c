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
/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#pragma ident	"@(#)stak.c	1.12	08/01/29 SMI"

#include "defs.h"

/*
 * This file contains modifications Copyright 2008-2009 J. Schilling
 *
 * @(#)stak.c	1.10 09/07/11 2008-2009 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)stak.c	1.10 09/07/11 2008-2009 J. Schilling";
#endif

/*
 * UNIX shell
 */

	unsigned char *getstak		__PR((Intptr_t asize));
	unsigned char *locstak		__PR((void));
	void		growstak	__PR((unsigned char *newtop));
	unsigned char *savstak		__PR((void));
	unsigned char *endstak		__PR((unsigned char *argp));
	void		tdystak		__PR((unsigned char *x));
	void		stakchk		__PR((void));
	unsigned char *movstrstak	__PR((unsigned char *a, unsigned char *b));
	unsigned char *memcpystak	__PR((unsigned char *s1, unsigned char *s2, int n));


/* ========	storage allocation	======== */

unsigned char *
getstak(asize)			/* allocate requested stack */
Intptr_t	asize;
{
	unsigned char	*oldstak;
	Intptr_t	size;

	size = round(asize, BYTESPERWORD);
	oldstak = stakbot;
	staktop = stakbot += size;
	if (staktop >= brkend)
		growstak(staktop);
	return(oldstak);
}

/*
 * set up stack for local use
 * should be followed by `endstak'
 */
unsigned char *
locstak()
{
	if (brkend - stakbot < BRKINCR)
	{
		if (setbrk(brkincr) == (unsigned char *)-1)
			error(nostack);
		if (brkincr < BRKMAX)
			brkincr += 256;
	}
	return(stakbot);
}

void
growstak(newtop)
unsigned char	*newtop;
{
	UIntptr_t	incr;

	incr = (UIntptr_t)round(newtop - brkend + 1, BYTESPERWORD);
	if (brkincr > incr)
		incr = brkincr;
	if (setbrk(incr) == (unsigned char *)-1)
		error(nospace);
}

unsigned char *
savstak()
{
	assert(staktop == stakbot);
	return(stakbot);
}

unsigned char *
endstak(argp)				/* tidy up after `locstak' */
	unsigned char	*argp;
{
	unsigned char	*oldstak;

	if (argp >= brkend)
		growstak(argp);
	*argp++ = 0;
	oldstak = stakbot;
	stakbot = staktop = (unsigned char *)round(argp, BYTESPERWORD);
	if (staktop >= brkend)
		growstak(staktop);
	return(oldstak);
}

void
tdystak(x)				/* try to bring stack back to x */
	unsigned char	*x;
{
	struct blk *next;

	while ((unsigned char *)stakbsy > x)
	{
		next = stakbsy->word;
		free(stakbsy);
		stakbsy = next;
	}
	staktop = stakbot = max(x, stakbas);
	rmtemp((struct ionod *)x);	/* XXX cheat */
}

void
stakchk()
{
	if ((brkend - stakbas) > BRKINCR + BRKINCR)
		setbrk(-BRKINCR);
}

unsigned char *
cpystak(x)
unsigned char	*x;
{
	return(endstak(movstrstak(x, locstak())));
}

unsigned char *
movstrstak(a, b)
	unsigned char	*a;
	unsigned char	*b;
{
	do
	{
		if (b >= brkend)
			growstak(b);
	}
	while ((*b++ = *a++) != '\0');
	return(--b);
}

/*
 * Copy s2 to s1, always copy n bytes.
 * Return s1
 */
unsigned char *
memcpystak(s1, s2, n)
	unsigned char	*s1;
	unsigned char	*s2;
	int		n;
{
	unsigned char *os1 = s1;

	while (--n >= 0) {
		if (s1 >= brkend)
			growstak(s1);
		*s1++ = *s2++;
	}
	return (os1);
}
