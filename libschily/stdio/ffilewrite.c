/* @(#)ffilewrite.c	1.6 04/08/08 Copyright 1986 J. Schilling */
/*
 *	Copyright (c) 1986 J. Schilling
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

#include "schilyio.h"

EXPORT int
ffilewrite(f, buf, len)
	register FILE	*f;
	void	*buf;
	int	len;
{
	down2(f, _IORWT, _IORW);

	return (write(fileno(f), (char *)buf, len));
}
