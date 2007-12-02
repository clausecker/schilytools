/* @(#)movebytes.c	1.16 07/06/24 Copyright 1985, 1989, 1995-2007 J. Schilling */
/*
 *	move data
 *
 *	Copyright (c) 1985, 1989, 1995-2007 J. Schilling
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

#include <schily/standard.h>
#include <schily/align.h>
#include <schily/schily.h>

#define	DO8(a)	a; a; a; a; a; a; a; a;

/*
 * movebytes(from, to, cnt) is the same as memmove(to, from, cnt)
 */
EXPORT char *
movebytes(fromv, tov, cnt)
	const void	*fromv;
	void		*tov;
	int		cnt;
{
	register const char	*from	= fromv;
	register char		*to	= tov;
	register int		n;

	/*
	 * If we change cnt to be unsigned, check for == instead of <=
	 */
	if ((n = cnt) <= 0)
		return (to);

	if (from >= to) {
		/*
		 * source is on higher adresses than destination:
		 *	move bytes forwards
		 */
		if (n >= (int)(8 * sizeof (long))) {
			if (l2aligned(from, to)) {
				register const long *froml = (const long *)from;
				register long *tol = (long *)to;
				register int rem = n % (8 * sizeof (long));

				n /= (8 * sizeof (long));
				do {
					DO8 (*tol++ = *froml++);
				} while (--n > 0);

				from = (const char *)froml;
				to = (char *)tol;
				n = rem;
			}

			if (n >= 8) {
				n -= 8;
				do {
					DO8 (*to++ = *from++);
				} while ((n -= 8) >= 0);
				n += 8;
			}

			if (n > 0) do {
				*to++ = *from++;
			} while (--n > 0);
			return (to);
		}
		if (n > 0) do {
			*to++ = *from++;
		} while (--n > 0);
		return (to);
	} else {
		char *ep;

		/*
		 * source is on lower adresses than destination:
		 *	move bytes backwards
		 */
		to += n;
		from += n;
		ep = to;
		if (n >= (int)(8 * sizeof (long))) {
			if (l2aligned(from, to)) {
				register const long *froml = (const long *)from;
				register long *tol = (long *)to;
				register int rem = n % (8 * sizeof (long));

				n /= (8 * sizeof (long));
				do {
					DO8 (*--tol = *--froml);
				} while (--n > 0);

				from = (const char *)froml;
				to = (char *)tol;
				n = rem;
			}
			if (n >= 8) {
				n -= 8;
				do {
					DO8 (*--to = *--from);
				} while ((n -= 8) >= 0);
				n += 8;
			}
			if (n > 0) do {
				*--to = *--from;
			} while (--n > 0);
			return (ep);
		}
		if (n > 0) do {
			*--to = *--from;
		} while (--n > 0);
		return (ep);
	}
}
