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

#if defined(sun)
#pragma ident	"@(#)stak.c	1.12	08/01/29 SMI"
#endif

#include "defs.h"

/*
 * This file contains modifications Copyright 2008-2012 J. Schilling
 *
 * @(#)stak.c	1.14 12/03/27 2008-2012 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)stak.c	1.14 12/03/27 2008-2012 J. Schilling";
#endif

/*
 * UNIX shell
 */

/*
 * Global variables stakbas, staktop, stakbot, stakbsy, brkend see defs.c
 */

	unsigned char *getstak		__PR((Intptr_t asize));
	unsigned char *locstak		__PR((void));
	unsigned char *growstak		__PR((unsigned char *newtop));
	unsigned char *savstak		__PR((void));
	unsigned char *endstak		__PR((unsigned char *argp));
	void		tdystak		__PR((unsigned char *x));
	void		stakchk		__PR((void));
	unsigned char *movstrstak	__PR((unsigned char *a, unsigned char *b));
	unsigned char *memcpystak	__PR((unsigned char *s1, unsigned char *s2, int n));


/* ========	storage allocation	======== */

/*
 * Turn the current "local" stak into a malloc()ed chunk
 * and start a new local stak by modifying stakbot and staktop.
 * Return the old local stak chunk base address.
 */
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

/*
 * Grow the current local use stak top.
 */
unsigned char *
growstak(newtop)
unsigned char	*newtop;
{
	UIntptr_t	incr;
	UIntptr_t	newoff = newtop - stakbot;

	incr = (UIntptr_t)round(newtop - brkend + 1, BYTESPERWORD);
	if (brkincr > incr)
		incr = brkincr;
	if (setbrk(incr) == (unsigned char *)-1)
		error(nospace);

	return (stakbot + newoff);	/* New value for newtop */
}

/*
 * Return an address to be used by tdystak later.
 */
unsigned char *
savstak()
{
	assert(staktop == stakbot);
	return(stakbot);
}

/*
 * Make the current growing stack a semi-permanent item and 
 * generate a new tiny growing stack.
 * Return the "permanent" address of the old stack item.
 */
unsigned char *
endstak(argp)				/* tidy up after `locstak' */
	unsigned char	*argp;
{
	unsigned char	*oldstak;

	if (argp >= brkend)
		argp = growstak(argp);
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

/*
 * Reduce the growing-stack size if possible
 */
void
stakchk()
{
	if ((brkend - stakbas) > BRKINCR + BRKINCR)
		setbrk(-BRKINCR);
}

/*
 * Copy the string in "x" to the stack and make it semi-permanent.
 * The current local stack is assumed to be empty.
 */
unsigned char *
cpystak(x)
unsigned char	*x;
{
	return(endstak(movstrstak(x, locstak())));
}

/*
 * Append the string in "a" to the string pointed to by "b".
 * "b" must be on the current local stack.
 * Return the address of the nul character at the end of the new string.
 *
 * The stack is kept growable.
 */
unsigned char *
movstrstak(a, b)
	unsigned char	*a;
	unsigned char	*b;
{
	do
	{
		if (b >= brkend)
			b = growstak(b);
	}
	while ((*b++ = *a++) != '\0');
	return(--b);
}

/*
 * Append the string in "s2" to the string pointed to by "s1".
 * "s1" must be on the current local stack.
 * Always copy n bytes from s2 to s1.
 * Return "old value" of s1,
 * taking care of that s1 may have been relocated by growstak().
 *
 * The stack is kept growable.
 */
unsigned char *
memcpystak(s1, s2, n)
	unsigned char	*s1;
	unsigned char	*s2;
	int		n;
{
	int amt = n > 0 ? n : 0;

	while (--n >= 0) {
		if (s1 >= brkend)
			s1 = growstak(s1);
		*s1++ = *s2++;
	}
	return (s1 - amt);
}
