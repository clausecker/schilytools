/* @(#)star_sym.c	1.21 18/07/23 Copyright 2005-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)star_sym.c	1.21 18/07/23 Copyright 2005-2018 J. Schilling";
#endif
/*
 *	Read in the star inode data base and write a human
 *	readable version.
 *
 *	Copyright (c) 2005-2018 J. Schilling
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
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include "star.h"
#include "restore.h"
#include "dumpdate.h"
#include <schily/jmpdefs.h>	/* To include __jmalloc() */
#include <schily/fcntl.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include <schily/maxpath.h>
#include "starsubs.h"

#ifndef	HAVE_LSTAT
#define	lstat	stat
#endif

#if	S_ISUID == TSUID && S_ISGID == TSGID && S_ISVTX == TSVTX && \
	S_IRUSR == TUREAD && S_IWUSR == TUWRITE && S_IXUSR == TUEXEC && \
	S_IRGRP == TGREAD && S_IWGRP == TGWRITE && S_IXGRP == TGEXEC && \
	S_IROTH == TOREAD && S_IWOTH == TOWRITE && S_IXOTH == TOEXEC

#define	HAVE_POSIX_MODE_BITS	/* st_mode bits are equal to TAR mode bits */
#endif

struct star_stats	xstats;		/* for printing statistics	*/

dev_t	curfs = NODEV;			/* Current st_dev for -M option	*/
char	*vers;				/* the full version string	*/
BOOL	force_remove = FALSE;		/* -force-remove on extraction	*/
BOOL	remove_recursive = FALSE;	/* -remove-recursive on extract	*/
BOOL	dopartial = FALSE;		/* -partial in incremental mode	*/
BOOL	forcerestore = FALSE;		/* -force-restore in incremental mode	*/
BOOL	uncond	  = FALSE;		/* -U unconditional extract	*/
BOOL	nowarn	  = FALSE;		/* -nowarn has been specified	*/
int	xdebug	  = 0;			/* eXtended debug level		*/
GINFO	_grinfo;			/* Global read information	*/
GINFO	*grip = &_grinfo;		/* Global read info pointer	*/

LOCAL	void	star_mkvers	__PR((void));
LOCAL	void	usage		__PR((int ret));
EXPORT	int	main		__PR((int ac, char *av[]));
LOCAL	void	make_symtab	__PR((int ac, char *av[]));


EXPORT int
hdr_type(name)
	char	*name;
{
	return (-1);
}

EXPORT BOOL
remove_file(name, isfirst)
	register char	*name;
		BOOL	isfirst;
{
	return (FALSE);
}

EXPORT BOOL
make_adir(info)
	register FINFO	*info;
{
	return (FALSE);
}

#include <schily/stat.h>
EXPORT BOOL
_getinfo(name, info)
	char	*name;
	register FINFO	*info;
{
	struct stat sb;

	if (lstatat(name, &sb, AT_SYMLINK_NOFOLLOW) < 0)
		return (FALSE);

	info->f_name = name;
	info->f_ino = sb.st_ino;
	switch ((int)(sb.st_mode & S_IFMT)) {

	case	S_IFREG:	info->f_filetype = F_FILE; break;
	case	S_IFDIR:	info->f_filetype = F_DIR; break;
#ifdef	S_IFLNK
	case	S_IFLNK:	info->f_filetype = F_SLINK; break;
#endif

	default:		info->f_filetype = F_SPEC;
	}
	return (TRUE);
}

#ifdef	HAVE_POSIX_MODE_BITS	/* st_mode bits are equal to TAR mode bits */
#define	OSMODE(tarmode)	    (tarmode)
#else
#define	OSMODE(tarmode)	    ((tarmode & TSUID   ? S_ISUID : 0)  \
			    | (tarmode & TSGID   ? S_ISGID : 0) \
			    | (tarmode & TSVTX   ? S_ISVTX : 0) \
			    | (tarmode & TUREAD  ? S_IRUSR : 0) \
			    | (tarmode & TUWRITE ? S_IWUSR : 0) \
			    | (tarmode & TUEXEC  ? S_IXUSR : 0) \
			    | (tarmode & TGREAD  ? S_IRGRP : 0) \
			    | (tarmode & TGWRITE ? S_IWGRP : 0) \
			    | (tarmode & TGEXEC  ? S_IXGRP : 0) \
			    | (tarmode & TOREAD  ? S_IROTH : 0) \
			    | (tarmode & TOWRITE ? S_IWOTH : 0) \
			    | (tarmode & TOEXEC  ? S_IXOTH : 0))
#endif

EXPORT mode_t
#ifdef	PROTOTYPES
osmode(register mode_t tarmode)
#else
osmode(tarmode)
	register mode_t		tarmode;
#endif
{
	register mode_t		_osmode;

	_osmode = OSMODE(tarmode);
	return (_osmode);
}


EXPORT char *
hdr_name(type)
	int	type;
{
	return ("XXX");
}

EXPORT BOOL
create_dirs(name)
	register char	*name;
{
	return (TRUE);
}

EXPORT char *
dt_name(type)
	int	type;
{
	return ("unknown");
}

EXPORT int
dt_type(name)
	char	*name;
{
	return (DT_UNKN);
}

EXPORT BOOL
same_symlink(info)
	FINFO	*info;
{
	return (FALSE);
}

EXPORT BOOL
same_special(info)
	FINFO	*info;
{
	return (FALSE);
}

LOCAL char	strvers[] = "1.5";
LOCAL void
star_mkvers()
{
	char	buf[512];

	if (vers != NULL)
		return;

	js_snprintf(buf, sizeof (buf),
		"%s %s (%s-%s-%s)", "star", strvers, HOST_CPU, HOST_VENDOR, HOST_OS);

	vers = ___savestr(buf);
}

LOCAL void
usage(ret)
	int	ret;
{
	error("Usage:\t%s [options] < file\n", get_progname());
	error("Options:\n");
	error("\t-help\t\tprint this help\n");
	error("\t-version\tPrint version number.\n");
	error("\n%s extracts star incremental restore database\n", get_progname());
	exit(ret);
	/* NOTREACHED */
}

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
extern	BOOL		is_star;
	int		cac = ac;
	char	*const *cav = av;
	char		*opt = "help,h,version";
	BOOL		help = FALSE;
	BOOL		prvers = FALSE;

	is_star = FALSE;
	save_args(ac, av);
	star_mkvers();

	cac--;
	cav++;
	if (getallargs(&cac, &cav, opt, &help, &help, &prvers) < 0) {
		errmsgno(EX_BAD, "Bad Option: '%s'.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (prvers) {
		printf("%s: %s\n\n", get_progname(), vers);
		gtprintf("Copyright (C) 2005-2018 Jörg Schilling\n");
		gtprintf("This is free software; see the source for copying conditions.  There is NO\n");
		gtprintf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		exit(0);
	}

	/*
	 * As long as all options cause star_sym to exit, we may continue
	 * with this lazy arg handling.
	 */
	if (ac > 2) {
		/*
		 * Reconstruct star-symtable
		 * Call:
		 * star_sym <restore-dir "."> <backup-dir>
		 */
		make_symtab(ac, av);
		return (0);
	}
	if (ac < 2)
		sym_open(NULL);		/* dump ./star-symtable */
	else
		sym_open(av[1]);	/* sump named symtable */
	printLsym(stderr);
	return (0);
}

/* ------------------------------------------------------------------------- */
#include <schily/nlsdefs.h>

#include <schily/walk.h>
#include <schily/find.h>


LOCAL	int	walkfunc	__PR((char *nm, struct stat *fs, int type, struct WALK *state));
EXPORT	int	doscan		__PR((int ac, char **av));

LOCAL void
make_symtab(ac, av)
	int	ac;
	char	*av[];
{
extern	BOOL	restore_valid;

	sym_initmaps();
	restore_valid = TRUE;
	doscan(ac, av);
	sym_close();
}


LOCAL int
walkfunc(nm, fs, type, state)
	char		*nm;
	struct stat	*fs;
	int		type;
	struct WALK	*state;
{
	if (state->quitfun) {
		/*
		 * Check for shell builtin signal abort condition.
		 */
		if ((*state->quitfun)(state->qfarg)) {
			state->flags |= WALK_WF_QUIT;
			return (0);
		}
	}
	if (type == WALK_NS) {
		ferrmsg(state->std[2],
				gettext("Cannot stat '%s'.\n"), nm);
		state->err = 1;
		return (0);
	} else if (type == WALK_SLN && (state->walkflags & WALK_PHYS) == 0) {
		ferrmsg(state->std[2],
				gettext("Cannot follow symlink '%s'.\n"), nm);
		state->err = 1;
		return (0);
	} else if (type == WALK_DNR) {
		if (state->flags & WALK_WF_NOCHDIR) {
			ferrmsg(state->std[2],
				gettext("Cannot chdir to '%s'.\n"), nm);
		} else {
			ferrmsg(state->std[2],
				gettext("Cannot read '%s'.\n"), nm);
		}
		state->err = 1;
		return (0);
	}

	if (state->maxdepth >= 0 && state->level >= state->maxdepth)
		state->flags |= WALK_WF_PRUNE;
	if (state->mindepth >= 0 && state->level < state->mindepth)
		return (0);

	{
#ifndef	HAVE_FCHDIR
		char		cwd[MAXPATHNAME+1];
#else
		int		f;
#endif
		struct stat	sb;
		char		name[4096];

		sprintf(name, "%s/%s", (char *)state->auxp, &nm[1]);
		sb.st_ino = 0;

#ifndef	HAVE_FCHDIR
		cwd[0] = '\0';
		getcwd(cwd, sizeof (cwd));
#else
		/*
		 * Note that we do not need O_RDONLY as we do not like to
		 * run readdir() on that directory but just fchdir().
		 */
		f = open(".", O_SEARCH|O_DIRECTORY);
#endif
		walkhome(state);
		lstatat(name, &sb, AT_SYMLINK_NOFOLLOW);
#ifndef	HAVE_FCHDIR
		chdir(cwd);
#else
		fchdir(f);
		close(f);
#endif
		padd_node(nm, sb.st_ino, fs->st_ino, (type ==  WALK_D) ?  I_DIR:0);
	}
	return (0);
}


EXPORT int
doscan(ac, av)
	int	ac;
	char	**av;
{
	finda_t	fa;
	findn_t	*Tree;
	struct WALK	walkstate;
	int	oraise[3];
	int	ret = 0;
	int	i;
	FILE	*std[3];

	std[0] = stdin;
	std[1] = stdout;
	std[2] = stderr;

	find_argsinit(&fa);
	for (i = 0; i < 3; i++) {
		oraise[i] = file_getraise(std[i]);
		file_raise(std[i], FALSE);
		fa.std[i] = std[i];
	}
	fa.walkflags = WALK_CHDIR | WALK_PHYS;
	fa.walkflags |= WALK_NOEXIT;

	Tree = 0;
	if (Tree == 0) {
		Tree = find_printnode();
	} else if (!fa.found_action) {
		Tree = find_addprint(Tree, &fa);
		if (Tree == (findn_t *)NULL) {
			ret = geterrno();
			goto out;
		}
	}
	walkinitstate(&walkstate);
	for (i = 0; i < 3; i++)
		walkstate.std[i] = std[i];
	if (fa.patlen > 0) {
		walkstate.patstate = __fjmalloc(std[2],
					sizeof (int) * fa.patlen,
					"space for pattern state", JM_RETURN);
		if (walkstate.patstate == NULL) {
			ret = geterrno();
			goto out;
		}
	}

	find_timeinit(time(0));

	walkstate.walkflags	= fa.walkflags;
	walkstate.maxdepth	= fa.maxdepth;
	walkstate.mindepth	= fa.mindepth;
	walkstate.lname		= NULL;
	walkstate.tree		= Tree;
	walkstate.err		= 0;
	walkstate.pflags	= 0;
	walkstate.auxp		= av[2];

	treewalk(av[1], walkfunc, &walkstate);

	find_free(Tree, &fa);
	ret = walkstate.err;

out:
	for (i = 0; i < 3; i++) {
		fflush(std[i]);
		if (ferror(std[i]))
			clearerr(std[i]);
		file_raise(std[i], oraise[i]);
	}
	return (ret);
}
