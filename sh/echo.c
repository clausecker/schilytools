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

/*
 * Copyright 1996 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#if defined(sun)
#pragma ident	"@(#)echo.c	1.17	05/09/13 SMI"
#endif

#include "defs.h"

/*
 * Copyright 2008-2016 J. Schilling
 *
 * @(#)echo.c	1.15 16/05/19 2008-2016 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)echo.c	1.15 16/05/19 2008-2016 J. Schilling";
#endif

/*
 *	UNIX shell
 */

#define	exit(a)	flushb(); return (a)

	int	echo		__PR((int argc, unsigned char **argv));
unsigned char	*escape_char	__PR((unsigned char *cp, unsigned char *res,
							int echomode));

int
echo(argc, argv)
int argc;
unsigned char **argv;
{
	unsigned char	*cp;
	int	i;
	int	nflg = 0;
	int	len;
	wchar_t	wc;

#ifdef	_iBCS2		/* SCO compatibility support */
	struct namnod   *sysv3;
	int	do_sysv3 = 0;

	sysv3 = findnam((unsigned char *)"SYSV3");
	if (sysv3 && (sysv3->namflg & (N_EXPORT | N_ENVNAM)))
		do_sysv3 = 1;

	/* Do the -n parsing if sysv3 is set or if ucb_builtsin is set */
	if (ucb_builtins && !do_sysv3) {
#else
	if (ucb_builtins) {
#endif /* _iBCS2 */

		nflg = 0;
		if (argc > 1 && argv[1][0] == '-' &&
		    argv[1][1] == 'n' && argv[1][2] == '\0') {
			nflg++;
			argc--;
			argv++;
		}

		for (i = 1; i < argc; i++) {
			sigchk();

			for (cp = argv[i]; *cp; cp++) {
				prc_buff(*cp);
			}

			if (i < argc-1)
				prc_buff(' ');
		}

		if (nflg == 0)
			prc_buff('\n');
		exit(0);
	} else {
		if (--argc == 0) {
			prc_buff('\n');
			exit(0);
		}
#ifdef  _iBCS2
		if (do_sysv3) {
			if (argc > 1 && argv[1][0] == '-' &&
			    argv[1][1] == 'n' && argv[1][2] == '\0') {
				nflg++;
				/* Step past the -n */
				argc--;
				argv++;
			}
		}
#endif /* _iBCS2 */

		for (i = 1; i <= argc; i++) {
			sigchk();
			(void) mbtowc(NULL, NULL, 0);
			for (cp = argv[i]; *cp; cp++) {
				if ((len = mbtowc(&wc, (char *)cp,
						MB_LEN_MAX)) <= 0) {
					(void) mbtowc(NULL, NULL, 0);
					prc_buff(*cp);
					continue;
				}

				if (wc == '\\') {
					unsigned char	cc;

					cp = escape_char(cp, &cc, TRUE);
					if (cp == NULL) {
						exit(0);
					}
					prc_buff(cc);
					continue;
				} else {
					for (; len > 0; len--)
						prc_buff(*cp++);
					cp--;
					continue;
				}
			}
#ifdef	_iBCS2
			/* Don't do if don't want newlines & out of args */
			if (!(nflg && i == argc))
#endif /* _iBCS2 */
				prc_buff(i == argc? '\n': ' ');
		}
		exit(0);
	}
}

unsigned char *
escape_char(cp, res, echomode)
	unsigned char	*cp;
	unsigned char	*res;
	int		echomode;	/* echo mode vs. C mode */
{
	int		j;
	int		wd;
	unsigned char	c;

	switch (*++cp) {
#if	defined(DO_SYSPRINTF) || defined(DO_ECHO_A)
	case 'a':	c = ALERT; break;
#endif
	case 'b':	c = '\b'; break;
	case 'c':	if (echomode)
				return (NULL);
			goto norm;
	case 'f':	c = '\f'; break;
	case 'n':	c = '\n'; break;
	case 'r':	c = '\r'; break;
	case 't':	c = '\t'; break;
	case 'v':	c = '\v'; break;
	case '\\':	c = '\\'; break;

	case '0':
		j = wd = 0;
	oct:
		while ((*++cp >= '0' &&
		    *cp <= '7') && j++ < 3) {
			wd <<= 3;
			wd |= (*cp - '0');
		}
		c = wd;
		--cp;
		break;

	case '1': case '2': case '3': case '4':
	case '5': case '6': case '7':
		if (!echomode) {
			j = 1;
			wd = (*cp - '0');
			goto oct;
		}

	default:
	norm:
		c = *--cp;
	}
	*res = c;
	return (cp);
}
