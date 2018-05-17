/* @(#)ctype.h	1.8 13/05/01 Copyright 1986-2013 J. Schilling */
/*
 *	Copyright (c) 1986-2013 J. Schilling
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

#ifndef	_CTYPE_A_H	/* Do not confuse this with the system ctype.h,   */
#define	_CTYPE_A_H	/* so we use _CTYPE_A_H as we also use _ctype_a[] */

#define	_UPC	01	/* Upper case */
#define	_LOWC	02	/* Lower case */
#define	_NUM	04	/* Numeral (digit) */
#define	_WHT	010	/* White character */
#define	_PUN	020	/* Punctuation */
#define	_CTL	040	/* Control character */
#define	_OCT	0100	/* Octal digit */
#define	_HEX	0200	/* hexadecimal digit */

#define	_DIG	(_NUM|_HEX)
#define	_ODIG	(_NUM|_OCT|_HEX)
#define	_UCHEX	(_UPC|_HEX)
#define	_LCHEX	(_LOWC|_HEX)

#define	_TAB	(_WHT|_CTL)

/*#ifndef lint*/
extern unsigned char	_ctype_a[];

/*
 * On HP-UX, /usr/include/limits.h pulls in /usr/include/ctype.h
 * This happens before we are pulled in and causes redefine warnings.
 * So we #define REDEFINE_CTYPE in case we did not pull in
 * /usr/include/ctype.h by our own will by using e.g. schily/ctype.h
 */
#if	!defined(_SCHILY_CTYPE_H) && !defined(_SCHILY_WCTYPE_H)
#ifndef	REDEFINE_CTYPE
#define	REDEFINE_CTYPE
#endif
#endif

/*
 * Argument to the following macros must be an int,
 * that is -1 on EOF and non-negative on all other characters.
 */

#ifdef	REDEFINE_CTYPE
#undef	isalpha
#undef	isupper
#undef	islower
#undef	isdigit
#undef	isoctal
#undef	isxdigit
#undef	isalnum
#undef	iswhite
#undef	isspace
#undef	ispunct
#undef	isprint
#undef	iscntrl
#undef	isascii
#undef	_toupper
#undef	_tolower
#undef	toascii
#undef	tolower
#undef	toupper
#endif	/* REDEFINE_CTYPE */

#define	isalpha(c)	((_ctype_a + 1)[c] & (_UPC|_LOWC))
#define	isupper(c)	((_ctype_a + 1)[c] & _UPC)
#define	islower(c)	((_ctype_a + 1)[c] & _LOWC)
#define	isdigit(c)	((_ctype_a + 1)[c] & _NUM)
#define	isoctal(c)	((_ctype_a + 1)[c] & _OCT)
#define	isxdigit(c)	((_ctype_a + 1)[c] & _HEX)
#define	isalnum(c)	((_ctype_a + 1)[c] & (_UPC|_LOWC|_NUM))
#define	iswhite(c)	((_ctype_a + 1)[c] & _WHT)
#define	isspace(c)	((_ctype_a + 1)[c] & _WHT)
#define	ispunct(c)	((_ctype_a + 1)[c] & _PUN)
#define	isprint(c)	(!((_ctype_a + 1)[c] & _CTL))
#define	iscntrl(c)	((_ctype_a + 1)[c] & _CTL)
#define	isascii(c)	(!((c) & ~0177))
#define	_toupper(c)	((c) - 'a' + 'A')
#define	_tolower(c)	((c) - 'A' + 'a')
#define	toascii(c)	((c) & 0177)
#define	tolower(c)	(isupper(c) ? _tolower(c) : (c))
#define	toupper(c)	(islower(c) ? _toupper(c) : (c))

/*#endif*/

#endif	/* _CTYPE_A_H */
