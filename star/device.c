/* @(#)device.c	1.19 19/08/07 Copyright 1996-2019 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)device.c	1.19 19/08/07 Copyright 1996-2019 J. Schilling";
#endif
/*
 *	Handle local and remote device major/minor mappings
 *
 *	Copyright (c) 1996-2019 J. Schilling
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

#include <schily/standard.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#define	__XDEV__	/* Needed to activate XDEV_T definitions */
#include <schily/device.h>

#ifdef	DEV_DEFAULTS
EXPORT	int	minorbits = 8;
EXPORT	XDEV_T	minormask = 0xFFLU;
#else
EXPORT	int	minorbits;
EXPORT	XDEV_T	minormask;
#endif

#ifdef	__STDC__
EXPORT	XDEV_T	_dev_mask[] = {
	0x00000000LU,
	0x00000001LU,
	0x00000003LU,
	0x00000007LU,
	0x0000000FLU,
	0x0000001FLU,
	0x0000003FLU,
	0x0000007FLU,
	0x000000FFLU,
	0x000001FFLU,
	0x000003FFLU,
	0x000007FFLU,
	0x00000FFFLU,
	0x00001FFFLU,
	0x00003FFFLU,
	0x00007FFFLU,
	0x0000FFFFLU,
	0x0001FFFFLU,
	0x0003FFFFLU,
	0x0007FFFFLU,
	0x000FFFFFLU,
	0x001FFFFFLU,
	0x003FFFFFLU,
	0x007FFFFFLU,
	0x00FFFFFFLU,
	0x01FFFFFFLU,
	0x03FFFFFFLU,
	0x07FFFFFFLU,
	0x0FFFFFFFLU,
	0x1FFFFFFFLU,
	0x3FFFFFFFLU,
	0x7FFFFFFFLU,
	0xFFFFFFFFLU,
#if SIZEOF_ULLONG > 4
	0x00000001FFFFFFFFLLU,
	0x00000003FFFFFFFFLLU,
	0x00000007FFFFFFFFLLU,
	0x0000000FFFFFFFFFLLU,
	0x0000001FFFFFFFFFLLU,
	0x0000003FFFFFFFFFLLU,
	0x0000007FFFFFFFFFLLU,
	0x000000FFFFFFFFFFLLU,
	0x000001FFFFFFFFFFLLU,
	0x000003FFFFFFFFFFLLU,
	0x000007FFFFFFFFFFLLU,
	0x00000FFFFFFFFFFFLLU,
	0x00001FFFFFFFFFFFLLU,
	0x00003FFFFFFFFFFFLLU,
	0x00007FFFFFFFFFFFLLU,
	0x0000FFFFFFFFFFFFLLU,
	0x0001FFFFFFFFFFFFLLU,
	0x0003FFFFFFFFFFFFLLU,
	0x0007FFFFFFFFFFFFLLU,
	0x000FFFFFFFFFFFFFLLU,
	0x001FFFFFFFFFFFFFLLU,
	0x003FFFFFFFFFFFFFLLU,
	0x007FFFFFFFFFFFFFLLU,
	0x00FFFFFFFFFFFFFFLLU,
	0x01FFFFFFFFFFFFFFLLU,
	0x03FFFFFFFFFFFFFFLLU,
	0x07FFFFFFFFFFFFFFLLU,
	0x0FFFFFFFFFFFFFFFLLU,
	0x1FFFFFFFFFFFFFFFLLU,
	0x3FFFFFFFFFFFFFFFLLU,
	0x7FFFFFFFFFFFFFFFLLU,
	0xFFFFFFFFFFFFFFFFLLU,
#endif
};
#else
EXPORT	XDEV_T	_dev_mask[] = {
	0x00000000L,
	0x00000001L,
	0x00000003L,
	0x00000007L,
	0x0000000FL,
	0x0000001FL,
	0x0000003FL,
	0x0000007FL,
	0x000000FFL,
	0x000001FFL,
	0x000003FFL,
	0x000007FFL,
	0x00000FFFL,
	0x00001FFFL,
	0x00003FFFL,
	0x00007FFFL,
	0x0000FFFFL,
	0x0001FFFFL,
	0x0003FFFFL,
	0x0007FFFFL,
	0x000FFFFFL,
	0x001FFFFFL,
	0x003FFFFFL,
	0x007FFFFFL,
	0x00FFFFFFL,
	0x01FFFFFFL,
	0x03FFFFFFL,
	0x07FFFFFFL,
	0x0FFFFFFFL,
	0x1FFFFFFFL,
	0x3FFFFFFFL,
	0x7FFFFFFFL,
	0xFFFFFFFFL,
#if SIZEOF_ULLONG > 4
	0x00000001FFFFFFFFLL,
	0x00000003FFFFFFFFLL,
	0x00000007FFFFFFFFLL,
	0x0000000FFFFFFFFFLL,
	0x0000001FFFFFFFFFLL,
	0x0000003FFFFFFFFFLL,
	0x0000007FFFFFFFFFLL,
	0x000000FFFFFFFFFFLL,
	0x000001FFFFFFFFFFLL,
	0x000003FFFFFFFFFFLL,
	0x000007FFFFFFFFFFLL,
	0x00000FFFFFFFFFFFLL,
	0x00001FFFFFFFFFFFLL,
	0x00003FFFFFFFFFFFLL,
	0x00007FFFFFFFFFFFLL,
	0x0000FFFFFFFFFFFFLL,
	0x0001FFFFFFFFFFFFLL,
	0x0003FFFFFFFFFFFFLL,
	0x0007FFFFFFFFFFFFLL,
	0x000FFFFFFFFFFFFFLL,
	0x001FFFFFFFFFFFFFLL,
	0x003FFFFFFFFFFFFFLL,
	0x007FFFFFFFFFFFFFLL,
	0x00FFFFFFFFFFFFFFLL,
	0x01FFFFFFFFFFFFFFLL,
	0x03FFFFFFFFFFFFFFLL,
	0x07FFFFFFFFFFFFFFLL,
	0x0FFFFFFFFFFFFFFFLL,
	0x1FFFFFFFFFFFFFFFLL,
	0x3FFFFFFFFFFFFFFFLL,
	0x7FFFFFFFFFFFFFFFLL,
	0xFFFFFFFFFFFFFFFFLL,
#endif
};
#endif

EXPORT void
dev_init(debug)
	BOOL	debug;
{
#ifdef	DEV_MINOR_NONCONTIG
	int	i = 0;
#else
	int	i;
	dev_t	x;

	for (i = 0, x = 1; minor(x) == x; i++, x <<= 1)
		/* LINTED */
		;
#endif

	minorbits = i;
	minormask = _dev_mask[i];

	if (debug)
		error("dev_minorbits:    %d\n", minorbits);
}

#ifdef	IS_LIBRARY

#undef	dev_major
EXPORT XDEV_T
dev_major(dev)
	XDEV_T	dev;
{
	return (dev >> minorbits);
}

#undef	_dev_major
EXPORT XDEV_T
_dev_major(mbits, dev)
	int	mbits;
	XDEV_T	dev;
{
	return (dev >> mbits);
}

#undef	dev_minor
EXPORT XDEV_T
dev_minor(dev)
	XDEV_T	dev;
{
	return (dev & minormask);
}

#undef	_dev_minor
EXPORT XDEV_T
_dev_minor(mbits, dev)
	int	mbits;
	XDEV_T	dev;
{
	return (dev & _dev_mask[mbits]);
}

#undef	dev_make
EXPORT XDEV_T
dev_make(majo, mino)
	XDEV_T	majo;
	XDEV_T	mino;
{
	if (minorbits == 0)
		return ((XDEV_T)-1);
	return ((majo << minorbits) | mino);
}

#undef	_dev_make
EXPORT XDEV_T
_dev_make(mbits, majo, mino)
	int	mbits;
	XDEV_T	majo;
	XDEV_T	mino;
{
	if (mino == 0)
		return ((XDEV_T)-1);
	return ((majo << mbits) | mino);
}
#endif	/* IS_LIBRARY */
