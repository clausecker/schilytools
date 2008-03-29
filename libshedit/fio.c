/* @(#)fio.c	1.3 08/02/03 Copyright 2006-2008 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)fio.c	1.3 08/02/03 Copyright 2006-2008 J. Schilling";
#endif
/*
 *	Replacement for some libschily/stdio/ *.c to allow
 *	FILE * -> int *
 *
 *	Copyright (c) 2006-2008 J. Schilling
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

int
getc(f)
	FILE	*f;
{
	char	c;

	if (read(*f, &c, 1) != 1)
		return (EOF);
	return (c);
}

int
fgetc(f)
	FILE	*f;
{
	char	c;

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

int
filewrite(f, buf, n)
	FILE	*f;
	void	*buf;
	int	n;
{
	return (write(*f, buf, n));
}
