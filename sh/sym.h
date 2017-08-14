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
#pragma ident	"@(#)sym.h	1.8	05/06/08 SMI"	/* SVr4.0 1.6	*/
#endif
/*
 * Copyright 2009-2017 J. Schilling
 * @(#)sym.h	1.11 17/08/01 2009-2017 J. Schilling
 */
/*
 *	UNIX shell
 */


/* symbols for parsing */
#define	NOTSYM	'!'		/* "!"		*/
#define	DOSYM	0405		/* "do"		*/
#define	FISYM	0420		/* "fi"		*/
#define	EFSYM	0422		/* "elif"	*/
#define	ELSYM	0421		/* "else"	*/
#define	INSYM	0412		/* "in"		*/
#define	BRSYM	0406		/* "{"		*/
#define	KTSYM	0450		/* "}"		*/
#define	THSYM	0444		/* "then"	*/
#define	ODSYM	0441		/* "done"	*/
#define	ESSYM	0442		/* "esac"	*/
#define	IFSYM	0436		/* "if"		*/
#define	FORSYM	0435		/* "for"	*/
#define	WHSYM	0433		/* "while"	*/
#define	UNSYM	0427		/* "until"	*/
#define	CASYM	0417		/* "case"	*/
#define	SELSYM 	0470		/* "select"	*/
#define	TIMSYM	0474		/* "time"	*/

#define	SYMREP	04000		/* symbols with doubled characters */
#define	ECSYM	(SYMREP|';')	/* ";;"		*/
#define	ANDFSYM	(SYMREP|'&')	/* "&&"		*/
#define	ORFSYM	(SYMREP|'|')	/* "||"		*/
#define	APPSYM	(SYMREP|'>')	/* ">>"		*/
#define	DOCSYM	(SYMREP|'<')	/* "<<"		*/
#define	EOFSYM	02000
#define	SYMFLG	0400		/* reserved symbols (see above) */

/* arg to `cmd' */
#define	NLFLG	1		/* treat NL as ';' */
#define	MTFLG	2		/* empty cmd does not cause a syntax error */
#define	SEMIFLG	4		/* semi-colon after NL is ok */
#define	DOIOFLG	8		/* Parse I/O first in item() */

/* for peekc */
#define	MARK	0x80000000

/* odd chars */
#define	DQUOTE	'"'
#define	SQUOTE	'`'
#define	LITERAL	'\''
#define	DOLLAR	'$'
#define	ESCAPE	'\\'
#define	BRACE	'{'
#define	COMCHAR '#'

/* wdset flags */
#define	KEYFLAG		1	/* var=value detected */
#define	IN_CASE		2	/* inside "case" - no keywords */
