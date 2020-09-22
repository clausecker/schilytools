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
 * Copyright 1994 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 1987, 2006-2020 J. Schilling
 *
 * @(#)zero.c	1.8 20/09/06 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)zero.c 1.8 20/09/06 J. Schilling"
#endif
/*
 * @(#)zero.c 1.3 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)zero.c"
#pragma ident	"@(#)sccs:lib/mpwlib/zero.c"
#endif
/*
	Zero `cnt' bytes starting at the address `ptr'.
	Return `ptr'.
*/

#include <defines.h>

#ifdef	pdp11

char	*zero(p,n)
register char *p;
register int n;
{
	char *op = p;
	while (--n >= 0)
		*p++ = 0;
	return(op);
}

#else	/* !pdp11 */

#include <schily/align.h>

#define	DO8(a)	a; a; a; a; a; a; a; a;

/*
 * zero(to, cnt)
 */
EXPORT char *
zero(to, cnt)
	register char	*to;
	int	cnt;
{
		char	*oto = to;
	register ssize_t n;
	register long	lval = 0L;

	/*
	 * If we change cnt to be unsigned, check for == instead of <=
	 */
	if ((n = cnt) <= 0)
		return (to);

	if (n < 8 * sizeof (long)) {	/* Simple may be faster... */
		do {			/* n is always > 0 */
			*to++ = '\0';
		} while (--n > 0);
		return (oto);
	}

	/*
	 * Assign byte-wise until properly aligned for a long pointer.
	 */
	while (--n >= 0 && !laligned(to)) {
		*to++ = '\0';
	}
	n++;

	if (n >= (ssize_t)(8 * sizeof (long))) {
		register ssize_t rem = n % (8 * sizeof (long));

		n /= (8 * sizeof (long));
		{
			register long *tol = (long *)to;

			do {
				DO8 (*tol++ = lval);
			} while (--n > 0);

			to = (char *)tol;
		}
		n = rem;

		if (n >= 8) {
			n -= 8;
			do {
				DO8 (*to++ = '\0');
			} while ((n -= 8) >= 0);
			n += 8;
		}
		if (n > 0) do {
			*to++ = '\0';
		} while (--n > 0);
		return (oto);
	}
	if (n > 0) do {
		*to++ = '\0';
	} while (--n > 0);
	return (oto);
}

#endif	/* !pdp11 */
