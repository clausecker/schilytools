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
 * This file contains modifications Copyright 2006-2009 J. Schilling
 *
 * @(#)help.c	1.4 09/01/04 J. Schilling
 */
#if defined(sun) || defined(__GNUC__)

#ident "@(#)help.c 1.4 09/01/04 J. Schilling"
#endif
/*
 * @(#)help.c 1.6 06/12/12
 */

#ident	"@(#)help.c"
#ident	"@(#)sccs:cmd/help.c"
#include	<defines.h>
#include	<i18n.h>

#ifdef	PROTOTYPES
static unsigned char Ohelpcmd[] = NOGETTEXT(INS_BASE "/ccs/lib/help/lib/help2");
#else
static unsigned char Ohelpcmd[] = NOGETTEXT("/usr/ccs/lib/help/lib/help2");
#endif

int main __PR((int argc, char **argv));

/*ARGSUSED*/
int
main(argc,argv)
int argc;
char *argv[];
{
	/*
	 * Set locale for all categories.
	 */
	setlocale(LC_ALL, NOGETTEXT(""));
	
	/* 
	 * Set directory to search for general l10n SCCS messages.
	 * Note this is not the same directory tree as that for the
	 * help text above.
	 */
#ifdef	PROTOTYPES
	(void) bindtextdomain(NOGETTEXT("SUNW_SPRO_SCCS"),
	   NOGETTEXT(INS_BASE "/ccs/lib/locale/"));
#else
	(void) bindtextdomain(NOGETTEXT("SUNW_SPRO_SCCS"),
	   NOGETTEXT("/usr/ccs/lib/locale/"));
#endif
	
	(void) textdomain(NOGETTEXT("SUNW_SPRO_SCCS"));

	execv((char *) Ohelpcmd, (char **) argv);
	fprintf(stderr, gettext("help: Could not exec: %s.  Errno=%d\n"), Ohelpcmd, errno);
	return (1);
}
