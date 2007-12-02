/* @(#)nixread.c	1.13 06/09/26 Copyright 1986, 1996-2003 J. Schilling */
/*
 *	Non interruptable extended read
 *
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
_nixread(f, buf, count)
	int	f;
	void	*buf;
	int	count;
{
	register char *p = (char *)buf;
	register int ret;
	register int total = 0;
		int	oerrno = geterrno();

	while (count > 0) {
		while ((ret = read(f, p, count)) < 0) {
			if (geterrno() == EINTR) {
				/*
				 * Set back old 'errno' so we don't change the
				 * errno visible to the outside if we did
				 * not fail.
				 */
				seterrno(oerrno);
				continue;
			}
			return (ret);	/* Any other error */
		}
		if (ret == 0)		/* Something went wrong */
			break;

		total += ret;
		count -= ret;
		p += ret;
	}
	return (total);
}
