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
 * @(#)init.c	1.7 20/06/25 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)init.c 1.7 20/06/25 J. Schilling"
#endif
#include <defines.h>
#include <version.h>
#include <schily/stat.h>
#include <schily/getopt.h>
#include <schily/sysexits.h>
#include <schily/schily.h>

EXPORT int	initcmd		__PR((int nfiles, int argc, char **argv));
LOCAL int	initdir		__PR((char *path, int intree));
EXPORT	void	sccs_sethdebug	__PR((void));

/*
 * Init directories for use with the project enhanced variant of SCCS.
 * This creates a directory named ".sccs" in the projects root directory.
 * The important content of that directory is the changeset history file.
 */
EXPORT int
initcmd(nfiles, argc, argv)
	int	nfiles;
	int	argc;
	char	**argv;
{
	int	rval;
	char	**np;
	int	flags = 0;
	int	files = 0;

	optind = 1;
	opt_sp = 1;
	while ((rval = getopt(argc, argv, "ifs")) != -1) {
		switch (rval) {

#define	IF_INTREE	1
		case 'i':
			flags |= IF_INTREE;
			break;

#define	IF_FORCE	2
		case 'f':
			flags |= IF_FORCE;
			break;

#define	IF_SINGLEFILE	4
		case 's':
			flags |= IF_SINGLEFILE;
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
		files |= 1;
		rval |= initdir(*np, flags);
	}
	if (files == 0) {
		rval |= initdir(".", flags);
	}
	xsethome(NULL);
	return (rval);
}

/*
 * Initialize a directory as root directory for a project enhanced SCCS.
 * This creates a directory named ".sccs" in the projects root directory.
 * The important content of that directory is the changeset history file.
 * The default is to have the SCCS history files for the files in the
 * project in $PROJECTROOT/.sccs/data but the files may be in the historic
 * location if the bit IF_INTREE is set in "flags".
 */
LOCAL int
initdir(hpath, flags)
	char	*hpath;
	int	flags;
{
	char	nbuf[FILESIZE];
	int	err = 0;

	resolvenpath(hpath, nbuf, sizeof (nbuf));	/* May not yet exist */
	xmkdir(nbuf, S_IRWXU|S_IRWXG|S_IRWXO);

	unsethome();					/* Forget old home */
	xsethome(hpath);

#ifdef	DEBUG
	if (sccs_debug()) {
		sccs_sethdebug();
	}
#endif

	if ((flags & IF_FORCE) == 0 && SETHOME_INIT()) {
		int	Oflags = Fflags;

		Fflags &= ~FTLACT;
		Ffile = setahome ? setahome : setrhome;
		fatal(gettext("already initialized (sc3)"));
		Ffile = NULL;
		Fflags = Oflags;
		return (1);
	}

	strlcat(nbuf, "/.sccs", sizeof (nbuf));
	xmkdir(nbuf, S_IRWXU|S_IRWXG|S_IRWXO);
	strlcat(nbuf, "/SCCS", sizeof (nbuf));
	xmkdir(nbuf, S_IRWXU|S_IRWXG|S_IRWXO);
	nbuf[strlen(nbuf) -4] = '\0';
	strlcat(nbuf, "dels", sizeof (nbuf));
	xmkdir(nbuf, S_IRWXU|S_IRWXG|S_IRWXO);
	strlcat(nbuf, "/SCCS", sizeof (nbuf));
	xmkdir(nbuf, S_IRWXU|S_IRWXG|S_IRWXO);
	if ((flags & IF_INTREE) == 0) {
		nbuf[strlen(nbuf) -9] = '\0';
		strlcat(nbuf, "data", sizeof (nbuf));
		xmkdir(nbuf, S_IRWXU|S_IRWXG|S_IRWXO);
	}
	if ((flags & IF_SINGLEFILE) == 0) {
		int	r;
		char	*av[6];
		extern int command __PR((char **argv, int forkflag, char *arg0));

		unsethome();				/* Forget old home */
		xsethome(hpath);			/* sets changesetfile */
		av[0] = "admin";
		av[1] = "-V6";
		av[2] = "-Xunlink";
		av[3] = "-n";
		av[4] = changesetfile;
		av[5] = NULL;
		r = command(av, TRUE, "");
		if (r)
			err = 1;
	}

	unsethome();
	return (err);
}

EXPORT void
sccs_sethdebug()
{
	printf(gettext("setahome:     '%s'\n"),
					setahome != NULL ? setahome : "NULL");
	printf(gettext("setphome:     '%s'\n"),
					setphome != NULL ? setphome : "NULL");
	printf(gettext("setrhome:     '%s'\n"),
					setrhome != NULL ? setrhome : "NULL");
	printf(gettext("cwdprefix:    '%s'\n"),
					cwdprefix != NULL ? cwdprefix : "NULL");
	printf(gettext("changeset:    '%s'\n"),
					changesetfile != NULL ? changesetfile : "NULL");
	printf(gettext("changesetg:   '%s'\n"),
					changesetgfile != NULL ? changesetgfile : "NULL");
	printf(gettext("homedist:     %d\n"),
					homedist);
	printf(gettext("setahomelen:  %d\n"),
					setahomelen);
	printf(gettext("setphomelen:  %d\n"),
					setphomelen);
	printf(gettext("setrhomelen:  %d\n"),
					setrhomelen);
	printf(gettext("cwdprefixlen: %d\n"),
					cwdprefixlen);
	printf(gettext("sethomestat:  0x%8.8X\n"),
					sethomestat);
	printf(gettext("sethome OK: %d INIT: %d OFFTREE: %d DELS %d CHSET %d NEWMODE %d\n"),
					(sethomestat & SETHOME_OK) != 0,
					SETHOME_INIT(),
					(sethomestat & SETHOME_OFFTREE) != 0,
					(sethomestat & SETHOME_DELS_OK) != 0,
					(sethomestat & SETHOME_CHSET_OK) != 0,
					(sethomestat & SETHOME_NEWMODE) != 0);
	if ((sethomestat & SETHOME_OK) && (sethomestat & SETHOME_DELS_OK) == 0)
		printf(gettext(
			"WARNING: Uninitialized .sccs directory found.\n"));
}
