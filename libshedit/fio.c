/* @(#)fio.c	1.8 17/01/22 Copyright 2006-2017 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)fio.c	1.8 17/01/22 Copyright 2006-2017 J. Schilling";
#endif
/*
 *	Replacement for some libschily/stdio/ *.c to allow
 *	FILE * -> int *
 *
 *	Copyright (c) 2006-2017 J. Schilling
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

#include "schilyio.h"

static int	getbufc	__PR((void));

static unsigned char	inbuf[64];
static unsigned char	*bufp;
static int		bamt;

static int
getbufc()
{
	if (--bamt < 0) {
		bamt = read(STDIN_FILENO, inbuf, sizeof (inbuf));
		if (bamt <= 0)
			return (EOF);
		bufp = inbuf;
		--bamt;
	}
	return (*bufp++);
}

int
getc(f)
	FILE	*f;
{
	unsigned char	c;

	if (*f == STDIN_FILENO)
		return (getbufc());
	if (read(*f, &c, 1) != 1)
		return (EOF);
	return (c);
}

int
fgetc(f)
	FILE	*f;
{
	unsigned char	c;

	if (*f == STDIN_FILENO)
		return (getbufc());
	if (read(*f, &c, 1) != 1)
		return (EOF);
	return (c);
}

int
putc(c, f)
	int	c;
	FILE	*f;
{
	unsigned char	ch;

	ch = c;
	c = write(*f, &ch, 1);
	if (c != 1)
		return (EOF);
	return (ch);
}

ssize_t
fileread(f, buf, n)
	FILE	*f;
	void	*buf;
	size_t	n;
{
	return (read(*f, buf, n));
}

ssize_t
filewrite(f, buf, n)
	FILE	*f;
	void	*buf;
	size_t	n;
{
	return (write(*f, buf, n));
}
