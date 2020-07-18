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
 * @(#)bulk.c	1.27 20/07/16 Copyright 2011-2020 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)bulk.c	1.27 20/07/16 Copyright 2011-2020 J. Schilling"
#endif

#if defined(sun)
#pragma ident	"@(#)bulk.c"
#pragma ident	"@(#)sccs:lib/comobj/bulk.c"
#endif
#include	<defines.h>
#include	<schily/libgen.h>	/* Needed for dirname() */
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
	N->n_didchdir = 0;	/* bulkprepare() no chdir() yet	 */
	N->n_prefix = NULL;
	N->n_parm = "";		/* Parameter from -Nbulk option	 */
	N->n_ifile = NULL;
	N->n_dir_name = NULL;
	N->n_dfd = -1;		/* Open file descriptor for '.'	 */
	N->n_sid.s_rel = N->n_sid.s_lev = N->n_sid.s_br = N->n_sid.s_seq = 0;
	N->n_mtime.tv_sec = 0;
	N->n_mtime.tv_nsec = 0;
	N->n_flags = N->n_pflags = 0;
	N->n_error = 0;
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

	/*
	 * First check the leading flag characters in the -N parameter.
	 */
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
	/*
	 * path not points to the path name component of the -N argument
	 */
	N->n_sdot = sccsfile(parm);		/* Check last pn component */
	if (N->n_sdot) {			/* parm ends in s.	   */
		if (strlen(sname(parm)) > 2) {
			N->n_error = BULK_BADARG;
			fatal(gettext(bulkerror(N)));
			return;			/* With FTLRET	*/
		}
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
		len += 3;			/* Add "s.\0"	*/
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
 *
 * Side effects:
 *
 *	May do a chdir() to the directory part of "afile".
 */
char *
bulkprepare(N, afile)
	Nparms	*N;
	char	*afile;
{
static char	Dir[FILESIZE];		/* The directory base of the g-file */
static char	Nhold[FILESIZE];	/* The space to hold the s. path    */
	char	tmp[FILESIZE];		/* For resolvepath() from libschily */

	N->n_error = 0;
#ifdef	HAVE_FCHDIR
	if ((N->n_pflags & NP_NOCHDIR) != 0) {		/* Need full path */
		;
	} else if (sethomestat & SETHOME_OFFTREE) {	/* Would need openat() */
		;
	} else if (N->n_dfd < 0) {
#ifdef	USE_CHDIR_IN_BULK
		/*
		 * Disable chdir() for a time until everythings works
		 * correctly. Performance is less important than correct
		 * behavior.
		 */
		N->n_dfd = open(".", O_SEARCH);	/* on failure use full path */
#endif
	} else {
		bulkchdir(N);				/* chdir() back to . */
	}
#endif
	if (N->n_get == 2 && *afile == '+') {
		afile = sid_ab(++afile, &N->n_sid);
		if (*afile++ != '+') {
			N->n_error = BULK_NOSID;
			fatal(gettext(bulkerror(N)));
			return (NULL);		/* With FTLRET	*/
		}
	} else {
		N->n_sid.s_rel =
		N->n_sid.s_lev =
		N->n_sid.s_br =
		N->n_sid.s_seq = 0;
	}

	while (afile[0] == '.' && afile[1] == '/' && afile[2] != '0')
		afile += 2;

	Dir[0] = '\0';
	N->n_ifile = NULL;
	N->n_mtime.tv_sec = 0;
	if (!N->n_sdot) {			/* afile is ifile name */
		BOOL	is_dir = FALSE;
		char	*sn = sname(afile);
		char	*pfx;
		size_t	len;

		if (exists(afile)) {		/* must exist */
			N->n_mtime.tv_sec = Statbuf.st_mtime;
			N->n_mtime.tv_nsec = stat_mnsecs(&Statbuf);

			if ((Statbuf.st_mode & S_IFMT) == S_IFDIR) {
				is_dir = TRUE;
				if ((N->n_pflags & NP_DIR) == 0) {
					N->n_error = BULK_EISDIR;
					return ((char *)NULL);
				}
			} else {
				if ((N->n_pflags & NP_DIR) != 0) {
					N->n_error = BULK_ENOTDIR;
					return ((char *)NULL);
				}
			}
		} else {
			N->n_mtime.tv_sec = 0;
			N->n_mtime.tv_nsec = 0;
			if (N->n_flags & N_IDOT) {
				N->n_error = BULK_ENOIENT;
				xmsg(afile, NOGETTEXT("bulkprepare"));
				return ((char *)NULL);
			}
		}
		if (!is_dir && sn == afile) {	/* No dir for short names */
			Dir[0] = '\0';
		} else if (!is_dir && *afile == '/' && &afile[1] == sn) {
			strlcpy(Dir, "/", sizeof (Dir));
		} else {			/* Get dir part from afile */
			len = sn - afile;
			if (is_dir)
				len = strlen(afile)+1;
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
				    resolvenpath(tmp, Dir, sizeof (Dir)))) == -1) {
				N->n_error = BULK_EPATHCONV;
				efatal(gettext(bulkerror(N)));
				return ((char *)NULL);		/* With FTLRET	*/
			} else if (len >= sizeof (Dir)) {
				N->n_error = BULK_ETOOLONG;
				fatal(gettext(bulkerror(N)));
				return ((char *)NULL);		/* With FTLRET	*/
			} else {
				Dir[len] = '\0'; /* Solaris syscall needs it */
			}
		}
		pfx = N->n_prefix;
		N->n_ifile = sn;

		if ((N->n_pflags & NP_DIR) != 0) {
			strlcpy(tmp, pfx, sizeof (tmp));
			pfx = dirname(tmp);
			sn = NULL;
		}
		if (Dir[0] != '\0' && (N->n_dfd < 0 || chdir(Dir) < 0)) {
			N->n_didchdir = 0;
			Nhold[0] = '\0';
			if (sethomestat & SETHOME_OFFTREE) {
				strlcatl(Nhold, sizeof (Nhold),
					setrhome, "/.sccs/data/",
					cwdprefixlen?cwdprefix:"",
					cwdprefixlen?"/":"",
					Dir, *Dir?"/":"",
					pfx, sn, (char *)0);
			} else {
				strlcatl(Nhold, sizeof (Nhold),
					Dir, *Dir?"/":"",
					pfx, sn, (char *)0);
			}
			N->n_ifile = afile;
			N->n_dir_name = NULL;
		} else {					/* Did chdir  */
			N->n_didchdir = 1;
			Nhold[0] = '\0';
			if (sethomestat & SETHOME_OFFTREE) {
				strlcatl(Nhold, sizeof (Nhold),
					setrhome, "/.sccs/data/",
					cwdprefixlen?cwdprefix:"",
					cwdprefixlen?"/":"",
					pfx, sn, (char *)0);
			} else {				/* use short name */
				strlcatl(Nhold, sizeof (Nhold),
					pfx, sn, (char *)0);
			}
			N->n_dir_name = Dir;
			if (N->n_dir_name[0] == '.' && N->n_dir_name[1] == '/')
				N->n_dir_name += 2;
		}

		/*
		 * We may have created a path like:
		 *	../.sccs/data/non-exist/../SCCS/s.foo
		 * so we call resolvenpath() to convert it to:
		 *	../.sccs/data/SCCS/s.foo
		 */
		strlcpy(tmp, Nhold, sizeof(tmp));
		if ((len = resolvenpath(tmp, Nhold, sizeof (Nhold))) == -1) {
			N->n_error = BULK_EPATHCONV;
			efatal(gettext(bulkerror(N)));
			return ((char *)NULL);		/* With FTLRET	*/
		} else if (len >= sizeof (Nhold)) {
			N->n_error = BULK_ETOOLONG;
			fatal(gettext(bulkerror(N)));
			return ((char *)NULL);		/* With FTLRET	*/
		} else {
			Nhold[len] = '\0';	/* Solaris syscall needs it */
		}
		afile = Nhold;			/* Use computed s.file name */
		if (afile[0] == '.' && afile[1] == '/')
			afile += 2;

	} else {				/* afile is s.file name */
		char	*np;
		size_t	plen = strlen(N->n_prefix);

		if (!sccsfile(afile)) {
			N->n_error = BULK_NOTSCCS;
			fatal(gettext(bulkerror(N)));
			return ((char *)NULL);	/* With FTLRET	*/
		}

		np = sname(afile);		/* simple-name/base-name */
		np = np + 2 - plen;
		if (np < afile ||
		    ((np > afile) && (np[-1] != '/')) ||
		    strncmp(np, N->n_prefix, plen) != 0) {
			N->n_error = BULK_NOTINSUBDIR;
			fatal(gettext(bulkerror(N)));
			return ((char *)NULL);	/* With FTLRET	*/
		}

		if (np > afile) {
			np[-1] = '\0';
			if (N->n_dfd >= 0) {
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
					    resolvenpath(tmp, Dir, sizeof (Dir)))) == -1) {
					N->n_error = BULK_EPATHCONV;
					efatal(gettext(bulkerror(N)));
					return ((char *)NULL);	/* With FTLRET	*/
				} else if (len >= sizeof (Dir)) {
					N->n_error = BULK_ETOOLONG;
					fatal(gettext(bulkerror(N)));
					return ((char *)NULL);	/* With FTLRET	*/
				} else {
					Dir[len] = '\0'; /* Solaris syscall needs it */
				}

				if (chdir(Dir) < 0)
					goto nochdir;
				N->n_didchdir = 1;
				N->n_dir_name = Dir;
				if (N->n_dir_name[0] == '.' && N->n_dir_name[1] == '/')
					N->n_dir_name += 2;
				afile = np;
				strlcpy(Nhold, auxf(np, 'g'), sizeof (Nhold));
			} else {
			nochdir:
				N->n_didchdir = 0;
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
			N->n_mtime.tv_sec = Statbuf.st_mtime;
			N->n_mtime.tv_nsec = stat_mnsecs(&Statbuf);

			if ((Statbuf.st_mode & S_IFMT) == S_IFDIR) {
				if ((N->n_pflags & NP_DIR) == 0) {
					N->n_error = BULK_EISDIR;
					return ((char *)NULL);
				}
			} else {
				if ((N->n_pflags & NP_DIR) != 0) {
					N->n_error = BULK_ENOTDIR;
					return ((char *)NULL);
				}
			}
		} else {
			N->n_mtime.tv_sec = 0;
			N->n_mtime.tv_nsec = 0;
			if (N->n_flags & N_IDOT) {
				N->n_error = BULK_ENOIENT;
				xmsg(N->n_ifile, NOGETTEXT("bulkprepare"));
				return ((char *)NULL);
			}
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
			if (mkdirs(dbuf, 0777) < 0) {
				N->n_error = BULK_EMKDIR;
				xmsg(dbuf, NOGETTEXT("mkdirs"));
				return ((char *)NULL);
			}
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
			if (mkdirs(dbuf, 0777) < 0) {
				N->n_error = BULK_EMKDIR;
				xmsg(dbuf, NOGETTEXT("mkdirs"));
				return ((char *)NULL);
			}
		}
	}
#ifdef	BULK_DEBUG
	fprintf(stderr, "Dir '%s' %p\n", Dir, Dir);
	fprintf(stderr, "Nhold '%s' %p\n", Nhold, Nhold);
	fprintf(stderr, "afile '%s' %p\n", afile, afile);
	fprintf(stderr, "ifile '%s' %p\n",
				N->n_ifile?N->n_ifile:"NULL", N->n_ifile);
#endif
	return (afile);		/* Always != NULL */
}

/*
 * chdir() back to the directory where we started bulkprepare()
 */
int
bulkchdir(N)
	Nparms	*N;
{
#ifdef	HAVE_FCHDIR
	if (N->n_dfd >= 0 && N->n_didchdir) {
		if (fchdir(N->n_dfd) < 0) {
			xmsg(".", NOGETTEXT("bulkchdir"));
			N->n_error = BULK_ECHDIR;
			return (-1);
		}
		N->n_didchdir = 0;
	}
#endif
	return (0);
}

/*
 * close() directpry in Nparms
 */
int
bulkclosedir(N)
	Nparms	*N;
{
#ifdef	HAVE_FCHDIR
	if (N->n_dfd >= 0) {
		if (close(N->n_dfd) < 0) {
			xmsg(".", NOGETTEXT("bulkclosedir"));
			N->n_error = BULK_ECLOSE;
			return (-1);
		}
		N->n_dfd = -1;
	}
#endif
	return (0);
}

char *
bulkerror(N)
	Nparms	*N;
{
	switch (N->n_error) {

	case BULK_OK:	return ("No error");

	case BULK_BADARG:
			return ("bad N argument (co37)");

	case BULK_NOSID:
			return ("not a SID-tagged filename (co38)");

	case BULK_EPATHCONV:
			return ("path conversion error (co28)");

	case BULK_ETOOLONG:
			return ("resolved path too long (co29)");

	case BULK_NOTSCCS:
			return ("not an SCCS file (co1)");

	case BULK_NOTINSUBDIR:
			return ("not in specified sub directory (co36)");

	case BULK_EISDIR:
			return ("directory specified as s-file (cm14)");

	case BULK_ENOTDIR:
			return ("non directory specified as argument (cm20)");

	case BULK_ENOIENT:
			return ("input file is missing (cm21)");

	case BULK_EMKDIR:
			return ("could not make directory (cm22)");

	default:	return ("Unknown error");
	}
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
