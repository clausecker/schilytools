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
 * @(#)add.c	1.1 20/05/09 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)add.c 1.1 20/05/09 J. Schilling"
#endif
#include <defines.h>
#include <version.h>
#include <i18n.h>
#include <schily/stat.h>
#include <schily/getopt.h>
#include <schily/sysexits.h>
#include <schily/schily.h>
#include "sccs.h"

/*
 * Code to support project enhancements for SCCS.
 * This is mainly the project set home directory, the directory ".sccs" in that
 * directory and the changeset file.
 */
static char	**anames;
static int	alen;
static int	anum;
static int	addfile		__PR((char *file));
static void	free_anames	__PR((void));
static int	addcmp		__PR((const void *file1, const void *file2));
static void	cvtpath		__PR((char *nm, char *nbuf, size_t nbsize,
					int cwd, int phome));

EXPORT char **
sccs_getanames()
{
	return (anames);
}

EXPORT int
sccs_getanum()
{
	return (anum);
}

/*
 * Add a single file name to the array of files that describes the project.
 */
static int
addfile(file)
	char	*file;
{
	if (alen <= anum) {
		alen += 128;
		anames = realloc(anames, alen * sizeof (char *));
		if (anames == NULL)
			efatal("out of space (ut9)");
	}
	if ((anames[anum++] = strdup(file)) == NULL)
		efatal("out of space (ut9)");
	return (0);
}

/*
 * Free old strings from anames[].
 */
static void
free_anames()
{
	int	i;

	for (i = 0; i < anum; i++) {
		free(anames[i]);
	}
	free(anames);
	anames = NULL;
	alen = 0;
}

/*
 * Read the current name cache file and keep it in an in core array.
 */
EXPORT int
sccs_readncache()
{
	char	nbuf[FILESIZE];
	char	lbuf[MAXLINE];
	FILE	*nfp;

	strlcpy(nbuf, setrhome, sizeof (nbuf));
	strlcat(nbuf, "/.sccs/ncache", sizeof (nbuf));
	nfp = fopen(nbuf, "rb");
	if (nfp == NULL) {
		return (-1);
	}

	/*
	 * Reset old entries in anames[].
	 * Then read in the file.
	 */
	free_anames();
	while (fgets(lbuf, sizeof (lbuf), nfp) != NULL) {
		size_t	llen = strlen(lbuf);

		if (llen > 0 && lbuf[llen-1] == '\n')
			lbuf[llen-1] = '\0';
		addfile(lbuf);
	}
	fclose(nfp);
	return (0);
}

/*
 * Add compare function for qsort().
 * Note that we do not compare the time_t field and thus use an offset of 15,
 * see below for the format used for this string.
 */
static int
addcmp(file1, file2)
	const void	*file1;
	const void	*file2;
{
	int	ret = strcmp(&(*(char **)file1)[15], &(*(char **)file2)[15]);

	if (ret == 0) {
		Ffile = &(*(char **)file1)[15];
		fatal(gettext("already tracked (sc4)"));
	}
	return (ret);
}

/*
 * Convert a path name that is relative to the current working directory into
 * a path name that is relative to the change set home directory and normalize
 * the resulting path name.
 */
static void
cvtpath(nm, nbuf, nbsize, cwd, phome)
	char	*nm;		/* The file name argument */
	char	*nbuf;		/* The file name output buffer */
	size_t	nbsize;		/* The size of the file name output buffer */
	int	cwd;		/* File descriptor to working dir */
	int	phome;		/* File descriptor to change set home dir */
{
	char	npbuf[FILESIZE];
	size_t	npboff;
	int	nlen;
	char	*name;

	name = nm;
	npboff = 0;
	if (cwdprefix[0] && name[0] != '/') {
		/*
		 * We are not in the change set home directory and
		 * "name" is not an absolute path name.
		 * Copy the cwd prefix before the path to get a path
		 * name that is relative to the the change set home.
		 */
		strlcpy(&npbuf[npboff], cwdprefix, sizeof (npbuf) - npboff);
		npboff += cwdprefixlen;
		strlcpy(&npbuf[npboff], "/", sizeof (npbuf) - npboff);
		npboff += 1;
		strlcpy(&npbuf[npboff], name, sizeof (npbuf) - npboff);
		name = npbuf;
	}
	/*
	 * Since all our path names are relative to the change set
	 * home directory, we need to fchdir() to that directory before
	 * we normalize the path name.
	 */
	if (*nm != '/' && fchdir(phome) < 0) {
		Ffile = setahome ? setahome : setrhome;
		efatal("cannot change directory (cm16)");
	}
	nlen = resolvepath(name, nbuf, nbsize);	/* Must exist */
	if (nlen < 0) {
		efatal("path conversion error (cm12)");
	} else if (nlen >= nbsize) {
		fatal("resolved path too long (cm13)");
	} else {
		/*
		 * While the libschily implementation null terminates
		 * the names, this is not the case for the Solaris
		 * syscall resolvepath().
		 */
		nbuf[nlen] = '\0';

		/*
		 * If the resulting name is an absolute path that starts
		 * with the absolute change set home directory string,
		 * try to make it relative by removing the absolute home
		 * path string.
		 *
		 * If the remaining path is not inside the change set
		 * home tree, abort.
		 */
		if (nbuf[0] == '/' && setahome)
			make_relative(nbuf);
		if (!in_tree(nbuf)) {
			Ffile = nbuf;
			fatal("not in tree (cm17)");
		}
	}
	/*
	 * Chdir() back to our previous working directory since all
	 * file arguments are relative to that directory.
	 */
	if (*nm != '/' && fchdir(cwd) < 0) {
		Ffile = ".";
		efatal("cannot change directory (cm16)");
	}
}

/*
 * Add file to the current file set for the current change set.
 * This command always needs to have file type argument(s).
 */
EXPORT int
addcmd(nfiles, argc, argv)
	int	nfiles;
	int	argc;
	char	**argv;
{
	char	nbuf[FILESIZE];
	size_t	nboff;
	int	rval;
	char	**np;
	int	files = 0;
	int	cwd = -1;	/* File descriptor to cwd */
	int	phome = -1;	/* File descriptor to project home */
	struct stat statb;

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
			/*
			 * In order to keep the first implementation simple,
			 * we assume that the current working directory is
			 * always inside the change set tree and that all
			 * file arguments are from the same change set.
			 *
			 * XXX If we like to enhance this, we first need to
			 * XXX find what we like to support.
			 */
			checkhome(NULL);	/* No project set home: abort */
			sccs_readncache();	/* Read already known files */

			cwd = opencwd();
			phome = openphome();
		}
		files |= 1;
		/*
		 * 12 digits work from year -1199 up to year 33658 but we may
		 * need to check for strange time stamps if time_t has more
		 * that 32 bits since such a strange time stamp could cause
		 * an overflow in the string below.
		 */
		strlcpy(nbuf, "A 000000000000 ", sizeof (nbuf));
		if (stat(*np, &statb) < 0)
			xmsg(*np, NOGETTEXT("add"));
#if SIZEOF_TIME_T > 4
		if (statb.st_mtime > 999999999999L ||
		    statb.st_mtime < -99999999999L)
			efatal("file not in supported time range (cm15)");
#endif
		sprintf(&nbuf[2], "%12ld ", (long)statb.st_mtime);
		nboff = 15;
		cvtpath(*np, &nbuf[nboff], sizeof (nbuf) - nboff, cwd, phome);
		addfile(nbuf);
	}
	closedirfd(cwd);
	closedirfd(phome);

	if (files == 0) {
		usrerr(gettext(" missing file arg (cm3)"));
	} else {
		FILE	*nfp;
		int	i;

		qsort((void *)anames, anum, sizeof (char *), addcmp);

		strlcpy(nbuf, setrhome, sizeof (nbuf));
		strlcat(nbuf, "/.sccs/ncache", sizeof (nbuf));
		nfp = xfcreat(nbuf, 0666);
		for (i = 0; i < anum; i++) {
			fputs(anames[i], nfp);
			fputs("\n", nfp);
		}
		fclose(nfp);
	}
	return (rval);
}
