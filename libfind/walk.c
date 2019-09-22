/* @(#)walk.c	1.62 19/09/08 Copyright 2004-2019 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)walk.c	1.62 19/09/08 Copyright 2004-2019 J. Schilling";
#endif
/*
 *	Walk a directory tree
 *
 *	Copyright (c) 2004-2019 J. Schilling
 *
 *	In order to make treewalk() thread safe, we need to make it to not use
 *	chdir(2)/fchdir(2) which is process global.
 *
 *	chdir(newdir)	->	old = dfd;
 *				dfd = openat(old, newdir, O_RDONLY);
 *				close(old)
 *	fchdir(dd)	->	close(dfd); dfd = dd;
 *	stat(name)	->	fstatat(dfd, name, statb, 0)
 *	lstat(name)	-> 	fstatat(dfd, name, statb, AT_SYMLINK_NOFOLLOW)
 *	opendir(dname)	->	dd = openat(dfd, dname, O_RDONLY);
 *				dir = fdopendir(dd);
 *
 *	Similar changes need to be introduced in fetchdir().
 *
 *	For an optimized version, we need to have:
 *
 *	dirfd() or DIR->dd_fd	To fetch a fd from a DIR *
 *	and opendir()
 *
 *	fchdir()		To change towards and backdwards directories
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/stdio.h>
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/fcntl.h>	/* AT_FDCWD */
#ifndef	HAVE_FCHDIR
#include <schily/maxpath.h>
#endif
#include <schily/param.h>	/* NODEV */
#include <schily/stat.h>
#include <schily/errno.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/getcwd.h>
#include <schily/jmpdefs.h>
#include <schily/nlsdefs.h>
#include <schily/walk.h>
#include <schily/dirent.h>
#include <schily/fetchdir.h>
#include <schily/schily.h>

#if (defined(HAVE_DIRFD) || defined(HAVE_DIR_DD_FD)) && defined(HAVE_FCHDIR)
#define	USE_DIRFD		/* Use fchdir() to traverse the tree */
#endif

#ifndef	_AT_TRIGGER
#define	_AT_TRIGGER	0
#endif

#ifndef	ENAMETOOLONG
#define	ENAMETOOLONG	EINVAL
#endif

#if	defined(IS_MACOS_X) && defined(HAVE_CRT_EXTERNS_H)
/*
 * The MAC OS X linker does not grok "common" varaibles.
 * We need to fetch the address of "environ" using a hack.
 */
#include <crt_externs.h>
#define	environ	*_NSGetEnviron()
#else
extern	char **environ;
#endif

#ifndef	HAVE_LSTAT
#define	lstat	stat
#endif
#ifndef	HAVE_DECL_STAT
extern int stat	__PR((const char *, struct stat *));
#endif
#ifndef	HAVE_DECL_LSTAT
extern int lstat __PR((const char *, struct stat *));
#endif

#define	IS_UFS(p)	((p)[0] == 'u' && \
			(p)[1] == 'f' && \
			(p)[2] == 's' && \
			(p)[3] == '\0')
#define	IS_ZFS(p)	((p)[0] == 'z' && \
			(p)[1] == 'f' && \
			(p)[2] == 's' && \
			(p)[3] == '\0')

#define	DIR_INCR	1024		/* How to increment Curdir size */
#define	TW_MALLOC	0x01		/* Struct was allocated		*/
struct twvars {
	char		*Curdir;	/* The current path name	*/
	int		Curdtail;	/* Where to append to Curdir	*/
	int		Curdlen;	/* Current size of 'Curdir'	*/
	int		Flags;		/* Flags related to this struct	*/
	int		Snmlen;		/* Short nm len for walk()	*/
	struct WALK	*Walk;		/* Backpointer to struct WALK	*/
	struct stat	Sb;		/* stat(2) buffer for start dir	*/
	struct pdirs	*pdirs;		/* Previous dirs for walkcwd()	*/
#ifdef	HAVE_FCHDIR
	int		Home;		/* open fd to start CWD		*/
#else
	char		Home[MAXPATHNAME+1];	/* Abspath to start CWD	*/
#endif
};

struct pdirs {
	struct pdirs	*p_last; /* Previous node in list	*/
	DIR		*p_dir;	/* Open directory for this one	*/
	struct twvars	*p_varp; /* Pointer to "Curdir" path	*/
	dev_t		p_dev;	/* st_dev for this dir		*/
	ino_t		p_ino;	/* st_ino for this dir		*/
	BOOL		p_stat;	/* Need to call stat always	*/
	nlink_t		p_nlink; /* Number of subdirs to stat	*/
};


typedef	int	(*statfun)	__PR((const char *_nm, struct stat *_fs,
						struct pdirs *_pd));
typedef	DIR	*(*opendirfun)	__PR((const char *_nm, struct pdirs *_pd));

EXPORT	int	treewalk	__PR((char *nm, walkfun fn,
						struct WALK *_state));
LOCAL	int	walk		__PR((char *nm, statfun sf, walkfun fn,
						struct WALK *state,
						struct pdirs *last));
LOCAL	int	incr_dspace	__PR((FILE *f, struct twvars *varp, int amt));
EXPORT	void	walkinitstate	__PR((struct WALK *_state));
EXPORT	void	*walkopen	__PR((struct WALK *_state));
EXPORT	int	walkgethome	__PR((struct WALK *_state));
EXPORT	int	walkhome	__PR((struct WALK *_state));
EXPORT	int	walkclose	__PR((struct WALK *_state));
EXPORT	int	walknlen	__PR((struct WALK *_state));
LOCAL	int	xchdotdot	__PR((struct pdirs *last, int tail,
						struct WALK *_state));
LOCAL	int	xchdir		__PR((char *p));

LOCAL	int	wstat		__PR((const char *nm, struct stat *sp,
					struct pdirs *pd));
LOCAL	int	wlstat		__PR((const char *nm, struct stat *sp,
					struct pdirs *pd));

LOCAL	int	wdstat		__PR((const char *nm, struct stat *sp,
					struct pdirs *pd));
LOCAL	int	wdlstat		__PR((const char *nm, struct stat *sp,
					struct pdirs *pd));

#ifdef	USE_DIRFD
LOCAL	DIR	*nopendir	__PR((const char *name, struct pdirs *pd));
LOCAL	DIR	*dopendir	__PR((const char *name, struct pdirs *pd));
#endif

EXPORT int
treewalk(nm, fn, state)
	char		*nm;	/* The name to start walking		*/
	walkfun		fn;	/* The function to call for each node	*/
	struct WALK	*state; /* Walk state				*/
{
	struct twvars	vars;
	statfun		statf = wstat;
	int		nlen;

#ifndef	USE_DIRFD
	/*
	 * If we have no dirfd(), we currently cannot implement
	 * the walk mode without chdir(). This mainly excludes DOS.
	 */
	if ((state->walkflags & WALK_CHDIR) == 0) {
		seterrno(EINVAL);
		return (-1);
	}
#endif

	vars.Curdir  = NULL;
	vars.Curdlen = 0;
	vars.Curdtail = 0;
	vars.Flags = 0;
#ifdef	HAVE_FCHDIR
	vars.Home = -1;
#endif
	state->twprivate = &vars;
	vars.Walk = state;
	if (walkgethome(state) < 0) {
		state->twprivate = NULL;
		return (-1);
	}

	if (nm == NULL || nm[0] == '\0') {
		nm = ".";
	} else if (state->walkflags & WALK_STRIPLDOT) {
		while (nm[0] == '.' && nm[1] == '/') {
			for (nm++; nm[0] == '/'; nm++)
				/* LINTED */
				;
		}
		if (nm[0] == '\0')
			nm = ".";
	}

	vars.Curdir = __fjmalloc(state->std[2],
					DIR_INCR, "path buffer", JM_RETURN);
	if (vars.Curdir == NULL)
		return (-1);
	vars.Curdir[0] = 0;
	vars.Curdlen = DIR_INCR;
	/*
	 * If initial Curdir space is not sufficient, expand it.
	 */
	nlen = strlen(nm);
	vars.Snmlen = nlen;
	if ((vars.Curdlen - 2) < nlen)
		if (incr_dspace(state->std[2], &vars, nlen + 2) < 0)
			return (-1);

	while (lstat(nm, &vars.Sb) < 0 && geterrno() == EINTR)
		;

	state->flags = 0;
	state->base = 0;
	state->level = 0;

	if ((state->walkflags & WALK_CHDIR) == 0) {
		statf = wdstat;
		if (state->walkflags & WALK_PHYS)
			statf = wdlstat;

		if (state->walkflags & (WALK_ARGFOLLOW|WALK_ALLFOLLOW))
			statf = wdstat;
	} else {
		if (state->walkflags & WALK_PHYS)
			statf = wlstat;

		if (state->walkflags & (WALK_ARGFOLLOW|WALK_ALLFOLLOW))
			statf = wstat;
	}

	nlen = walk(nm, statf, fn, state, (struct pdirs *)0);
	walkhome(state);
	walkclose(state);

	free(vars.Curdir);
	state->twprivate = NULL;
	return (nlen);
}

LOCAL int
walk(nm, sf, fn, state, last)
	char		*nm;	/* The current name for the walk	*/
	statfun		sf;	/* stat() or lstat()			*/
	walkfun		fn;	/* The function to call for each node	*/
	struct WALK	*state;	/* For communication with (*fn)()	*/
	struct pdirs	*last;	/* This helps to avoid loops		*/
{
	struct pdirs	thisd;
	struct stat	fs;
	int		type;
	int		ret;
	int		otail;
	struct twvars	*varp = state->twprivate;

	varp->pdirs = last;
#ifdef	USE_DIRFD
	thisd.p_dir  = (DIR *)NULL;
#endif
	thisd.p_varp = varp;
	otail = varp->Curdtail;
	state->base = otail;
	if (varp->Curdtail == 0 || varp->Curdir[varp->Curdtail-1] == '/') {
		if (varp->Curdtail == 0 &&
		    (state->walkflags & WALK_STRIPLDOT) &&
		    (nm[0] == '.' && nm[1] == '\0')) {
			varp->Curdir[0] = '.';
			varp->Curdir[1] = '\0';
		} else {
			char	*p;

			p = strcatl(&varp->Curdir[varp->Curdtail], nm,
								(char *)0);
			varp->Curdtail = p - varp->Curdir;
			/*
			 * Let state->base point to the last component for
			 * command line arguments indicated by otail == 0.
			 */
			if (otail == 0) {
				char		*e = p;
				register char	*c = varp->Curdir;

				while (--p > c) {	/* Skip trailing '/' */
					if (*p != '/')
						break;
				}
				if (*p != '/') {	/* Not only '/'s */
					while (p >= c && *p != '/')
						p--;
					p++;		/* To first non '/' */
				}
				state->base = p - varp->Curdir;
				varp->Snmlen = e-p;
			}
		}
	} else {
		char	*p;

		p = strcatl(&varp->Curdir[varp->Curdtail], "/", nm, (char *)0);
		varp->Curdtail = p - varp->Curdir;
		state->base++;
	}

	if ((state->walkflags & WALK_NOSTAT) &&
	    (
#ifdef	HAVE_DIRENT_D_TYPE
	    ((state->flags & (WALK_WF_NOTDIR|WALK_WF_ISLNK)) ==
		WALK_WF_NOTDIR) ||
#endif
	    (last != NULL && !last->p_stat && last->p_nlink <= 0))) {
		/*
		 * Set important fields to useful values to make sure that
		 * no wrong information is evaluated in the no stat(2) case.
		 */
		fs.st_mode = 0;
		fs.st_ino = 0;
		fs.st_dev = NODEV;
		fs.st_nlink = 0;
		fs.st_size = 0;

		type = WALK_F;
		state->flags = 0;

		goto type_known;
	} else {
		while ((ret = (*sf)(nm, &fs, last)) < 0 && geterrno() == EINTR)
			;
	}
	state->flags = 0;
	if (ret >= 0) {
#ifdef	HAVE_ST_FSTYPE
		/*
		 * Check for autofs mount points...
		 */
		if (fs.st_fstype[0] == 'a' &&
		    strcmp(fs.st_fstype, "autofs") == 0) {
			int	f = open(nm, O_RDONLY|O_NDELAY);
			if (f < 0) {
				type = WALK_DNR;
			} else {
				if (fstat(f, &fs) < 0)
					type = WALK_NS;
				close(f);
			}
		}
#endif
		if (S_ISDIR(fs.st_mode)) {
			type = WALK_D;
			if (last != NULL && !last->p_stat && last->p_nlink > 0)
				last->p_nlink--;
		} else if (S_ISLNK(fs.st_mode))
			type = WALK_SL;
		else
			type = WALK_F;
	} else {
		int	err = geterrno();
		statfun	sp;	/* lstat() or fstatat(AT_SYMLINK_NOFOLLOW) */

		if ((state->walkflags & WALK_CHDIR) == 0)
			sp = wdlstat;
		else
			sp = wlstat;
		while ((ret = (*sp)(nm, &fs, last)) < 0 && geterrno() == EINTR)
			;
		if (ret >= 0 &&
		    S_ISLNK(fs.st_mode)) {
			seterrno(err);
			/*
			 * Found symbolic link that points to nonexistent file.
			 */
			ret = (*fn)(varp->Curdir, &fs, WALK_SLN, state);
			goto out;
		} else {
			/*
			 * Found unknown file type because lstat() failed.
			 */
			ret = (*fn)(varp->Curdir, &fs, WALK_NS, state);
			goto out;
		}
	}
	if ((state->walkflags & WALK_MOUNT) != 0 &&
	    varp->Sb.st_dev != fs.st_dev) {
		ret = 0;
		goto out;
	}

type_known:
	if (type == WALK_D) {
		BOOL		need_cd = !(nm[0] == '.' && nm[1] == '\0');
		struct pdirs	*pd = last;
#ifdef	USE_DIRFD
		opendirfun	opendirp = nopendir;
#endif

		ret = 0;
		if ((state->walkflags & WALK_CHDIR) == 0) {
			if ((state->walkflags & (WALK_PHYS|WALK_ALLFOLLOW)) ==
			    WALK_PHYS) {
				sf = wdlstat;
			}
			need_cd = FALSE;
#ifdef	USE_DIRFD
			opendirp = dopendir;
#endif
		} else if ((state->walkflags & (WALK_PHYS|WALK_ALLFOLLOW)) ==
			    WALK_PHYS) {
			sf = wlstat;
		}

		/*
		 * Search parent dir structure for possible loops.
		 * If a loop is found, do not descend.
		 */
		thisd.p_last = last;
		thisd.p_dev  = fs.st_dev;
		thisd.p_ino  = fs.st_ino;
		if (state->walkflags & WALK_NOSTAT && fs.st_nlink >= 2) {
			thisd.p_stat  = FALSE;
			thisd.p_nlink  = fs.st_nlink - 2;
#ifdef	HAVE_ST_FSTYPE
			if (!IS_UFS(fs.st_fstype) &&
			    !IS_ZFS(fs.st_fstype))
				thisd.p_stat  = TRUE;
#ifndef	HAVE_DIRENT_D_TYPE
			else if (state->walkflags &
					(WALK_ARGFOLLOW|WALK_ALLFOLLOW))
				thisd.p_stat  = TRUE;
#endif
#else
			thisd.p_stat  = TRUE;	/* Safe fallback */
#endif
		} else {
			thisd.p_stat  = TRUE;
			thisd.p_nlink  = 1;
		}

		while (pd) {
			if (pd->p_dev == fs.st_dev &&
			    pd->p_ino == fs.st_ino) {
				/*
				 * Found a directory that has been previously
				 * visited already. This may happen with hard
				 * linked directories. We found a loop, so do
				 * not descend this directory.
				 */
				ret = (*fn)(varp->Curdir, &fs, WALK_DP, state);
				goto out;
			}
			pd = pd->p_last;
		}

		if ((state->walkflags & WALK_XDEV) != 0 &&
		    varp->Sb.st_dev != fs.st_dev) {
			/*
			 * A directory that is a mount point. Report and return.
			 */
			ret = (*fn)(varp->Curdir, &fs, type, state);
			goto out;
		}
		if ((state->walkflags & WALK_DEPTH) == 0) {
			/*
			 * Found a directory, check the content past this dir.
			 */
			ret = (*fn)(varp->Curdir, &fs, type, state);

			if (state->flags & WALK_WF_PRUNE)
				goto out;
		}

#ifdef	USE_DIRFD
		thisd.p_dir = (*opendirp)(nm, last);
		/*
		 * If we are out of fd's, close the directory above and
		 * try again.
		 */
		if (thisd.p_dir == (DIR *)NULL && geterrno() == EMFILE) {
			if (last != NULL && last->p_dir) {
				closedir(last->p_dir);
				last->p_dir = (DIR *)NULL;
			}
			thisd.p_dir = (*opendirp)(nm, last);
		}
		/*
		 * Check whether we opened the same file as we expect.
		 */
		if ((state->walkflags & WALK_NOSTAT) == 0) {
			struct stat	fs2;
			if (thisd.p_dir &&
			    (fstat(dirfd(thisd.p_dir), &fs2) < 0 ||
			    fs2.st_ino != fs.st_ino ||
			    fs2.st_dev != fs.st_dev)) {
				seterrno(EAGAIN);
				ret = -1;
				goto out;
			}
		}
		if (need_cd &&
		    thisd.p_dir != NULL && fchdir(dirfd(thisd.p_dir)) < 0) {
#else
		if (need_cd && chdir(nm) < 0) {
#endif
			state->flags |= WALK_WF_NOCHDIR;
			/*
			 * Found a directory that does not allow chdir() into.
			 */
			ret = (*fn)(varp->Curdir, &fs, WALK_DNR, state);
			state->flags &= ~WALK_WF_NOCHDIR;
			goto out;
		} else {
			char	*dp;
			char	*odp;
			int	nents;
			int	Dspace;
			int	olen = varp->Snmlen;

			/*
			 * Add space for '/' & '\0'
			 */
			Dspace = varp->Curdlen - varp->Curdtail - 2;

#ifdef	USE_DIRFD
			if (thisd.p_dir == NULL ||
			    (dp = dfetchdir(thisd.p_dir, ".",
						&nents, NULL, NULL)) == NULL) {
#else
			if ((dp = fetchdir(".", &nents, NULL, NULL)) == NULL) {
#endif
				/*
				 * Found a directory that cannot be read.
				 */
				ret = (*fn)(varp->Curdir, &fs, WALK_DNR, state);
				goto skip;
			}

			odp = dp;
			while (nents > 0 && ret == 0) {
				register char	*name;
				register int	nlen;

				name = &dp[1];
				nlen = strlen(name);

				if (Dspace < nlen) {
					int incr = incr_dspace(state->std[2],
								varp, nlen + 2);
					if (incr < 0) {
						ret = -1;
						break;
					}
					Dspace += incr;
				}
#ifdef	HAVE_DIRENT_D_TYPE
				if (dp[0] == FDT_LNK) {
					state->flags |=
					    WALK_WF_NOTDIR|WALK_WF_ISLNK;

				} else if (dp[0] != FDT_DIR && dp[0] != FDT_UNKN) {
					state->flags |= WALK_WF_NOTDIR;
				}
#endif
				state->level++;
				varp->Snmlen = nlen;
				ret = walk(name, sf, fn, state, &thisd);
				state->level--;

				if (state->flags & WALK_WF_QUIT)
					break;
				nents--;
				dp += nlen +2;
			}
			varp->Snmlen = olen;
			free(odp);
		skip:
#ifdef	USE_DIRFD
			if (thisd.p_dir) {
				closedir(thisd.p_dir);
				thisd.p_dir = (DIR *)NULL;
			}
#endif
			if (need_cd && state->level > 0 && xchdotdot(last,
							otail, state) < 0) {
				ret = geterrno();
				state->flags |= WALK_WF_NOHOME;
				if ((state->walkflags & WALK_NOMSG) == 0) {
					ferrmsg(state->std[2],
					_(
					"Cannot chdir to '..' from '%s/'.\n"),
						varp->Curdir);
				}
				if ((state->walkflags & WALK_NOEXIT) == 0)
					comexit(ret);
				ret = -1;
				goto out;
			}
			if (ret < 0)		/* Do not call callback	    */
				goto out;	/* func past fatal errors   */
		}
		if ((state->walkflags & WALK_DEPTH) != 0) {
			if (varp->Curdtail == 0 &&
			    (state->walkflags & WALK_STRIPLDOT) &&
			    (nm[0] == '.' && nm[1] == '\0')) {
				varp->Curdir[0] = '.';
				varp->Curdir[1] = '\0';
			}
			ret = (*fn)(varp->Curdir, &fs, type, state);
		}
	} else {
		/*
		 * Any other non-directory and non-symlink file type.
		 */
		ret = (*fn)(varp->Curdir, &fs, type, state);
	}
out:
#ifdef	USE_DIRFD
	if (thisd.p_dir)
		closedir(thisd.p_dir);
#endif
	varp->Curdir[otail] = '\0';
	varp->Curdtail = otail;
	return (ret);
}

LOCAL int
incr_dspace(f, varp, amt)
	FILE		*f;
	struct twvars	*varp;
	int		amt;
{
	int	incr = DIR_INCR;
	char	*new;

	if (amt < 0)
		amt = 0;
	while (incr < amt)
		incr += DIR_INCR;
	new = __fjrealloc(f, varp->Curdir, varp->Curdlen + incr,
						"path buffer", JM_RETURN);
	if (new == NULL)
		return (-1);
	varp->Curdir = new;
	varp->Curdlen += incr;
	return (incr);
}

/*
 * Call first to create a useful WALK state default.
 */
EXPORT void
walkinitstate(state)
	struct WALK	*state;
{
	state->flags = state->base = state->level = state->walkflags = 0;
	state->twprivate = NULL;
	state->std[0] = stdin;
	state->std[1] = stdout;
	state->std[2] = stderr;
	state->env = environ;
	state->quitfun = NULL;
	state->qfarg = NULL;
	state->maxdepth = state->mindepth = 0;
	state->lname = state->tree = state->patstate = NULL;
	state->err = state->pflags = state->auxi = 0;
	state->auxp = NULL;
}

/*
 * For users that do not call treewalk(), e.g. star in extract mode.
 */
EXPORT void *
walkopen(state)
	struct WALK	*state;
{
	struct twvars	*varp = __fjmalloc(state->std[2],
					sizeof (struct twvars), "walk vars",
								JM_RETURN);

	if (varp == NULL)
		return (NULL);
	varp->Curdir  = NULL;
	varp->Curdlen = 0;
	varp->Curdtail = 0;
	varp->Flags = TW_MALLOC;
#ifdef	HAVE_FCHDIR
	varp->Home = -1;
#else
	varp->Home[0] = '\0';
#endif
	state->twprivate = varp;
	varp->Walk = state;

	return ((void *)varp);
}

EXPORT int
walkgethome(state)
	struct WALK	*state;
{
	struct twvars	*varp = state->twprivate;
	int		err = EX_BAD;

	if (varp == NULL) {
		if ((state->walkflags & WALK_NOMSG) == 0)
			ferrmsg(state->std[2],
				_("walkgethome: NULL twprivate\n"));
		if ((state->walkflags & WALK_NOEXIT) == 0)
			comexit(err);
		return (-1);
	}
#ifdef	HAVE_FCHDIR
	if (varp->Home >= 0)
		close(varp->Home);
	/*
	 * Note that we do not need O_RDONLY as we do not like to
	 * run readdir() on that directory but just fchdir().
	 */
	if ((varp->Home = open(".", O_SEARCH|O_DIRECTORY|O_NDELAY)) < 0) {
		err = geterrno();
		state->flags |= WALK_WF_NOCWD;
		if ((state->walkflags & WALK_NOMSG) == 0)
			ferrmsg(state->std[2],
				_("Cannot get working directory.\n"));
		if ((state->walkflags & WALK_NOEXIT) == 0)
			comexit(err);
		return (-1);
	}
#ifdef	F_SETFD
	fcntl(varp->Home, F_SETFD, FD_CLOEXEC);
#endif
#else
	if (getcwd(varp->Home, sizeof (varp->Home)) == NULL) {
		err = geterrno();
		state->flags |= WALK_WF_NOCWD;
		if ((state->walkflags & WALK_NOMSG) == 0)
			ferrmsg(state->std[2],
				_("Cannot get working directory.\n"));
		if ((state->walkflags & WALK_NOEXIT) == 0)
			comexit(err);
		return (-1);
	}
#endif
	return (0);
}

/*
 * Walk back to the directory from where treewalk() has been started.
 */
EXPORT int
walkhome(state)
	struct WALK	*state;
{
	struct twvars	*varp = state->twprivate;

	if (varp == NULL)
		return (0);
#ifdef	HAVE_FCHDIR
	if (varp->Home >= 0)
		return (fchdir(varp->Home));
#else
	if (varp->Home[0] != '\0')
		return (chdir(varp->Home));
#endif
	return (0);
}

/*
 * Walk back to the previous "cwd".
 * This always works with fchdir() and low directory nesting.
 * In the other case, it is assumed that walkcwd() is called after walkhome().
 */
EXPORT int
walkcwd(state)
	struct WALK	*state;
{
	struct twvars	*varp = state->twprivate;
	char		c;

	if (varp == NULL)
		return (0);

	if (varp->pdirs == NULL)
		return (0);

#if	defined(HAVE_FCHDIR) && defined(USE_DIRFD)
	if (varp->pdirs->p_dir)
		return (fchdir(dirfd(varp->pdirs->p_dir)));
#endif
	c = varp->Curdir[state->base];
	varp->Curdir[state->base] = '\0';

	if (chdir(varp->Curdir) < 0) {
		if (geterrno() != ENAMETOOLONG) {
			varp->Curdir[state->base] = c;
			return (-1);
		}
		if (xchdir(varp->Curdir) < 0) {
			varp->Curdir[state->base] = c;
			return (-1);
		}
	}
	varp->Curdir[state->base] = c;
	return (0);
}

EXPORT int
walkclose(state)
	struct WALK	*state;
{
	int		ret = 0;
	struct twvars	*varp = state->twprivate;

	if (varp == NULL)
		return (0);
#ifdef	HAVE_FCHDIR
	if (varp->Home >= 0)
		ret = close(varp->Home);
	varp->Home = -1;
#else
	varp->Home[0] = '\0';
#endif
	if (varp->Flags & TW_MALLOC) {
		free(varp);
		state->twprivate = NULL;
	}
	return (ret);
}

/*
 * Return length of last pathnmame component.
 */
EXPORT int
walknlen(state)
	struct WALK	*state;
{
	struct twvars	*varp = state->twprivate;

	return (varp->Snmlen);
}

/*
 * We only call xchdotdot() with state->level > 0,
 * so "last" is known to be != NULL.
 */
LOCAL int
xchdotdot(last, tail, state)
	struct pdirs	*last;
	int		tail;
	struct WALK	*state;
{
	struct twvars	*varp = state->twprivate;
	char	c;
	struct stat sb;

#ifdef	USE_DIRFD
	if (last->p_dir) {
		if (fchdir(dirfd(last->p_dir)) >= 0)
			return (0);
	}
#endif

	if (chdir("..") >= 0) {
		seterrno(0);
		if (stat(".", &sb) >= 0) {
			if (sb.st_dev == last->p_dev &&
			    sb.st_ino == last->p_ino)
				return (0);
		}
	}

	if (walkhome(state) < 0)
		return (-1);

	c = varp->Curdir[tail];
	varp->Curdir[tail] = '\0';
	if (chdir(varp->Curdir) < 0) {
		if (geterrno() != ENAMETOOLONG) {
			varp->Curdir[tail] = c;
			return (-1);
		}
		if (xchdir(varp->Curdir) < 0) {
			varp->Curdir[tail] = c;
			return (-1);
		}
	}
	varp->Curdir[tail] = c;
	seterrno(0);
	if (stat(".", &sb) >= 0) {
		if (sb.st_dev == last->p_dev &&
		    sb.st_ino == last->p_ino)
			return (0);
	}
	return (-1);
}

LOCAL int
xchdir(p)
	char	*p;
{
	char	*p2;

	while (*p) {
		if ((p2 = strchr(p, '/')) != NULL)
			*p2 = '\0';
		if (chdir(p) < 0)
			return (-1);
		if (p2 == NULL)
			break;
		*p2 = '/';
		p = &p2[1];
	}
	return (0);
}

/*
 * Get stat() info when WALK_CHDIR is set.
 * Use vanilla stat()
 */
LOCAL int
wstat(nm, sp, pd)
	const char	*nm;
	struct stat	*sp;
	struct pdirs	*pd;
{
#ifdef	HAVE_FSTATAT
	return (fstatat(AT_FDCWD, nm, sp, _AT_TRIGGER));
#else
	return (stat(nm, sp));
#endif
}

/*
 * Get lstat() info when WALK_CHDIR is set.
 * Use vanilla lstat()
 */
LOCAL int
wlstat(nm, sp, pd)
	const char	*nm;
	struct stat	*sp;
	struct pdirs	*pd;
{
#ifdef	HAVE_FSTATAT
	return (fstatat(AT_FDCWD, nm, sp, AT_SYMLINK_NOFOLLOW|_AT_TRIGGER));
#else
	return (lstat(nm, sp));
#endif
}

/*
 * Get stat() info when WALK_CHDIR is not set.
 * Use vanilla stat()/statat() based on the directory fd.
 */
LOCAL int
wdstat(nm, sp, pd)
	const char	*nm;
	struct stat	*sp;
	struct pdirs	*pd;
{
	int	fd;

#ifdef	USE_DIRFD
	if (pd && pd->p_dir) {
		fd = dirfd(pd->p_dir);
	} else {
#endif
		fd = AT_FDCWD;
		if (pd)
			nm = pd->p_varp->Curdir;
#ifdef	USE_DIRFD
	}
#endif
	return (fstatat(fd, nm, sp, _AT_TRIGGER));
}

/*
 * Get lstat() info when WALK_CHDIR is not set.
 * Use vanilla lstat()/statat() based on the directory fd.
 */
LOCAL int
wdlstat(nm, sp, pd)
	const char	*nm;
	struct stat	*sp;
	struct pdirs	*pd;
{
	int	fd;

#ifdef	USE_DIRFD
	if (pd && pd->p_dir) {
		fd = dirfd(pd->p_dir);
	} else {
#endif
		fd = AT_FDCWD;
		if (pd)
			nm = pd->p_varp->Curdir;
#ifdef	USE_DIRFD
	}
#endif
	return (fstatat(fd, nm, sp, AT_SYMLINK_NOFOLLOW|_AT_TRIGGER));
}

#ifdef	USE_DIRFD
/*
 * Open directory WALK_CHDIR is set.
 */
/* ARGSUSED */
LOCAL DIR *
nopendir(name, pd)
	const char	*name;
	struct pdirs	*pd;
{
	return (opendir(name));
}

/*
 * Open directory WALK_CHDIR is not set.
 */
LOCAL DIR *
dopendir(name, pd)
	const char	*name;
	struct pdirs	*pd;
{
	char	*nm;
	char	*p;
	char	*p2;
	DIR	*ret = NULL;
	int	fd;
	int	dfd;

	if (pd && pd->p_dir) {
		fd = dirfd(pd->p_dir);
	} else {
		fd = AT_FDCWD;
		if (pd)
			name = pd->p_varp->Curdir;
	}
	if (((dfd = openat(fd, name, O_RDONLY|O_DIRECTORY|O_NDELAY)) < 0 &&
	    geterrno() != ENAMETOOLONG) || (ret = fdopendir(dfd)) == NULL) {
		return (NULL);
	}
	if (ret)
		return (ret);
	if (pd)
		name = pd->p_varp->Curdir;
	if ((nm = strdup(name)) == NULL) {
		seterrno(ENAMETOOLONG);
		return ((DIR *)NULL);
	}

	p = nm;
	fd = AT_FDCWD;
	if (*p == '/') {
		fd = openat(fd, "/", O_SEARCH|O_DIRECTORY|O_NDELAY);
		while (*p == '/')
			p++;
	}
	while (*p) {
		if ((p2 = strchr(p, '/')) != NULL) {
			if (p2[1] == '\0')
				p2 = NULL;
			else
				*p2++ = '\0';
		}
		if ((dfd = openat(fd, p,
				p2 ? O_SEARCH|O_DIRECTORY|O_NDELAY :
					O_RDONLY|O_DIRECTORY|O_NDELAY)) < 0) {
			int err = geterrno();

			free(nm);
			close(fd);
			if (err == EMFILE)
				seterrno(err);
			else
				seterrno(ENAMETOOLONG);
			return ((DIR *)NULL);
		}
		close(fd);
		fd = dfd;
		if (p2 == NULL)
			break;
		while (*p2 == '/')
			p2++;
		p = p2;
	}
	free(nm);
	ret = fdopendir(fd);
	return (ret);
}
#endif	/* USE_DIRFD */
