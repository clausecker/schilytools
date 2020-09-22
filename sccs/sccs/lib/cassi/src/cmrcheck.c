/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * Copyright 2006-2020 J. Schilling
 *
 * @(#)cmrcheck.c	1.10 20/09/06 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)cmrcheck.c 1.10 20/09/06 J. Schilling"
#endif
/*
 * @(#)cmrcheck.c 1.5 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)cmrcheck.c"
#pragma ident	"@(#)sccs:lib/cassi/cmrcheck.c"
#endif

/* EMACS_MODES: c tabstop=4 !fill */

/*
 *	cmrcheck -- Check list in p file to see if this cmr is valid.
 *
 *
 */

#include <defines.h>
#include <filehand.h>
#include <i18n.h>

/* Debugging options. */
#define MAXLENCMR 12
#ifdef TRACE
#define TR(W,X,Y,Z) fprintf (stdout, W, X, Y, Z)
#else
#define TR(W,X,Y,Z) /* W X Y Z */
#endif

#define CMRLIMIT 128		/* Length of cmr string. */

int
cmrcheck (cmr, appl)
char	*cmr,
		*appl;
{
	char		lcmr[CMRLIMIT],	/* Local copy of CMR list. */
				*p[2], /* Field to match in .FRED file. */
				*formatp;

	formatp = gettext("%s is not a valid CMR.\n");

	TR("Cmrcheck: cmr=(%s) appl=(%s)\n", cmr, appl, NULL);
	p[1] = EMPTY;
	(void) strlcpy (lcmr, cmr, sizeof (lcmr));
	while ((p[0] = strrchr (lcmr, ',')) != EMPTY) {
		p[0]++;		/* Skip the ','. */
		if (strlen (p[0]) != MAXLENCMR || sweep (SEQVERIFY, gf (appl), EMPTY,
		  '\n', WHITE, 40, p, EMPTY, (char**) NULL, (int (*) __PR((char *, int, char **))) NULL,
		  (int (*) __PR((char **, char **, int))) NULL) != FOUND) {
			fprintf (stdout, formatp, p[0]);
			TR("Cmrcheck: return=1\n", NULL, NULL, NULL);
			return (1);
			}
		p[0]--;						/* Go back to comma. */
		*p[0] = '\0';					/* Clobber comma to end string. */
		}
	TR("Cmrcheck: last entry\n", NULL, NULL, NULL);
	p[0] = lcmr;						/* Last entry on the list. */
	if (strlen (p[0]) != MAXLENCMR || sweep (SEQVERIFY, gf (appl), EMPTY, '\n',
	  WHITE, 40, p, EMPTY, (char**) NULL, (int (*) __PR((char *, int, char **))) NULL,
		(int (*) __PR((char **, char **, int))) NULL)
	  != FOUND) {
		fprintf (stdout, formatp, p[0]);
		TR("Cmrcheck: return=1\n", NULL, NULL, NULL);
		return (1);
		}
	TR("Cmrcheck: return=0\n", NULL, NULL, NULL);
	return (0);
}
