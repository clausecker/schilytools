/* @(#)tputs.c	1.8 09/07/05 Copyright 1986-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)tputs.c	1.8 09/07/05 Copyright 1986-2009 J. Schilling";
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

#include <schily/mconfig.h>
#include <schily/standard.h>
#include <schily/termcap.h>
#include <ctype.h>
#include <schily/utypes.h>

EXPORT	int	tputs	__PR((char *sp, int affcnt, int (*outc)(int c)));

/*
 * Define exported variables.
 */
#if	defined(IS_MACOS_X)
/*
 * The MAC OS X linker does not grok "common" varaibles.
 * Make ospeed/PC a "data" variable.
 */
EXPORT	short	ospeed = 0; /* Line speed from sgtty to compute padding	*/
EXPORT	char	PC = 0;	/* Pad Character				*/
#else
EXPORT	short	ospeed;	/* Line speed from sgtty to compute padding	*/
EXPORT	char	PC;	/* Pad Character				*/
#endif

/*
 * Table to convert the speed returned by cfgetospeed() into tens of
 * milliseconds (duration) per char.
 * B50 is 1, B75 is 2, B110 is 3 ...
 * One character at 110 baud is 90.9 ms.
 */
LOCAL short
tmspch[] = {
	0, 2000, 1333, 909, 743, 667, 500, 333, 167, 83, 56, 42, 21, 10, 5,
	3,    2,    1,   1,   1,   0,   0,   0,   0,  0,  0,  0,  0,  0, 0,
};
#define	maxspeed	(sizeof (tmspch) / sizeof (tmspch[0]))

/*
 * Output the string pointed to by 'sp', insert padding if desired.
 * 'affcnt' is the number used to multiply with the delay.
 * 'outc' is a pointer to a function to really output the char.
 */
EXPORT int
tputs(sp, affcnt, outc)
		char	*sp;
		int	affcnt;
		int	(*outc) __PR((int c));
{
	register Uchar	*cp = (Uchar *)sp;
	register int	cdelay;		/* Delay value in # of chars */
	register int	delay = 0;	/* Delay value in tens of ms */

	if (cp == 0 || *cp == '\0')
		return (0);

	/*
	 * Convert the ms delay value (number before decimal point).
	 */
	while (isdigit(*cp)) {
		delay *= 10;
		delay += (*cp++ - '0');
	}
	delay *= 10;		/* Make it tens of milliseconds		*/

	if (*cp == '.') {	/* found decimal point			*/
		cp++;
		if (isdigit(*cp)) {
			delay += (*cp - '0');
		}
		/*
		 * Ignore the rest, only one digit past decimal point allowed.
		 */
		while (isdigit(*cp)) {
			cp++;
		}
	}
	if (*cp == '*') {	/* '*' means multiply by 'affcnt'.	*/
		cp++;
		delay *= affcnt;
	}
	while (*cp) {		/* Now outout the string		*/
		(*outc)(*cp++);
	}

	if (delay == 0)		/* No delay needed			*/
		return (0);
	/*
	 * Check if 'ospeed' is out of known range.
	 */
	if (ospeed <= 0 || ospeed >= maxspeed)
		return (0);

	/*
	 * XXX Hack: 'delay' by writing pad characters.
	 * XXX Writing pad chars may not work on all terminals.
	 * XXX If the terminal's interrupt routine is not designed for
	 * XXX delays, the effect may even be worse than without delay.
	 * XXX There should be a way to use real delays.
	 * XXX 'usleep()' is not usable because there might be characters
	 * XXX in the terminal's output queue.
	 */
	cdelay = tmspch[ospeed]; /* Get delay value for actual speed	*/
	delay += cdelay / 2;	/* Round up to the next half character	*/
	delay /= cdelay;	/* Compute # of characters for delay	*/

	while (--delay >= 0) {
		(*outc)(PC);
	}
	return (0);
}
