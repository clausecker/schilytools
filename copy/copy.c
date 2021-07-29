/* @(#)copy.c	1.53 21/07/22 Copyright 1984, 86-90, 95-97, 99, 2000-2021 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)copy.c	1.53 21/07/22 Copyright 1984, 86-90, 95-97, 99, 2000-2021 J. Schilling";
#endif
/*
 *	copy files ...
 *
 *	Copyright (c) 1984, 86-90, 95-97, 99, 2000-2021 J. Schilling
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

/*
 *	Operation modes:
 *
 *	copy [-r] [-q] [-v] [-i] [-s] [-o] [-sparse] file1 file2
 *	copies file1 to file2
 *
 *	copy [-r] [-q] [-v] [-i] [-s] [-o] [-sparse] file1...filen todir
 *	copies file1...filen to todir
 *
 *	copy [-r] [-q] [-v] [-i] [-s] [-o] [-sparse] from
 *	prompts "To: "
 *	for <to filename>
 *
 *	copy [-r] [-v] [-s] [-o] [-sparse] -i
 *	prompts "From: " and "To: "
 *	until a null name is read.
 */

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/varargs.h>
#include <schily/errno.h>
#include <schily/param.h>	/* DEV_BSIZE */
#include <schily/stat.h>
#include <schily/time.h>
#include <schily/fcntl.h>
#include <schily/string.h>
#include <schily/maxpath.h>
#include <schily/libport.h>
#include <schily/nlsdefs.h>
#include <schily/schily.h>

		/* Probably only needed for Mark Williams C */
#ifndef	EEXIST
#define	EEXIST	-1	/* XXX ??? */
#endif
#define	is_dir(sp)	S_ISDIR((sp)->st_mode)
#define	is_link(sp)	S_ISLNK((sp)->st_mode)
#ifndef	S_IFLNK
#define	lstat		stat
#endif
#ifndef	HAVE_LCHOWN
#define	lchown	chown
#endif
#define	file_type(sp)	((int)((sp)->st_mode & S_IFMT))
#define	file_size(sp)	((sp)->st_size)
#define	disk_size(sp)	((sp)->st_size)
#define	file_ino(sp)	((sp)->st_ino)
#define	file_dev(sp)	((sp)->st_dev)
#define	STATBUF		struct stat

#ifdef	HAVE_ST_BLOCKS
#undef	disk_size
#if	defined(hpux) || defined(__hpux)
#define	disk_size(sp)	((sp)->st_blocks * (off_t)1024)
#else
#define	disk_size(sp)	((sp)->st_blocks * (off_t)DEV_BSIZE)
#endif
#endif

#ifndef	HAVE_VALLOC
#define	valloc	malloc
#endif
#define	COPY_SIZE	(8*1024*1024)
#define	COPY_SIZE_SMALL	(16*1024)

char	*copybuf;
char	*zeroblk;
int	copybsize	= COPY_SIZE;
int	uid;
int	verbose		= FALSE;
int	setall		= FALSE;
int	setown		= FALSE;
int	setgrp		= FALSE;
int	olddate		= FALSE;
int	setperm		= FALSE;
int	Recurse		= FALSE;
int	recurse		= FALSE;
int	is_recurse	= FALSE;
int	query		= FALSE;
int	interactive	= FALSE;
int	sparseflag	= FALSE;
int	force_hole	= FALSE;

LOCAL	void	usage		__PR((int ret));
#ifdef	_FASCII		/* Mark Williams C	*/
LOCAL	void	setup_env	__PR((void));
#endif
#if	tos
LOCAL	BOOL	is_root		__PR((char *n));
LOCAL	int	mystat		__PR((char *name, STATBUF *statbuf));
#endif
EXPORT	int	main		__PR((int ac, char **av));
LOCAL	int	do_one_arg	__PR((char *from));
LOCAL	int	do_interactive	__PR((void));
LOCAL	int	copy		__PR((char *from, char *to));
LOCAL	int	copyfile	__PR((char *from, char *to, off_t fromsize,
							off_t disksize));
LOCAL	int	copy_link	__PR((char *from, char *to));
LOCAL	int	do_recurse	__PR((char *from, char *to));
LOCAL	BOOL	samefile	__PR((STATBUF * sp1, STATBUF * sp2));
LOCAL	void	set_access	__PR((STATBUF * fromstat, char *to,
							BOOL to_exists));
LOCAL	void	etoolong	__PR((char *name));
LOCAL	BOOL	yes		__PR((char *form, ...));
LOCAL	BOOL	getbase		__PR((char *path, char *basenamep,
							size_t bsize));
LOCAL	void	mygetline	__PR((char *pstr, char *str, int len));
LOCAL	int	xutimes		__PR((char *name, STATBUF * sp));
LOCAL	BOOL	doremove	__PR((char *name));
#if	defined(SEEK_HOLE) && defined(SEEK_DATA)
LOCAL	BOOL	sparse_file	__PR((int fd, off_t fsize));
LOCAL	int	sparse_copy	__PR((int fin, int fout, char *from, char *to,
							off_t fsize));
LOCAL	int	write_end_hole	__PR((int fout, char *to, off_t fsize));
#endif

LOCAL void
usage(ret)
	int	ret;
{
	error(_("Usage:\tcopy -i [options]\n"));
	error(_("\tcopy [options] from to\n"));
	error(_("\tcopy [options] file1..filen target_dir\n"));
	error(_("Options:\n"));
	error(_("\t-q\t\tquery to confirm each copy\n"));
	error(_("\t-i\t\tinteractive copy / prompt to confirm to overwrite files\n"));
	error(_("\t-R\t\trecursive copy - POSIX mode\n"));
	error(_("\t-r\t\trecursive copy\n"));
	error(_("\t-v\t\tbe verbose\n"));
	error(_("\t-setowner\trestore the original user\n"));
	error(_("\t-setgrp\t\trestore the original group\n"));
	error(_("\t-s\t\trestore the original user and group\n"));
	error(_("\t-olddate|-o\trestore the original access times\n"));
	error(_("\t-p\t\tpreserve file permissons ids and acces times\n"));
	error(_("\t-sparse\t\tpreserve holes in sparse regular files\n"));
	error(_("\t-force-hole\ttry to insert holes into all copied regular files\n"));
	error(_("\t-help\t\tPrint this help.\n"));
	error(_("\t-version\tPrint version information and exit.\n"));
	exit(ret);
}

#ifdef	_FASCII		/* Mark Williams C	*/
char	*_stksize = (char *)8192;

LOCAL void
setup_env()
{
	register char	*ep;
	extern	 char	*getenv();

	if ((ep = getenv("PATH")) == (char *)NULL || *ep == '\0')
		putenv("PATH=.bin,,\\bin,\\lib");

	if ((ep = getenv("SUFF")) == (char *)NULL || *ep == '\0')
		putenv("SUFF=,.prg,.tos,.ttp");

	if ((ep = getenv("LIBPATH")) == (char *)NULL || *ep == '\0')
		putenv("LIBPATH=\\lib,\\bin");

	if ((ep = getenv("TMPDIR")) == (char *)NULL || *ep == '\0')
		putenv("TMPDIR=\\tmp");

	if ((ep = getenv("INCDIR")) == (char *)NULL || *ep == '\0')
		putenv("INCDIR=\\include");
}
#endif

#if	tos

#define	is_char(c)	((c) >= 'a' && (c) <= 'z' || (c) >= 'A' && (c) <= 'Z')

LOCAL BOOL
is_root(n)
	register char	*n;
{
	return (strlen(n) == 2 && is_char(n[0]) && n[1] == ':');
}

LOCAL int
mystat(name, statbuf)
	char	*name;
	STATBUF *statbuf;
{
	int	ret;

	if ((ret = stat(name, statbuf)) < 0) {
		if (is_root(name)) {
			statbuf->st_mode = S_IFDIR;
			return (0);
		}
	}
	return (ret);
}

#define	stat		mystat

#endif

LOCAL	char *opts =
/* CSTYLED */
"q,i,v,R,r,s,setowner,setgrp,o,olddate,p,is_recurse,sparse,force-hole,help,h,version";

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	int	cac;
	char	* const * cav;
	char	*lastarg = NULL;
#if	defined(USE_NLS)
	char	*dir;
#endif
	STATBUF statbuf;
	char	toname[PATH_MAX];
	char	frombase[PATH_MAX];
	int	cnt;
	int	filecount;
	int	ret;
	BOOL	last_is_dir = FALSE;
	BOOL	help	  = FALSE;
	BOOL	prversion = FALSE;

	save_args(ac, av);

	file_raise((FILE *)NULL, FALSE);

	(void) setlocale(LC_ALL, "");

#if	defined(USE_NLS)
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "copy"	/* Use this only if it weren't */
#endif
	dir = searchfileinpath("share/locale", F_OK,
					SIP_ANY_FILE|SIP_NO_PATH, NULL);
	if (dir)
		(void) bindtextdomain(TEXT_DOMAIN, dir);
	else
#if defined(PROTOTYPES) && defined(INS_BASE)
		(void) bindtextdomain(TEXT_DOMAIN, INS_BASE "/share/locale");
#else
		(void) bindtextdomain(TEXT_DOMAIN, "/usr/share/locale");
#endif
	(void) textdomain(TEXT_DOMAIN);
#endif

#ifdef	_FASCII			/* Mark Williams C	*/
	stderr->_ff &= ~_FSTBUF; /* setbuf was called ??? */

	setup_env();
#endif
	uid = geteuid();
	cac = --ac;
	cav = ++av;

	if (getallargs(&cac, &cav, opts,
			&query,
			&interactive,
			&verbose,
			&Recurse,
			&recurse,
			&setall, &setown, &setgrp,
			&olddate, &olddate,
			&setperm,
			&is_recurse,
			&sparseflag, &force_hole,
			&help, &help, &prversion) < 0) {
		errmsgno(EX_BAD, _("Bad flag: '%s'.\n"), cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (prversion) {
		/* CSTYLED */
		printf(_("Copy release %s %s (%s-%s-%s) Copyright (C) 1984, 86-90, 95-97, 99, 2000-2021 %s\n"),
				"1.53", "2021/07/22",
				HOST_CPU, HOST_VENDOR, HOST_OS,
				_("Joerg Schilling"));
		exit(0);
	}

	if (Recurse)
		recurse = TRUE;
	if (setperm) { setall = TRUE; olddate = TRUE; };
	if (setall) { setown = TRUE; setgrp = TRUE; };

	filecount = 0;
	cac = ac;
	cav = av;

	while (getfiles(&cac, &cav, opts) > 0) {
		filecount++;
		lastarg = cav[0];
		cac--;
		cav++;
	}
	if (filecount == 0 && !interactive)
		usage(EX_BAD);

	copybuf = valloc(copybsize);
	if (copybuf == NULL) {
		copybsize = COPY_SIZE_SMALL;
		copybuf = valloc(copybsize);
	}
	if (copybuf == NULL)
		comerr(("Cannot allocate copy buffer.\n"));
	fillbytes(copybuf, copybsize, '\0');
	zeroblk	= &copybuf[1024];


	if (interactive && filecount == 0)
		exit(do_interactive());
	if (filecount == 1)
		exit(do_one_arg(lastarg));
	if (filecount > 2) {
		if (stat(lastarg, &statbuf) < 0)
			comerr("'%s'%s%s", lastarg, _(" must be a directory, "),
					_("but cannot get status on it\n"));
		if (!is_dir(&statbuf)) {
			errmsgno(EX_BAD, _("'%s' is not a directory.\n"),
					lastarg);
			usage(EX_BAD);
		} else {
			last_is_dir = TRUE;
		}
	} else {
		if (stat(lastarg, &statbuf) >= 0)
			if (is_dir(&statbuf))
				last_is_dir = TRUE;
	}
	cac = ac;
	cav = av;
	cnt = 0;
	ret = 0;
	for (; getfiles(&cac, &cav, opts) > 0; cac--, cav++) {
		char	*tonmp;

		if (++cnt >= filecount)
			exit(ret);
		if (filecount > 2 || is_recurse || (Recurse && last_is_dir)) {
			if (!getbase(cav[0], frombase, sizeof (frombase))) {
				etoolong(cav[0]);
				ret = 1;
				continue;
			}
			if (snprintf(toname, sizeof (toname), "%s%s%s",
			    lastarg, PATH_DELIM_STR, frombase) >=
			    sizeof (toname)) {
				etoolong(toname);
				ret = 1;
				continue;
			}
			tonmp = toname;
		} else {
			tonmp = lastarg;
		}
		if (copy(cav[0], tonmp) < 0)
			ret = 1;
	}
	exit(ret);
	return (ret);	/* Keep lint happy */
}

LOCAL int
do_one_arg(from)
	char	*from;
{
	char	toname[PATH_MAX];

	mygetline(_("To:"), toname, sizeof (toname));
	if (toname[0] != '\0' && copy(from, toname) < 0)
		return (1);
	return (0);
}

LOCAL int
do_interactive()
{
	char	fromname[PATH_MAX];
	char	toname[PATH_MAX];
	int	ret = 0;

	for (;;) {
		mygetline(_("From:"), fromname, sizeof (fromname));
		if (fromname[0] == '\0')
			return (ret);
		mygetline(_("To:"), toname, sizeof (toname));
		if (toname[0] == '\0')
			return (ret);
		if (copy(fromname, toname) < 0)
			ret = 1;
	}
}

/*
 * Copy a single file
 */
LOCAL int
copy(from, to)
	char	*from;
	char	*to;
{
	char	name[PATH_MAX];
	char	frombase[PATH_MAX];
	STATBUF fromstat;
	STATBUF tostat;
	BOOL	to_is_dir = FALSE;
	BOOL	to_exists = FALSE;

	if (!getbase(from, frombase, sizeof (frombase))) {
		etoolong(from);
		return (-1);
	}

	if (is_recurse && (streql(frombase, ".") || streql(frombase, ".."))) {
		return (0);
	}
	if (query && !interactive && !yes(_("%s to %s?"), from, to)) {
		return (0);
	}
	if (lstat(from, &fromstat) < 0) {
		/*
		 * First stat() to verify whether a file with the literal name
		 * ".*" or "*" exists.
		 */
		if (is_recurse &&
			(streql(frombase, ".*") || streql(frombase, "*"))) {
			return (0);
		}
		errmsg(_("Cannot get status of '%s'.\n"), from);
		return (-1);
	}
	if (stat(to, &tostat) >= 0) {
		if (samefile(&fromstat, &tostat)) {
			errmsgno(EEXIST,
				_("Will not copy '%s' to itself ('%s').\n"),
								from, to);
			return (-1);
		}
		if (!(to_is_dir = is_dir(&tostat)) &&
				interactive && !yes(_("overwrite %s? "), to)) {
			return (0);
		}
		to_exists = TRUE;
	}
	if (is_dir(&fromstat)) {
		if (!to_is_dir && mkdir(to, 0777) < 0) {
			errmsg(_("Cannot make dir '%s'.\n"), to);
			return (-1);
		}
	} else if (to_is_dir) {
		if (snprintf(name, sizeof (name), "%s%s%s",
			    to, PATH_DELIM_STR, frombase) >= sizeof (name)) {
			etoolong(name);
			return (-1);
		}
		to = name;
		if (stat(to, &tostat) >= 0) {
			if (samefile(&fromstat, &tostat)) {
				errmsgno(EEXIST,
				_("Will not copy '%s' to itself ('%s').\n"),
								from, to);
				return (-1);
			}
			if (interactive && !yes(_("overwrite %s? "), to)) {
				return (0);
			}
			to_exists = TRUE;
		} else {
			to_exists = FALSE;
		}
	}
	if (!is_dir(&fromstat)) {
		switch (file_type(&fromstat)) {
		default:
		case S_IFREG:
			if (copyfile(from, to,
			    file_size(&fromstat), disk_size(&fromstat)) < 0) {
				return (-1);
			}
		break;
#ifdef	S_IFCHR
		case S_IFCHR:
#endif
#ifdef	S_IFBLK
		case S_IFBLK:
#endif
#if	defined(S_IFCHR) || defined(S_IFBLK)
#if	defined(HAVE_MKNOD) && defined(HAVE_ST_RDEV)
			if (mknod(to, fromstat.st_mode, fromstat.st_rdev) < 0) {
#else
			seterrno(EINVAL);
			if (1) {
#endif
				errmsg(_("Could not make device '%s'.\n"), to);
				return (-1);
			}
			break;
#endif
#ifdef	S_IFIFO
		case S_IFIFO:
#ifdef	HAVE_MKFIFO
			if (mkfifo(to, fromstat.st_mode) < 0) {
#else
#ifdef	HAVE_MKNOD
#ifdef	HAVE_ST_RDEV
			if (mknod(to, fromstat.st_mode, fromstat.st_rdev) < 0) {

#else
			if (mknod(to, fromstat.st_mode, (dev_t)0) < 0) {
#endif
#else
			seterrno(EINVAL);
			if (1) {
#endif
#endif
				errmsg(_("Could not make fifo '%s'.\n"), to);
				return (-1);
			}
			break;
#endif	/* S_IFIFO */
#ifdef	S_IFLNK
		case S_IFLNK:
			if (copy_link(from, to) < 0) {
				return (-1);
			}
			break;
#endif
#ifdef	S_IFSOCK
		case S_IFSOCK:
			errmsgno(EX_BAD, _("Cannot copy socket '%s'.\n"), from);
			return (-1);
#endif
		}
	}
	if (!is_dir(&fromstat) || !recurse)
		set_access(&fromstat, to, to_exists);
	if (verbose)
		error(_("Copied '%s' to '%s'.\n"), from, to);
	fflush(stderr);
	if (is_dir(&fromstat) && recurse) {
		int	ret;

		ret = do_recurse(from, to);
		set_access(&fromstat, to, to_exists);
		return (ret);
	}
	return (0);
}

LOCAL int
copyfile(from, to, fromsize, disksize)
	char	*from;
	char	*to;
	off_t	fromsize;
	off_t	disksize;
{
	register int	fin;
	register int	fout;
	register int	cnt;
	register char	*bp = copybuf;
	register int	size;
	register off_t	newpos = 0;
		int	err = 0;
		int	serrno;
	STATBUF statbuf;
		BOOL	do_sparse = FALSE;

	if ((fin = open(from, O_RDONLY)) < 0) {
		errmsg(_("Cannot open '%s'.\n"), from);
		return (-1);
	}
	if ((fout = creat(to, 0666)) < 0) {
		/*
		 * If cannot stat, don't remove
		 */
		serrno = geterrno();
		if (lstat(to, &statbuf) >= 0 &&
		    interactive && yes(_("remove %s? "), to) &&
		    doremove(to) &&
		    (fout = creat(to, 0666)) >= 0)
			goto docopy;
		errmsgno(serrno, _("Cannot create '%s'.\n"), to);
		close(fin);
		return (-1);
	}
docopy:

#if	defined(SEEK_HOLE) && defined(SEEK_DATA)
	if (sparse_file(fin, fromsize)) {
		return (sparse_copy(fin, fout, from, to, fromsize));
	}
#endif

	if (sparseflag && (fromsize > disksize))
		do_sparse = TRUE;
	else if (force_hole)
		do_sparse = TRUE;
	size = do_sparse ? 512 : copybsize;

	while ((cnt = read(fin, bp, size)) > 0) {
		newpos += cnt;
		if (do_sparse && newpos < fromsize &&
					cmpbytes(bp, zeroblk, size) >= size) {
			if (lseek(fout, newpos, SEEK_SET) < 0) {
				err = geterrno();
				errmsgno(err,
					_("A seek error occurred on '%s'.\n"),
					to);
				cnt = 0;
				break;
			}
		} else if (write(fout, bp, cnt) != cnt) {
			err = geterrno();
			errmsgno(err, _("A write error occurred on '%s'.\n"),
					to);
			cnt = 0;
			break;
		}
	}
	if (cnt != 0)
		err = geterrno();

	close(fin);
	close(fout);
	if (cnt != 0)
		errmsgno(err, _("A read error occurred on '%s'.\n"), from);
	if (err || cnt != 0)
		return (-1);
	return (0);
}

#ifdef	S_IFLNK
LOCAL int
copy_link(from, to)
	char	*from;
	char	*to;
{
	int	cnt;

	if ((cnt = readlink(from, copybuf, copybsize)) < 0) {
		errmsg(_("Could not read symbolic link '%s'.\n"), from);
		return (-1);
	}
	copybuf[cnt] = '\0';
	if (symlink(copybuf, to) < 0) {
		errmsg(_("Could not make symbolic link '%s'.\n"), to);
		return (-1);
	}
	return (0);
}
#endif

LOCAL int
do_recurse(from, to)
	char	*from;
	char	*to;
{
	char cmdbuf[2*PATH_MAX + 80];

	if (snprintf(cmdbuf, sizeof (cmdbuf),
			"%s -is_recurse %s%s%s%s%s%s%s%s%s%s%s %s%s.* %s%s* %s",
			saved_av0(),
			query ? "-q " : "",
			interactive ? "-i " : "",
			verbose ? "-v " : "",
			Recurse ? "-R " : "",
			recurse ? "-r " : "",
			setown ? "-setowner " : "",
			setgrp ? "-setgrp " : "",
			olddate ? "-olddate " : "",
			setperm ? "-p " : "",
			sparseflag ? "-sparse " : "",
			force_hole ? "-force-hole " : "",
			from, PATH_DELIM_STR, from, PATH_DELIM_STR,
			to) >= sizeof (cmdbuf)) {
		etoolong(cmdbuf);
		return (-1);
	}
	return (system(cmdbuf));
}

/*
 * Check for same file ID (filesystem and inode number)
 */
LOCAL BOOL
samefile(sp1, sp2)
	STATBUF	*sp1;
	STATBUF	*sp2;
{
#ifdef	DEBUG
	error("file_dev(sp1): %d file_dev(sp2): %d\n",
				file_dev(sp1), file_dev(sp2));
	error("file_ino(sp1): %d file_ino(sp2): %d\n",
				file_ino(sp1), file_ino(sp2));
#endif
	return (file_dev(sp1) == file_dev(sp2) &&
		file_ino(sp1) == file_ino(sp2));
}

LOCAL void
set_access(fromstat, to, to_exists)
	STATBUF *fromstat;
	char	*to;
	BOOL	to_exists;
{
	int	id_ok = TRUE;

#if	defined(HAVE_CHOWN) || defined(HAVE_LCHOWN)
	if (uid == 0 && setown && lchown(to, fromstat->st_uid, -1) < 0) {
		errmsg(_("Unable to set owner of '%s'.\n"), to);
		id_ok = FALSE;
	}
	if (uid == 0 && setgrp && lchown(to, -1, fromstat->st_gid) < 0) {
		errmsg(_("Unable to set group of '%s'.\n"), to);
		id_ok = FALSE;
	}
#endif
	if (!is_link(fromstat)) {
		mode_t	omode = fromstat->st_mode;

		if (olddate && xutimes(to, fromstat) < 0)
			errmsg(_("Unable to set date of '%s'.\n"), to);

		if (!id_ok)
			omode &= ~(S_ISUID|S_ISGID);

		if ((setperm || ! to_exists) && chmod(to, omode) < 0)
			errmsg(_("Unable to set access modes of '%s'.\n"), to);
	}
}

LOCAL void
etoolong(name)
	char	*name;
{
	errmsgno(EX_BAD, _("Path name '%s' too long.\n"), name);
}

/* VARARGS1 */
#ifdef	PROTOTYPES
LOCAL BOOL
yes(char *form, ...)
#else
LOCAL BOOL
yes(form, va_alist)
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	char ansbuf[128];

#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	printf("%r", form, args);
	va_end(args);
	flush();
	getline(ansbuf, sizeof (ansbuf));
	if (streql(ansbuf, "y") || streql(ansbuf, "yes"))
		return (TRUE);
	else
		return (FALSE);
}

LOCAL BOOL
getbase(path, basenamep, bsize)
		char *path;
	register char *basenamep;
		size_t	bsize;
{
	register char *p;

#ifdef	tos
	if (strlen(path) > 1 && path[1] == ':')
		path += 2;
#endif
	for (p = &path[strlen(path)-1]; p > &path[0]; p--) {
#ifdef	tos
		if (*p == '\\') {
#else
		if (*p == '/') {
#endif
			p++;
			break;
		}
	}

	if (strlcpy(basenamep, p, bsize) >= bsize)
		return (FALSE);
	return (TRUE);
}

LOCAL void
mygetline(pstr, str, len)
	char	*pstr;
	char	*str;
	int	len;
{
	for (;;) {
		printf("%s", pstr);
		flush();
		getline(str, len);
#ifdef	tos
		if (strchr(str, '\\') && streql(getenv("SLASH"), "off")) {
#else
		if (strchr(str, '/') && streql(getenv("SLASH"), "off")) {
#endif
			error(_("restricted.\n"));
			continue;
		} else {
			break;
		}
	}
}

LOCAL int
xutimes(name, sp)
	char	*name;
	STATBUF	*sp;
{
	struct	timespec tp[2];

	tp[0].tv_sec = sp->st_atime;
	tp[1].tv_sec = sp->st_mtime;

	tp[0].tv_nsec = stat_ansecs(sp);
	tp[1].tv_nsec = stat_mnsecs(sp);

	return (utimens(name, tp));
}

LOCAL BOOL
doremove(name)
	char	*name;
{
	int	err;

	/*
	 * Only unlink non directories or empty directories
	 * XXX need to implement the -remove_recursive flag
	 */
	if (rmdir(name) < 0) {
		err = geterrno();
		if (err == EACCES)
			goto cannot;

#if	defined(__CYGWIN32__) || defined(__CYGWIN__)
		if (err == ENOTEMPTY) {
			/*
			 * Cygwin returns ENOTEMPTY if 'name'
			 * is not a dir.
			 * XXX Never do this on UNIX.
			 * XXX If you are root, you may unlink
			 * XXX even nonempty directories.
			 */
			err = ENOTDIR;
		}
#endif
		if (err == ENOTDIR) {
			if (unlink(name) < 0) {
				err = geterrno();
				goto cannot;
			}
		}
	}
	return (TRUE);
cannot:
	errmsgno(err, _("File '%s' not removed.\n"), name);
	return (FALSE);
}

#if	defined(SEEK_HOLE) && defined(SEEK_DATA)
LOCAL BOOL
sparse_file(fd, fsize)
	int	fd;
	off_t	fsize;
{
	off_t	pos;

	/*
	 * If we have been compiled on an OS that supports SEEK_HOLE but run
	 * on an OS that does not support SEEK_HOLE, we get EINVAL.
	 * If the underlying filesystem does not support the SEEK_HOLE call,
	 * we get ENOTSUP. In all other cases, we will get either the position
	 * of the first real hole in the file or statb.st_size in case the file
	 * definitely has no holes.
	 */
	pos = lseek(fd, (off_t)0, SEEK_HOLE);	/* Check for first hole	   */
	if (pos == (off_t)-1)			/* SEEK_HOLE not supported */
		return (FALSE);

	if (pos != 0)				/* Not at pos 0: seek back */
		(void) lseek(fd, (off_t)0, SEEK_SET);

	if (pos >= fsize)			/* Definitely not sparse */
		return (FALSE);
	return (TRUE);				/* Definitely sparse */
}

LOCAL int
sparse_copy(fin, fout, from, to, fsize)
	register int	fin;
	register int	fout;
		char	*from;
		char	*to;
		off_t	fsize;
{
		off_t	off = 0;
		off_t	data;
		off_t	hole;
	register off_t	newpos = 0;
		int	err = 0;
	register int	cnt;
	register int	size;
	register char	*bp = copybuf;

	for (;;) {
		data = lseek(fin, off, SEEK_DATA);
		if (data == (off_t)-1 || data > fsize)
			break;
		hole = lseek(fin, data, SEEK_HOLE);
		if (hole == (off_t)-1 || hole > fsize)
			break;

		size = copybsize;
		newpos = data;
		if ((newpos + size) > hole)
			size = hole - newpos;

		if (lseek(fin, data, SEEK_SET) == (off_t)-1) {
			err = geterrno();
			errmsgno(err, _("A seek error occurred on '%s'.\n"),
					from);
			goto fail;
		}
		if (lseek(fout, data, SEEK_SET) == (off_t)-1) {
			err = geterrno();
			errmsgno(err, _("A seek error occurred on '%s'.\n"),
					to);
			goto fail;
		}
		cnt = 0;
		while ((newpos < hole) && (cnt = read(fin, bp, size)) > 0) {
			newpos += cnt;
			if (write(fout, bp, cnt) != cnt) {
				err = geterrno();
				errmsgno(err,
					_("A write error occurred on '%s'.\n"),
						to);
				cnt = 0;
				goto fail;
			}
			size = copybsize;
			if ((newpos + size) > hole)
				size = hole - newpos;
		}
		if (cnt < 0) {
			err = geterrno();
			errmsgno(err,
				_("A read error occurred on '%s'.\n"),
				from);
			goto fail;
		}
		if (cnt == 0) {
			errmsgno(EX_BAD, _("File '%s' shrunk.\n"), from);
			err = ENDOFFILE;
			goto fail;
		}
		off = hole;	/* Start for next data chunk */
	}

	if (newpos < fsize) {
#ifdef	HAVE_FTRUNCATE
		/*
		 * In order to prevent Solaris from allocating space at the end
		 * of the file we need to shrink the file. For this reason, we
		 * first create the file a bit too large.
		 */
		off = fsize;

#ifdef	_PC_MIN_HOLE_SIZE
		size = fpathconf(fout, _PC_MIN_HOLE_SIZE);
#else
		size = 0;
#endif
		if (size <= 0)
			size = 8192;

		if ((OFF_T_MAX - off) > size)
			(void) ftruncate(fout, off+size);

		if (ftruncate(fout, off) < 0)
			err = write_end_hole(fout, to, fsize);
#else
		err = write_end_hole(fout, to, fsize);
#endif
	}

fail:
	close(fin);
	close(fout);
	if (err)
		return (-1);
	return (0);
}

LOCAL int
write_end_hole(fout, to, fsize)
	int	fout;
	char	*to;
	off_t	fsize;
{
	int	err = 0;

	if (lseek(fout, fsize-1, SEEK_SET) == (off_t)-1) {
		err = geterrno();
	} else if (write(fout, "", 1) != 1) {
		err = geterrno();
	}
	if (err)
		errmsgno(err, _("A seek error occurred on '%s'.\n"), to);
	return (err);
}
#endif
