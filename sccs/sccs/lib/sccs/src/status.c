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
 * @(#)status.c	1.1 20/05/09 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)status.c 1.1 20/05/09 J. Schilling"
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
statuscmd(nfiles, argc, argv)
	int	nfiles;
	int	argc;
	char	**argv;
{
	int	rval;
	char	**np;
	char	**anames;
	int	anum;
	int	files = 0;
	int	i;

	optind = 1;
	opt_sp = 1;
	while ((rval = getopt(argc, argv, "")) != -1) {
		switch (rval) {

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
		if (files == 0) {
			checkhome(NULL);	/* No project set home: abort */
			sccs_readncache();	/* Read already known files */
		}
		files |= 1;
/*		rval |= initdir(*np, flags);*/
	}
	if (files == 0) {
/*		rval |= initdir(".", flags);*/
		checkhome(NULL);		/* No project set home: abort */
	}
	anames = sccs_getanames();
	anum = sccs_getanum();
	for (i = 0; i < anum; i++) {
		char	nbuf[FILESIZE];
		int	nlen;
		struct stat statb;

		if (stat(anames[i]+13, &statb) < 0)
			xmsg(*np, NOGETTEXT("status"));

		nlen = resolvepath(anames[i]+13, nbuf, sizeof (nbuf));
		if (nlen < 0) {
			efatal("path conversion error (cm12)");
		} else if (nlen >= sizeof (nbuf)) {
			fatal("resolved path too long (cm13)");
		} else {
			/*
			 * While the libschily implementation null terminates
			 * the names, this is not the case for the Solaris
			 * syscall resolvepath().
			 */
			nbuf[nlen] = '\0';
		}
	}
	return (rval);
}
