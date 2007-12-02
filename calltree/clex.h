/* @(#)clex.h	1.15 04/03/11 Copyright 1985, 1999 J. Schilling */
/*
 *	C lexer definitions
 *
 *	Copyright (c) 1985, 1999 J. Schilling
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

/*
 * The known lexer token types
 */
#define	T_NONE		0	/* C-lexer initial state	*/
#define	T_EOF		1	/* EOF condifion found		*/
#define	T_ERROR		2	/* Found junk characters	*/
#define	T_HASH		3	/* Found a '#'			*/
#define	T_LCURLY	4	/* Found a '{'			*/
#define	T_RCURLY	5	/* Found a '}'			*/
#define	T_OPEN		6	/* Found a '('			*/
#define	T_CLOSE		7	/* Found a ')'			*/
#define	T_SEMI		8	/* Found a ';'			*/
#define	T_COMMA		9	/* Found a ','			*/
#define	T_OPER		10	/* Found an operator symbol	*/
#define	T_CHAR		11	/* Found character 'x'		*/
#define	T_STRING	12	/* Sound string "xxxxxx"	*/
#define	T_ALPHA		13	/* Found an alpha identifier	*/
#define	T_NUMBER	14	/* Found a number		*/
#define	T_COMMENT	15	/* Found a comment		*/
#define	T_KEYW		16	/* Found a C keyword		*/

#define	LEXBSIZE	2048	/* Max size of a token or string*/

extern	unsigned char	lexbuf[];	/* the token buffer	*/
extern	char		*lexfile;	/* the current filename	*/
extern	int		lexline;	/* the current line #	*/
extern	char		*lextnames[];	/* lex token types names*/

extern	void	initkeyw	__PR((void));
extern	BOOL	keyword		__PR((char *name));
extern	void	clexinit	__PR((void));
extern	int	clex		__PR((FILE *fp));
