/* @(#)fillbytes.c	1.16 07/06/24 Copyright 1987, 1995-2007 J. Schilling */
/*
 *	fill memory with data
 *
 *	Copyright (c) 1987, 1995-2007 J. Schilling
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

#define	cval	((char) lval)

/*
 * fillbytes(to, cnt, val) is the same as memset(to, val, cnt)
 */
#ifdef	PROTOTYPES
EXPORT char *
fillbytes(void *tov, int cnt, char val)
#else
EXPORT char *
fillbytes(tov, cnt, val)
	void	*tov;
	int	cnt;
	char	val;
#endif
{
	register char	*to = (char *)tov;
	register int	n;
	register long	lval;

	/*
	 * If we change cnt to be unsigned, check for == instead of <=
	 */
	if ((n = cnt) <= 0)
		return (to);

	lval = val & 0xFF;

	/*
	 * Assign byte-wise until properly aligned for a long pointer.
	 */
	while (--n >= 0 && !laligned(to)) {
		*to++ = cval;
	}
	n++;

	if (n >= (int)(8 * sizeof (long))) {
		register int rem = n % (8 * sizeof (long));

		lval |= (lval<<8);
		lval |= (lval<<16);
#if SIZE_LONG > SIZE_INT
		lval |= (lval<<32);
#endif

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
				DO8 (*to++ = cval);
			} while ((n -= 8) >= 0);
			n += 8;
		}
		if (n > 0) do {
			*to++ = cval;
		} while (--n > 0);
		return (to);
	}
	if (n > 0) do {
		*to++ = cval;
	} while (--n > 0);
	return (to);
}
