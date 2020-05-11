/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may use this file only in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
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
 * Copyright 2005 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 * from "@(#)sccs.c 1.2 2/27/90"
 */

/*
 * Copyright 2015-2020 J. Schilling
 *
 * @(#)usrerr.c	1.1 20/05/09 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)usrerr.c 1.1 20/05/09 J. Schilling"
#endif
#include <defines.h>
#include <version.h>
#include <schily/stat.h>
#include <schily/getopt.h>
#include <schily/sysexits.h>
#include <schily/schily.h>

/*
 *  USRERR -- issue user-level error
 *
 *	Parameters:
 *		f -- format string.
 *		p1-p3 -- parameters to a printf.
 *
 *	Returns:
 *		-1
 *
 *	Side Effects:
 *		none.
 */

#ifdef	PROTOTYPES
EXPORT int
usrerr(const char *f, ...)
#else
EXPORT int
usrerr(f, va_alist)
	const char	*f;
	va_dcl
#endif
{
	va_list	ap;
	char	*errstr = NULL;

#ifdef	SCHILY_BUILD
	errstr = get_progname();
#else
#ifdef	HAVE_GETPROGNAME
	errstr = (char *)getprogname();
#endif
#endif
	if (errstr) {
		fprintf(stderr, "\n%s: ", errstr);
	}

#ifdef	PROTOTYPES
	va_start(ap, f);
#else
	va_start(ap);
#endif
	vfprintf(stderr, f, ap);
	fprintf(stderr, "\n");
	va_end(ap);

#ifdef	SCCS_FATALHELP
	if (strchr(f, '(')) {
		sccsfatalhelp((char *)f);
		errno = 0;
	}
#endif
	return (-1);
}
