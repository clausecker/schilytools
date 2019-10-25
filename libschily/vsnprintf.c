/* @(#)vsnprintf.c	1.1 19/10/19 Copyright 1985, 1996-2019 J. Schilling */
/*
 *	Copyright (c) 1985, 1996-2019 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#define	vsnprintf __nothing__	/* prototype may be wrong (e.g. IRIX) */
#include <schily/mconfig.h>
#include <schily/unistd.h>	/* include <sys/types.h> try to get size_t */
#include <schily/stdio.h>	/* Try again for size_t	*/
#include <schily/stdlib.h>	/* Try again for size_t	*/
#include <schily/varargs.h>
#include <schily/standard.h>
#include <schily/schily.h>
#undef	vsnprintf

/*
 * If PORT_ONLY is defined, vsnprintf() will only be compiled in if it is
 * missing on the local platform. This is used by e.g. libschily.
 */
#if	!defined(HAVE_VSNPRINTF) || !defined(PORT_ONLY)

EXPORT	int vsnprintf __PR((char *, size_t maxcnt, const char *, va_list));

typedef struct {
	char	*ptr;
	int	count;
} *BUF, _BUF;

#ifdef	PROTOTYPES
static void
_cput(char c, void *l)
#else
static void
_cput(c, l)
	char	c;
	void	*l;
#endif
{
	register BUF	bp = (BUF)l;

	if (--bp->count > 0) {
		*bp->ptr++ = c;
	} else {
		/*
		 * Make sure that there will never be a negative overflow.
		 */
		bp->count++;
	}
}

EXPORT int
vsnprintf(buf, maxcnt, form, args)
	char		*buf;
	size_t		maxcnt;
	const char	*form;
	va_list		args;
{
	int	cnt;
	_BUF	bb;

	bb.ptr = buf;
	bb.count = maxcnt;

	cnt = format(_cput, &bb, form, args);
	if (maxcnt > 0)
		*(bb.ptr) = '\0';
	if (bb.count < 0)
		return (-1);

	return (cnt);
}

#endif	/* !defined(HAVE_VSNPRINTF) || !defined(PORT_ONLY) */
