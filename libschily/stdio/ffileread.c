/* @(#)ffileread.c	1.11 06/09/26 Copyright 1986, 1996-2003 J. Schilling */
/*
 *	Copyright (c) 1986, 1996-2003 J. Schilling
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
ffileread(f, buf, len)
	register FILE	*f;
	void	*buf;
	int	len;
{
	register int	fd;
	register int	ret;

	down2(f, _IOREAD, _IORW);
	fd = fileno(f);

	while ((ret = read(fd, buf, len)) < 0 && geterrno() == EINTR)
		/* LINTED */
		;
	return (ret);
}
