/* @(#)ctab.c	1.12 06/09/13 Copyright 1987-2004 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)ctab.c	1.12 06/09/13 Copyright 1987-2004 J. Schilling";
#endif
/*
 *	Character expansion table
 *
 *	Copyright (c) 1987-2004 J. Schilling
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
#include <stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/schily.h>

#define	STATIC_TABLE
#ifdef	STATIC_TABLE
unsigned char csize[256] = {
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2,

	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3,
};

/*unsigned*/ char *ctab[256] = {
	"^@",	"^A",	"^B",	"^C",	"^D",	"^E",	"^F",	"^G",
	"^H",	"^I",	"^J",	"^K",	"^L",	"^M",	"^N",	"^O",
	"^P",	"^Q",	"^R",	"^S",	"^T",	"^U",	"^V",	"^W",
	"^X",	"^Y",	"^Z",	"^[",	"^\\",	"^]",	"^^",	"^_",
	" ",	"!",	"\"",	"#",	"$",	"%",	"&",	"'",
	"(",	")",	"*",	"+",	",",	"-",	".",	"/",
	"0",	"1",	"2",	"3",	"4",	"5",	"6",	"7",
	"8",	"9",	":",	";",	"<",	"=",	">",	"?",
	"@",	"A",	"B",	"C",	"D",	"E",	"F",	"G",
	"H",	"I",	"J",	"K",	"L",	"M",	"N",	"O",
	"P",	"Q",	"R",	"S",	"T",	"U",	"V",	"W",
	"X",	"Y",	"Z",	"[",	"\\",	"]",	"^",	"_",
	"`",	"a",	"b",	"c",	"d",	"e",	"f",	"g",
	"h",	"i",	"j",	"k",	"l",	"m",	"n",	"o",
	"p",	"q",	"r",	"s",	"t",	"u",	"v",	"w",
	"x",	"y",	"z",	"{",	"|",	"}",	"~",	"^?",

	"~^@",	"~^A",	"~^B",	"~^C",	"~^D",	"~^E",	"~^F",	"~^G",
	"~^H",	"~^I",	"~^J",	"~^K",	"~^L",	"~^M",	"~^N",	"~^O",
	"~^P",	"~^Q",	"~^R",	"~^S",	"~^T",	"~^U",	"~^V",	"~^W",
	"~^X",	"~^Y",	"~^Z",	"~^[",	"~^\\",	"~^]",	"~^^",	"~^_",
	"~ ",	"~!",	"~\"",	"~#",	"~$",	"~%",	"~&",	"~'",
	"~(",	"~)",	"~*",	"~+",	"~,",	"~-",	"~.",	"~/",
	"~0",	"~1",	"~2",	"~3",	"~4",	"~5",	"~6",	"~7",
	"~8",	"~9",	"~:",	"~;",	"~<",	"~=",	"~>",	"~?",
	"~@",	"~A",	"~B",	"~C",	"~D",	"~E",	"~F",	"~G",
	"~H",	"~I",	"~J",	"~K",	"~L",	"~M",	"~N",	"~O",
	"~P",	"~Q",	"~R",	"~S",	"~T",	"~U",	"~V",	"~W",
	"~X",	"~Y",	"~Z",	"~[",	"~\\",	"~]",	"~^",	"~_",
	"~`",	"~a",	"~b",	"~c",	"~d",	"~e",	"~f",	"~g",
	"~h",	"~i",	"~j",	"~k",	"~l",	"~m",	"~n",	"~o",
	"~p",	"~q",	"~r",	"~s",	"~t",	"~u",	"~v",	"~w",
	"~x",	"~y",	"~z",	"~{",	"~|",	"~}",	"~~",	"~^?"
};
#else
unsigned char csize[256];
/*unsigned*/ char *ctab[256];
#endif


LOCAL	unsigned char	*makestr	__PR((unsigned char *s));
EXPORT	void		init_charset	__PR((void));
LOCAL	void		init_csize	__PR((void));
LOCAL	void		init_ctab	__PR((void));

LOCAL unsigned char *
makestr(s)
	register unsigned char *s;
{
		unsigned char *tmp;
	register unsigned char *s1;

	if ((tmp = (unsigned char *)malloc(strlen((char *)s)+1)) == NULL) {
		raisecond("makstr", 0L);
	}
	for (s1 = tmp; (*s1++ = *s++) != '\0'; );
	return (tmp);
}

extern	int	raw8;

EXPORT void
init_charset()
{
#ifdef	STATIC_TABLE
	if (getenv("CTAB") || raw8) {
		init_ctab();
		init_csize();
	}
#else
	init_ctab();
	init_csize();
#endif
}

#define	SP	0x20
#define	DEL	0x7F
#define	SP8	0xA0
#define	DEL8	0xFF

LOCAL void
init_csize()
{
	register unsigned c;
	register unsigned char *rcsize = csize;

	for (c = 0; c <= 255; c++, rcsize++) {
		if (c < SP || c == DEL)			  /*ctl*/ /*B0*/
			*rcsize = 2;
		else if ((c > DEL && c < SP8) || c == DEL8)  /*8bit ctl*/ /*B0*/
			*rcsize = 3;
		else if (c >= SP8 && !raw8)		  /*8bit norm*/ /*B0*/
			*rcsize = 2;
		else					  /*7bit norm*/ /*B0*/
			*rcsize = 1;
	}
}

LOCAL void
init_ctab()
{
	register unsigned c;
	register unsigned char *p;
	register unsigned char **rctab = (unsigned char **)ctab;
	register unsigned char *ctl = (unsigned char *) "^ ";
	register unsigned char *eight = (unsigned char *) "~ ";
	register unsigned char *eightctl = (unsigned char *) "~^ ";
	register unsigned char *ch = (unsigned char *) " ";

	for (c = 0; c <= 255; c++, rctab++) {
		if (c < SP || c == DEL) {			/* ctl char */
			p = makestr(ctl);
			p[1] = c ^ 0100;
		} else if ((c > DEL && c < SP8) || c == DEL8) {  /* 8 bit ctl */
			p = makestr(eightctl);
			p[2] = c ^ 0300;
		} else if (c >= SP8 && !raw8) {			/* 8 bit char */
			p = makestr(eight);
			p[1] = c & 0177;
		} else {					/* normal char */
			p = makestr(ch);
			p[0] = c;
		}
		*rctab = p;
	}
}
