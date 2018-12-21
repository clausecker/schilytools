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
 * @(#)bulk.c	1.15 18/12/16 Copyright 2011-2018 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)bulk.c	1.15 18/12/16 Copyright 2011-2018 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)bulk.c"
#pragma ident	"@(#)sccs:lib/comobj/bulk.c"
#endif
#include	<defines.h>
#include	<i18n.h>

#ifdef	BULK_DEBUG
static	void	Ndbg	__PR((Nparms *));
#endif

static	char	sdot[] = "s.";

void
initN(N)
	Nparms	*N;
{
	N->n_comma = 0;		/* admin: rename ifile to ,ifile */
	N->n_unlink = 0;	/* admin: remove ifile		 */
	N->n_get = 0;
	N->n_sdot = 1;		/* file name is s.file		 */
	N->n_subd = 0;		/* s.file is in sub-directory	 */
	N->n_prefix = NULL;
	N->n_parm = "";		/* Parameter from -Nbulk option	 */
	N->n_ifile = NULL;
	N->n_dir_name = NULL;
	N->n_sid.s_rel = N->n_sid.s_lev = N->n_sid.s_br = N->n_sid.s_seq = 0;
	N->n_mtime.tv_sec = 0;
	N->n_mtime.tv_nsec = 0;
}

void
freeN(N)
	Nparms	*N;
{
	if (N->n_prefix == NULL)
		return;

	if (N->n_prefix == sdot) {
		;
	} else if (N->n_prefix >= N->n_parm &&
		N->n_prefix <= &N->n_parm[strlen(N->n_prefix)]) {
		;
	} else {
		free(N->n_prefix);
	}
	N->n_prefix = NULL;
}

void
parseN(N)
	Nparms	*N;
{
	char	*parm = N->n_parm;

	while (*parm && any(*parm, "+-, ")) {
		if (*parm == ' ') {		/* Dummy flag	   */
			parm++;
		}
		if (*parm == '-') {
			N->n_unlink = 1;	/* Unlink ifile	   */
			parm++;
		}
		if (*parm == '+') {
			if (N->n_get > 0)	/* -N++ used by get  */
				N->n_get = 2;	/* for +sid+filename */
			else
				N->n_get = 1;	/* admin: get ifile  */
			parm++;
		}
		if (*parm == ',') {		/* admin only	   */
			N->n_comma = 1;		/* Rename to ,file */
			parm++;
		}
	}
	N->n_sdot = sccsfile(parm);
	if (N->n_sdot) {			/* parm ends in s. */
		if (strlen(sname(parm)) > 2)
			fatal(gettext("bad N argument (co37)"));
		N->n_prefix = parm;		/* -Ns. / -NSCCS/s. */
		N->n_subd = parm != sname(parm);
	} else if (*parm == '\0') {		/* Empty parm	*/
		N->n_prefix = sdot;		/* "s."		*/
		N->n_subd = 0;
	} else {				/* -NSCCS	*/
		size_t	len = strlen(parm);
		int	slseen = 1;

		if (parm[len-1] != '/') {
			slseen = 0;
			len++;
		}
		len += 3;
		N->n_prefix = xmalloc(len);
		cat(N->n_prefix, parm, slseen?"":"/", sdot, (char *)0);
		N->n_subd = 1;
	}
#ifdef	BULK_DEBUG
	Ndbg(N);
#endif
}

/*
 * Compute path names for the bulk enter mode that is selected by -N
 *
 * 	-Ns.		Arguments are aaa/s.xxx files with matching aaa/xxx files
 * 	-N		Arguments are aaa/xxx files, aaa/s.xxx is created
 * 	-NSCCS/s.	Arguments are aaa/SCCS/s.xxx with matching aaa/xxx files
 * 	-NSCCS		Arguments are aaa/xxx Files, aaa/SCCS/s.xxx is created
 *
 * Returns:
 *
 *	afile		the probably new pointer to the s. file
 *	NULL		afile/ifile is a directory, N->n_ifile != NULL -> ifile
 */
char *
bulkprepare(N, afile)
	Nparms	*N;
	char	*afile;
{
static int	dfd = -1;
static char	Dir[FILESIZE];		/* The directory base of the g-file */
static char	Nhold[FILESIZE];	/* The space to hold the s. path    */
	char	tmp[FILESIZE];		/* For resolvepath() from libschily */

#ifdef	HAVE_FCHDIR
	if (sethomestat & SETHOME_OFFTREE) {	/* Would need openat() */
		;
	} else if (dfd < 0) {
		dfd = open(".", O_SEARCH);	/* on failure use full path */
	} else {
		if (fchdir(dfd) < 0)
			xmsg(".", NOGETTEXT("bulkprepare"));
	}
#endif
	if (N->n_get == 2 && *afile == '+') {
		afile = sid_ab(++afile, &N->n_sid);
		if (*afile++ != '+')
			fatal(gettext("not a SID-tagged filename (co38)"));
	} else {
		N->n_sid.s_rel =
		N->n_sid.s_lev =
		N->n_sid.s_br =
		N->n_sid.s_seq = 0;
	}

	Dir[0] = '\0';
	N->n_ifile = NULL;
	N->n_mtime.tv_sec = 0;
	if (!N->n_sdot) {			/* afile is ifile name */
		char	*sn = sname(afile);

		if (exists(afile)) {		/* must exist */
			if ((Statbuf.st_mode & S_IFMT) == S_IFDIR) {
				return ((char *)NULL);
			} else {
				N->n_mtime.tv_sec = Statbuf.st_mtime;
				N->n_mtime.tv_nsec = stat_mnsecs(&Statbuf);
			}
		} else {
			N->n_mtime.tv_sec = 0;
			N->n_mtime.tv_nsec = 0;
			if (N->n_flags & N_IDOT)
				xmsg(afile, NOGETTEXT("bulkprepare"));
		}
		if (sn == afile) {		/* No dir for short names */
			Dir[0] = '\0';
		} else if (*afile == '/' && &afile[1] == sn) {
			strlcpy(Dir, "/", sizeof (Dir));
		} else {			/* Get dir part from afile */
			size_t	len = sn - afile;
			if (len > sizeof (Dir))
				len = sizeof (Dir);
			strlcpy(Dir, afile, len); /* replace last '/' by '\0' */
			/*
			 * We need a path free of symlink components.
			 * resolvepath() is available as syscall on Solaris,
			 * or as user space implementation in libschily.
			 * The base directory needs to exist in case we are
			 * running something like "admin -N -i. file", then we
			 * may use * resolvepath() that requires the existence.
			 * resolvepath() from libschily does not support input
			 * and output buffer to be the same storage.
			 */
			strlcpy(tmp, Dir, sizeof (Dir));
			if ((len = ((N->n_flags & N_IDOT) ?
				    resolvepath(tmp, Dir, sizeof (Dir)):
				    resolvenpath(tmp, Dir, sizeof (Dir)))) == -1)
				efatal(gettext("path conversion error (co28)"));
			else if (len >= sizeof (Dir))
				fatal(gettext("resolved path too long (co29)"));
			else
				Dir[len] = '\0'; /* Solaris syscall needs it */
		}
		if (Dir[0] != '\0' && (dfd < 0 || chdir(Dir) < 0)) {
			Nhold[0] = '\0';
			if (sethomestat & SETHOME_OFFTREE) {
				strlcatl(Nhold, sizeof (Nhold),
					setrhome, "/.sccs/data/",
					cwdprefix?cwdprefix:"",
					cwdprefix?"/":"",
					Dir, *Dir?"/":"",
					N->n_prefix, sn, (char *)0);
			} else {
				strlcatl(Nhold, sizeof (Nhold),
					Dir, *Dir?"/":"",
					N->n_prefix, sn, (char *)0);
			}
			N->n_ifile = afile;
			N->n_dir_name = NULL;
		} else {					/* Did chdir  */
			Nhold[0] = '\0';
			if (sethomestat & SETHOME_OFFTREE) {
				strlcatl(Nhold, sizeof (Nhold),
					setrhome, "/.sccs/data/",
					cwdprefix?cwdprefix:"",
					cwdprefix?"/":"",
					N->n_prefix, sn, (char *)0);
			} else {				/* use short name */
				strlcatl(Nhold, sizeof (Nhold),
					N->n_prefix, sn, (char *)0);
			}
			N->n_ifile = sn;
			N->n_dir_name = Dir;
			if (N->n_dir_name[0] == '.' && N->n_dir_name[1] == '/')
				N->n_dir_name += 2;
		}
		afile = Nhold;			/* Use computed s.file name */
		if (afile[0] == '.' && afile[1] == '/')
			afile += 2;

	} else {				/* afile is s.file name */
		char	*np;
		size_t	plen = strlen(N->n_prefix);

		if (!sccsfile(afile))
			fatal(gettext("not an SCCS file (co1)"));

		np = sname(afile);		/* simple-name/base-name */
		np = np + 2 - plen;
		if (np < afile ||
		    ((np > afile) && (np[-1] != '/')) ||
		    strncmp(np, N->n_prefix, plen) != 0)
			fatal(gettext("not in specified sub directory (co36)"));

		if (np > afile) {
			np[-1] = '\0';
			if (dfd >= 0) {
				int	len;

				if (afile[0] == '\0')
					strlcpy(Dir, "/", sizeof (Dir));
				else
					strlcpy(Dir, afile, sizeof (Dir));

				/*
				 * The base directory needs to exist in case we
				 * are running something like
				 * "admin -Ns. -i. s.file" or any command that
				 * is not something like "admin -Ns. -n s.file",
				 * then we may use
				 * resolvepath() that requires the existence.
				 * resolvepath() from libschily does not support
				 * input and output buffer to be the same
				 * storage.
				 */
				strlcpy(tmp, Dir, sizeof (Dir));
				if ((len =
				    (((N->n_flags & (N_IFILE|N_NFILE)) == 0) ||
				    (N->n_flags & N_IDOT) ?
					    resolvepath(tmp, Dir, sizeof (Dir)):
					    resolvenpath(tmp, Dir, sizeof (Dir)))) == -1)
					efatal(gettext("path conversion error (co28)"));
				else if (len >= sizeof (Dir))
					fatal(gettext("resolved path too long (co29)"));
				else
					Dir[len] = '\0'; /* Solaris syscall needs it */

				if (chdir(Dir) < 0)
					goto nochdir;
				N->n_dir_name = Dir;
				if (N->n_dir_name[0] == '.' && N->n_dir_name[1] == '/')
					N->n_dir_name += 2;
				afile = np;
				strlcpy(Nhold, auxf(np, 'g'), sizeof (Nhold));
			} else {
			nochdir:
				N->n_dir_name = NULL;
				Nhold[0] = '\0';
				strlcatl(Nhold, sizeof (Nhold),
					afile, "/", auxf(np, 'g'), (char *)0);
			}
			np[-1] = '/';
		} else {
			N->n_dir_name = NULL;
			strlcpy(Nhold, auxf(np, 'g'), sizeof (Nhold));
		}
		N->n_ifile = Nhold;

		if (exists(N->n_ifile)) {
			if ((Statbuf.st_mode & S_IFMT) == S_IFDIR) {
				return ((char *)NULL);
			} else {
				N->n_mtime.tv_sec = Statbuf.st_mtime;
				N->n_mtime.tv_nsec = stat_mnsecs(&Statbuf);
			}
		} else {
			N->n_mtime.tv_sec = 0;
			N->n_mtime.tv_nsec = 0;
			if (N->n_flags & N_IDOT)
				xmsg(N->n_ifile, NOGETTEXT("bulkprepare"));
		}
	}
	if ((N->n_flags & (N_IFILE|N_IDOT|N_NFILE)) &&	/* Want to create new s.file */
	    (N->n_subd || strchr(afile, '/'))) {	/* Want to put s.file in subdir */
		char	dbuf[FILESIZE];

					/* Copy as dname() modifies arg */
		strlcpy(dbuf, afile, sizeof (dbuf));
		dname(dbuf);		/* Get directory name for s.file */
		if (!exists(dbuf)) {
			/*
			 * Make sure that the path to the subdir is present.
			 */
			if (mkdirs(dbuf, 0777) < 0)
				xmsg(dbuf, NOGETTEXT("mkdirs"));
		}
	}
	if (N->n_flags & N_GETI) {	/* get file, but dir may be missing */
		char	dbuf[FILESIZE];

					/* Copy as dname() modifies arg */
		strlcpy(dbuf, N->n_ifile, sizeof (dbuf));
		dname(dbuf);		/* Get directory name for g-file */
		if (!exists(dbuf)) {
			/*
			 * Make sure that the path to the subdir is present.
			 */
			if (mkdirs(dbuf, 0777) < 0)
				xmsg(dbuf, NOGETTEXT("mkdirs"));
		}
	}
#ifdef	BULK_DEBUG
	fprintf(stderr, "Dir '%s' %p\n", Dir, Dir);
	fprintf(stderr, "Nhold '%s' %p\n", Nhold, Nhold);
	fprintf(stderr, "afile '%s' %p\n", afile, afile);
	fprintf(stderr, "ifile '%s' %p\n",
				N->n_ifile?N->n_ifile:"NULL", N->n_ifile);
#endif
	return (afile);
}

#ifdef	BULK_DEBUG
static void
Ndbg(N)
	Nparms	*N;
{
	fprintf(stderr, "n_comma %d\n", N->n_comma);
	fprintf(stderr, "n_unlink %d\n", N->n_unlink);
	fprintf(stderr, "n_get %d\n", N->n_get);
	fprintf(stderr, "n_sdot %d\n", N->n_sdot);
	fprintf(stderr, "n_subd %d\n", N->n_subd);
	fprintf(stderr, "n_prefix '%s'\n", N->n_prefix?N->n_prefix:"NULL");
	fprintf(stderr, "n_parm '%s'\n", N->n_parm);
	fprintf(stderr, "n_ifile '%s'\n", N->n_ifile?N->n_ifile:"NULL");
	fprintf(stderr, "n_dir_name '%s'\n",
					N->n_dir_name?N->n_dir_name:"NULL");
}
#endif
