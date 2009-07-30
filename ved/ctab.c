/* @(#)ctab.c	1.14 09/07/09 Copyright 1986-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)ctab.c	1.14 09/07/09 Copyright 1986-2009 J. Schilling";
#endif
/*
 *	Character string and stringlength tables for screen
 *	output functions.
 *
 *	Copyright (c) 1986-2009 J. Schilling
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

#include "ved.h"

EXPORT	Uchar	csize[256];
EXPORT	Uchar	*ctab[256];

LOCAL	Uchar	*cmakestr	__PR((ewin_t *wp, Uchar* s));
EXPORT	void	init_charset	__PR((ewin_t *wp));
LOCAL	void	init_csize	__PR((ewin_t *wp));
LOCAL	void	init_ctab	__PR((ewin_t *wp));

LOCAL Uchar *
cmakestr(wp, s)
		ewin_t	*wp;
	register Uchar	*s;
{
		Uchar	*tmp;
	register Uchar	*s1;

	if ((tmp = (Uchar *) malloc(strlen(C s)+1)) == NULL) {
		rsttmodes(wp);
		raisecond("makstr", 0L);
	}
	for (s1 = tmp; (*s1++ = *s++) != '\0'; );
	return (tmp);
}

EXPORT void
init_charset(wp)
	ewin_t	*wp;
{
	init_ctab(wp);
	init_csize(wp);
}

LOCAL void
init_csize(wp)
	ewin_t	*wp;
{
	register unsigned c;
	register Uchar	*rcsize = csize;

	for (c = 0; c <= 255; c++, rcsize++) {
		if (c < SP || c == DEL)			/*ctl*/
			*rcsize = 2;
		else if ((c > DEL && c < SP8) || c == DEL8) /*8bit ctl*/
			*rcsize = 3;
		else if (c >= SP8 && !wp->raw8)		/*8bit norm*/
			*rcsize = 2;
		else					/*7bit norm*/
			*rcsize = 1;
	}
}

LOCAL char chpre[] = " ";
LOCAL char ctlpre[] = "^ ";
LOCAL char eightpre[] = "~ ";
LOCAL char eightctlpre[] = "~^ ";

LOCAL void
init_ctab(wp)
	ewin_t	*wp;
{
	register unsigned c;
	register Uchar	*p;
	register Uchar	**rctab	= ctab;
	register Uchar	*ch	= (Uchar *) chpre;
	register Uchar	*ctl	= (Uchar *) ctlpre;
	register Uchar	*eight	= (Uchar *) eightpre;
	register Uchar	*eightctl = (Uchar *) eightctlpre;

	for (c = 0; c <= 255; c++, rctab++) {
		if (c < SP || c == DEL) {		/* ctl char */
			p = cmakestr(wp, ctl);
			p[1] = c ^ 0100;
		} else if ((c > DEL && c < SP8) || c == DEL8) { /* 8 bit ctl */
			p = cmakestr(wp, eightctl);
			p[2] = c ^ 0300;
		} else if (c >= SP8 && !wp->raw8) {	/* 8 bit char */
			p = cmakestr(wp, eight);
			p[1] = c & 0177;
		} else {				/* normal char */
			p = cmakestr(wp, ch);
			p[0] = (Uchar)c;
		}
		*rctab = p;
	}
}
