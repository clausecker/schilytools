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
 * Copyright 2015-2020 J. Schilling
 *
 * @(#)debug.c	1.1 20/05/09 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)debug.c 1.1 20/05/09 J. Schilling"
#endif
#include <defines.h>
#include <version.h>
#include <schily/schily.h>

static int	libsccs_debug;

EXPORT int
sccs_debug()
{
	return (libsccs_debug);
}

EXPORT int
sccs_setdebug(val)
	int	val;
{
	int	old	= libsccs_debug;

	libsccs_debug = val;

	return (old);
}
