/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/


#if defined(sun)
#pragma ident	"@(#)ctype.h	1.8	05/06/08 SMI"	/* SVr4.0 1.10.1.1 */
#endif
/*
 * Copyright 2009-2019 J. Schilling
 *
 * @(#)ctype.h	1.10 19/09/28 2009-2019 J. Schilling
 */
/*
 *	UNIX shell
 */


/* table 1 */
#define	T_SUB	01
#define	T_MET	02
#define	T_SPC	04
#define	T_DIP	010
#define	T_EOF	020
#define	T_EOR	040
#define	T_QOT	0100
#define	T_ESC	0200

/* table 2 */
#define	T_BRC	01			/* '{'			*/
#define	T_DEF	02			/* defchars "}=-+?"	*/
#define	T_AST	04			/* '*' or '@'		*/
#define	T_DIG	010			/* Digits		*/
#define	T_SHN	040			/* Legal parameter characters */
#define	T_IDC	0100			/* Upper + Lower ID Characters */
#define	T_SET	0200			/* Parameter set '+'	*/

/* for single chars */
#define	_TAB	(T_SPC)			/* '\11' (^I)	*/
#define	_SPC	(T_SPC)			/* ' '		*/
#define	_UPC	(T_IDC)			/* Upper case	*/
#define	_LPC	(T_IDC)			/* Lower case	*/
#define	_DIG	(T_DIG)			/* Digits	*/
#define	_EOF	(T_EOF)			/* '\0'		*/
#define	_EOR	(T_EOR)			/* '\n'		*/
#define	_BAR	(T_DIP)			/* '|'		*/
#define	_HAT	(T_MET)			/* '^'		*/
#define	_BRA	(T_MET)			/* '('		*/
#define	_KET	(T_MET)			/* ')'		*/
#define	_AMP	(T_DIP)			/* '&'		*/
#define	_SEM	(T_DIP)			/* ';'		*/
#define	_LT	(T_DIP)			/* '<'		*/
#define	_GT	(T_DIP)			/* '>'		*/
#define	_LQU	(T_QOT|T_ESC)		/* '`'		*/
#define	_BSL	(T_ESC)			/* '\\'		*/
#define	_DQU	(T_QOT|T_ESC)		/* '"'		*/
#define	_DOL1	(T_SUB|T_ESC)		/* '$'		*/

#define	_CBR	T_BRC			/* '{'		*/
#define	_CKT	T_DEF			/* '}'		*/
#define	_AST	(T_AST)			/* '*'		*/
#define	_EQ	(T_DEF)			/* '='		*/
#define	_MIN	(T_DEF|T_SHN)		/* '-'		*/
#define	_PCS	(T_SHN)			/* '!'		*/
#ifdef	DO_SUBSTRING			/* ${parm%word}	*/
#define	_PCT	(T_DEF|T_SET)		/* '%'		*/
#define	_NUM	(T_DEF|T_SHN|T_SET)	/* '#'		*/
#else
#define	_PCT	(0)			/* '%'		*/
#define	_NUM	(T_SHN)			/* '#'		*/
#endif
#ifdef	DO_DOL_SLASH
#define	_SLA	(T_SHN)			/* '/'		*/
#else
#define	_SLA	(0)			/* '/'		*/
#endif
#define	_DOL2	(T_SHN)			/* '$'		*/
#define	_PLS	(T_DEF|T_SET)		/* '+'		*/
#define	_AT	(T_AST)			/* '@'		*/
#define	_QU	(T_DEF|T_SHN)		/* '?'		*/

/* abbreviations for tests */
#define	_IDCH	(T_IDC|T_DIG)		/* Alphanumeric	*/
#define	_META	(T_SPC|T_DIP|T_MET|T_EOR) /* " |&;<>^()\n" */

extern
#ifdef __STDC__
const
#endif
unsigned char	_ctype1[];

/* nb these args are not call by value !!!! */
#define	space(c)	((c < QUOTE) && _ctype1[c]&(T_SPC))
#define	white(c)	((c < QUOTE) && _ctype1[c]&(T_SPC|T_EOR))
#define	eofmeta(c)	((c < QUOTE) && _ctype1[c]&(_META|T_EOF))
#define	qotchar(c)	((c < QUOTE) && _ctype1[c]&(T_QOT))
#define	eolchar(c)	((c < QUOTE) && _ctype1[c]&(T_EOR|T_EOF))
#define	dipchar(c)	((c < QUOTE) && _ctype1[c]&(T_DIP))
#define	subchar(c)	((c < QUOTE) && _ctype1[c]&(T_SUB|T_QOT))
#define	escchar(c)	((c < QUOTE) && _ctype1[c]&(T_ESC))
#define	escmeta(c)	((c < QUOTE) && _ctype1[c]&(_META|T_ESC))

extern
#ifdef __STDC__
const
#endif
unsigned char   _ctype2[];

#define	digit(c)	((c < QUOTE) && _ctype2[c]&(T_DIG))
#define	dolchar(c)	((c < QUOTE) && \
				_ctype2[c]&(T_AST|T_BRC|T_DIG|T_IDC|T_SHN))
#define	defchar(c)	((c < QUOTE) && _ctype2[c]&(T_DEF))
#define	setchar(c)	((c < QUOTE) && _ctype2[c]&(T_SET))
#define	digchar(c)	((c < QUOTE) && _ctype2[c]&(T_AST|T_DIG))
#define	letter(c)	((c < QUOTE) && _ctype2[c]&(T_IDC))
#define	alphanum(c)	((c < QUOTE) && _ctype2[c]&(_IDCH))
#define	astchar(c)	((c < QUOTE) && _ctype2[c]&(T_AST))
