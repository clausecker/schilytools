/* @(#)device.c	1.13 06/10/31 Copyright 1996-2006 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)device.c	1.13 06/10/31 Copyright 1996-2006 J. Schilling";
#endif
/*
 *	Handle local and remote device major/minor mappings
 *
 *	Copyright (c) 1996-2006 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/standard.h>
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
#if SIZEOF_UNSIGNED_LONG_INT > 4
	0x00000001FFFFFFFFLU,
	0x00000003FFFFFFFFLU,
	0x00000007FFFFFFFFLU,
	0x0000000FFFFFFFFFLU,
	0x0000001FFFFFFFFFLU,
	0x0000003FFFFFFFFFLU,
	0x0000007FFFFFFFFFLU,
	0x000000FFFFFFFFFFLU,
	0x000001FFFFFFFFFFLU,
	0x000003FFFFFFFFFFLU,
	0x000007FFFFFFFFFFLU,
	0x00000FFFFFFFFFFFLU,
	0x00001FFFFFFFFFFFLU,
	0x00003FFFFFFFFFFFLU,
	0x00007FFFFFFFFFFFLU,
	0x0000FFFFFFFFFFFFLU,
	0x0001FFFFFFFFFFFFLU,
	0x0003FFFFFFFFFFFFLU,
	0x0007FFFFFFFFFFFFLU,
	0x000FFFFFFFFFFFFFLU,
	0x001FFFFFFFFFFFFFLU,
	0x003FFFFFFFFFFFFFLU,
	0x007FFFFFFFFFFFFFLU,
	0x00FFFFFFFFFFFFFFLU,
	0x01FFFFFFFFFFFFFFLU,
	0x03FFFFFFFFFFFFFFLU,
	0x07FFFFFFFFFFFFFFLU,
	0x0FFFFFFFFFFFFFFFLU,
	0x1FFFFFFFFFFFFFFFLU,
	0x3FFFFFFFFFFFFFFFLU,
	0x7FFFFFFFFFFFFFFFLU,
	0xFFFFFFFFFFFFFFFFLU,
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
#if SIZEOF_UNSIGNED_LONG_INT > 4
	0x00000001FFFFFFFFL,
	0x00000003FFFFFFFFL,
	0x00000007FFFFFFFFL,
	0x0000000FFFFFFFFFL,
	0x0000001FFFFFFFFFL,
	0x0000003FFFFFFFFFL,
	0x0000007FFFFFFFFFL,
	0x000000FFFFFFFFFFL,
	0x000001FFFFFFFFFFL,
	0x000003FFFFFFFFFFL,
	0x000007FFFFFFFFFFL,
	0x00000FFFFFFFFFFFL,
	0x00001FFFFFFFFFFFL,
	0x00003FFFFFFFFFFFL,
	0x00007FFFFFFFFFFFL,
	0x0000FFFFFFFFFFFFL,
	0x0001FFFFFFFFFFFFL,
	0x0003FFFFFFFFFFFFL,
	0x0007FFFFFFFFFFFFL,
	0x000FFFFFFFFFFFFFL,
	0x001FFFFFFFFFFFFFL,
	0x003FFFFFFFFFFFFFL,
	0x007FFFFFFFFFFFFFL,
	0x00FFFFFFFFFFFFFFL,
	0x01FFFFFFFFFFFFFFL,
	0x03FFFFFFFFFFFFFFL,
	0x07FFFFFFFFFFFFFFL,
	0x0FFFFFFFFFFFFFFFL,
	0x1FFFFFFFFFFFFFFFL,
	0x3FFFFFFFFFFFFFFFL,
	0x7FFFFFFFFFFFFFFFL,
	0xFFFFFFFFFFFFFFFFL,
#endif
};
#endif

EXPORT void
dev_init(debug)
	BOOL	debug;
{
	int	i;
	dev_t	x;

	for (i = 0, x = 1; minor(x) == x; i++, x <<= 1)
		/* LINTED */
		;

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
	return ((majo << minorbits) | mino);
}

#undef	_dev_make
EXPORT XDEV_T
_dev_make(mbits, majo, mino)
	int	mbits;
	XDEV_T	majo;
	XDEV_T	mino;
{
	return ((majo << mbits) | mino);
}
#endif	/* IS_LIBRARY */
