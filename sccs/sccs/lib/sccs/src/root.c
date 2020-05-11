/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may use this file only in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
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
 * Copyright 2015-2020 J. Schilling
 *
 * @(#)root.c	1.1 20/05/09 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)root.c 1.1 20/05/09 J. Schilling"
#endif
#include <defines.h>
#include <version.h>
#include <i18n.h>
#include <schily/stat.h>
#include <schily/getopt.h>
#include <schily/sysexits.h>
#include <schily/schily.h>
#include "sccs.h"


EXPORT int
rootcmd(nfiles, argc, argv)
	int	nfiles;
	int	argc;
	char	**argv;
{
	int	rval;
	char	**np;
	int	files = 0;
	int	verbose = 0;

	optind = 1;
	opt_sp = 1;
	while ((rval = getopt(argc, argv, "v")) != -1) {
		switch (rval) {

		case 'v':
			verbose++;
			break;
		default:
			usrerr("%s %s",
				gettext("unknown option"),
				argv[optind-1]);
			rval = EX_USAGE;
			exit(EX_USAGE);
			/*NOTREACHED*/
		}
	}

	rval = 0;
	for (np = &argv[optind]; *np != NULL; np++) {
		if (files) {
			usrerr(gettext("too many args"));
			rval = EX_USAGE;
			exit(EX_USAGE);
			/*NOTREACHED*/
		}
		files |= 1;
		unsethome();
		sethome(*np);
	}
	if (verbose) {
		/*
		 * If not yet done, set home directory from "."
		 */
		xsethome(NULL);	/* Only abort in case of error */
		sccs_sethdebug();
	} else {
		checkhome(NULL); /* No complete project set home: abort */
		printf("%s\n",
			setahome != NULL ? setahome :
			setrhome != NULL ? setrhome : "ERROR");
	}
	return (rval);
}
