/* @(#)bcrypt.c	1.15 06/09/13 Copyright 1988-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)bcrypt.c	1.15 06/09/13 Copyright 1988-2004 J. Schilling";
#endif
/*
 *	Copyright (c) 1988-2004 J. Schilling
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
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include "fmt.h"
#include <schily/schily.h>
#include <schily/libport.h>

typedef	unsigned long	Ulong;

EXPORT	char	*getnenv	 __PR((const char *, int));
EXPORT	Ulong	my_gethostid	__PR((void));
EXPORT	BOOL	bsecurity	__PR((int));
EXPORT	Ulong	bcrypt		__PR((Ulong));
EXPORT	char	*bmap		__PR((Ulong));
EXPORT	Ulong	bunmap		__PR((const char *));



/*---------------------------------------------------------------------------
|
| Get n'th value from colon separated list in environment
|
+---------------------------------------------------------------------------*/

EXPORT char *
getnenv(name, idx)
	const char	*name;
	int		idx;
{
	static	char rbuf[10];
	char	*ep = getenv(name);
	char	*xp;
	int	i = 0;

	if (!ep)
		return (NULL);

	while (i++ < idx) {
		if ((xp = strchr(ep, ':')) != NULL)
			ep = &xp[1];
		else
			return (NULL);
	}

	strncpy(rbuf, ep, sizeof (rbuf));
	rbuf[sizeof (rbuf)-1] = '\0';

	if ((xp = strchr(rbuf, ':')) != NULL)
		*xp = 0;
	return (rbuf);
}

EXPORT Ulong
my_gethostid()
{
	Ulong	id;

	id = gethostid();
	return (id);
}

EXPORT BOOL
bsecurity(idx)
	int	idx;
{
	Ulong	id;
	char	*sp;

	id = my_gethostid();
	sp = getnenv("SFORMAT_SECURITY", idx);
	while (idx-- >= 0)
		id = bcrypt(id);
	if (!sp || id != bunmap(sp))
		return (FALSE);
	return (TRUE);
}


EXPORT Ulong
bcrypt(i)
	Ulong	i;
{
	register Ulong	k;
	register Ulong	erg;

	k = i + 19991;
	erg = 0;
	do {
		erg += 1 + k / 19;
		erg *= 1 + k % 19;
		k /= 11;
	} while (k != 0);
	return (erg);
}


/*---------------------------------------------------------------------------
|
| Convert unsigned long to string similar to l64a()
|
+---------------------------------------------------------------------------*/

EXPORT char *
bmap(i)
	register Ulong	i;
{
	register int	c;
	static	char	buf[8];
	register char	*bp;

	bp = &buf[7];
	*bp = '\0';
	do {
		c = i % 64;
		i /= 64;
		c += '.';
		if (c > '9')
			c += 7;
		if (c > 'Z')
			c += 6;
		*--bp = c;
	} while (i);
	return (bp);
}


/*---------------------------------------------------------------------------
|
| Convert string to unsigned long similar to a64l()
|
+---------------------------------------------------------------------------*/

EXPORT Ulong
bunmap(s)
	register const char	*s;
{
	register Ulong	l;
	register int	c;

	l = 0L;
	while (*s) {
		c = *s++;
		if (c > 'Z')
			c -= 6;
		if (c > '9')
			c -= 7;
		c -= '.';
		l *= 64;
		l += c;
	}
	return (l);
}
