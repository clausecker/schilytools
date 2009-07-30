/* @(#)tgoto.c	1.10 09/07/11 Copyright 1986-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)tgoto.c	1.10 09/07/11 Copyright 1986-2009 J. Schilling";
#endif
/*
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

#include <schily/standard.h>
#include <schily/string.h>
#include <schily/termcap.h>

EXPORT	char *	tgoto	__PR((char *CM, int col, int line));

/*
 * Define exported variables.
 */
#if	defined(IS_MACOS_X)
/*
 * The MAC OS X linker does not grok "common" varaibles.
 * Make UP/BC a "data" variable.
 */
EXPORT	char	*UP = 0; /* Cursor up 1 line			*/
EXPORT	char	*BC = 0; /* Back Cursor movement (Cursor left)	*/
#else
EXPORT	char	*UP;	/* Cursor up 1 line			*/
EXPORT	char	*BC;	/* Back Cursor movement (Cursor left)	*/
#endif

#define	OBUF_SIZE	80

/*
 * Perform string preparation/conversion for cursor addressing.
 * The string cm contains a format string.
 */
EXPORT char *
tgoto(cm, col, line)
	char	*cm;
	int	col;
	int	line;
{
	static	char	outbuf[OBUF_SIZE];	/* Where the output goes to */
		char	xbuf[10];		/* for %. corrections	    */
	register char	*op = outbuf;
	register char	*p = cm;
	register int	c;
	register int	val = line;
		int	usecol = 0;

	if (p == 0) {
badfmt:
		/*
		 * Be compatible to 'vi' in case of bad format.
		 */
		return ("OOPS");
	}
	xbuf[0] = 0;
	while ((c = *p++) != '\0') {
		if ((op + 5) >= &outbuf[OBUF_SIZE])
			return ("OVERFLOW");

		if (c != '%') {
			*op++ = c;
			continue;
		}
		switch (c = *p++) {

		case '%':		/* %% -> %			*/
					/* This is from BSD		*/
			*op++ = c;
			continue;

		case 'd':		/* output as printf("%d"...	*/
					/* This is from BSD (use val)	*/
			if (val < 10)
				goto onedigit;
			if (val < 100)
				goto twodigits;
			/*FALLTHROUGH*/

		case '3':		/* output as printf("%03d"...	*/
					/* This is from BSD (use val)	*/
			if (val >= 1000) {
				*op++ = '0' + (val / 1000);
				val %= 1000;
			}
			*op++ = '0' + (val / 100);
			val %= 100;
			/*FALLTHROUGH*/

		case '2':		/* output as printf("%02d"...	*/
					/* This is from BSD (use val)	*/
		twodigits:
			*op++ = '0' + val / 10;
		onedigit:
			*op++ = '0' + val % 10;
		nextparam:
			usecol ^= 1;
		setval:
			val = usecol ? col : line;
			continue;

		case 'C': 		/* For c-100: print quotient of	*/
					/* value by 96, if nonzero,	*/
					/* then do like %+.		*/
					/* This is from GNU (use val)	*/
			if (val >= 96) {
				*op++ = val / 96;
				val %= 96;
			}
			/*FALLTHROUGH*/

		case '+':		/* %+x like %c but add x before	*/
					/* This is from BSD (use val)	*/
			val += *p++;
			/*FALLTHROUGH*/

		case '.':		/* output as printf("%c" but...	*/
					/* This is from BSD (use val)	*/
			if (usecol || UP)  {
				/*
				 * We assume that backspace works and we don't
				 * need to test for BC too.
				 *
				 * If you did not call stty tabs while termcap
				 * is used you will get other problems, so we
				 * exclude tab from the execptions.
				 */
				while (val == 0 || val == '\004' ||
					/* val == '\t' || */ val == '\n') {

					strcat(xbuf,
						usecol ? (BC?BC:"\b") : UP);
					val++;
				}
			}
			*op++ = val;
			goto nextparam;

		case '>':		/* %>xy if val > x add y	*/
					/* This is from BSD (chng state)*/

			if (val > *p++)
				val += *p++;
			else
				p++;
			continue;

		case 'B':		/* convert to BCD char coding	*/
					/* This is from BSD (chng state)*/

			val += 6 * (val / 10);
			continue;

		case 'D':		/* weird Delta Data conversion	*/
					/* This is from BSD (chng state)*/

			val -= 2 * (val % 16);
			continue;

		case 'i':		/* increment row/col by one	*/
					/* This is from BSD (chng state)*/
			col++;
			line++;
			val++;
			continue;

		case 'm':		/* xor both parameters by 0177	*/
					/* This is from GNU (chng state)*/
			col ^= 0177;
			line ^= 0177;
			goto setval;

		case 'n':		/* xor both parameters by 0140	*/
					/* This is from BSD (chng state)*/
			col ^= 0140;
			line ^= 0140;
			goto setval;

		case 'r':		/* reverse row/col		*/
					/* This is from BSD (chng state)*/
			usecol = 1;
			goto setval;

		default:
			goto badfmt;
		}
	}
	/*
	 * append to output if there is space...
	 */
	if ((op + strlen(xbuf)) >= &outbuf[OBUF_SIZE])
		return ("OVERFLOW");
	for (p = xbuf; *p; )
		*op++ = *p++;
	*op = '\0';
	return (outbuf);
}
