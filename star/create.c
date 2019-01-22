/* @(#)create.c	1.155 19/01/22 Copyright 1985, 1995, 2001-2019 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)create.c	1.155 19/01/22 Copyright 1985, 1995, 2001-2019 J. Schilling";
#endif
/*
 *	Copyright (c) 1985, 1995, 2001-2019 J. Schilling
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
#include "star.h"
#include "props.h"
#include "table.h"
#include <schily/errno.h>	/* XXX seterrno() is better JS */
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/dirent.h>
#include <schily/string.h>
#include <schily/jmpdefs.h>	/* For __fjmalloc() */
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include <schily/idcache.h>
#include "restore.h"
#ifdef	USE_FIND
#include <schily/stat.h>	/* Fuer stat_to_info() in starsubs.h */
#include <schily/walk.h>
#include <schily/find.h>
#endif
#include "starsubs.h"
#include "checkerr.h"
#include "fifo.h"
#include <schily/fetchdir.h>

#ifdef	USE_ACL

#ifdef	OWN_ACLTEXT
#if	defined(UNIXWARE) && defined(HAVE_ACL)
#define	HAVE_SUN_ACL
#define	HAVE_ANY_ACL
#endif
#endif
/*
 * HAVE_ANY_ACL currently includes HAVE_POSIX_ACL and HAVE_SUN_ACL.
 * This definition must be in sync with the definition in acl_unix.c
 * As USE_ACL is used in star.h, we are not allowed to change the
 * value of USE_ACL before we did include star.h or we may not include
 * star.h at all.
 * HAVE_HP_ACL is currently not included in HAVE_ANY_ACL.
 */
#ifndef	HAVE_ANY_ACL
#undef	USE_ACL		/* Do not try to get or set ACLs */
#endif
#endif

struct pdirs {
	struct pdirs	*p_last;
	dev_t		p_dev;
	ino_t		p_ino;
};

typedef	struct	lname {
	struct	lname	*l_lnext;	/* Next in list */
		char	l_lname[1];	/* actually longer */
} LNAME;

typedef	struct	links {
	struct	links	*l_next;	/* Next in list */
	struct	lname	*l_lnames;	/* Link names for SVr4 cpio */
		ino_t	l_ino;		/* Inode number (st_ino) */
		dev_t	l_dev;		/* Filesys number (st_dev) */
		Ulong	l_linkno;	/* Link serial number in compl. list */
		long	l_nlink;	/* Link count for file */
		short	l_namlen;	/* strlen(l_name) */
		Uchar	l_flags;	/* Flags (see below) */
		char	l_name[1];	/* actually longer */
} LINKS;

#define	L_ISDIR		1		/* This entry refers to a directory  */
#define	L_ISLDIR	2		/* A dir, hard linked to another dir */
#define	L_DATA		4		/* SVr4 cpio file data encountered   */

#define	L_HSIZE		256		/* must be a power of two */

#define	l_hash(info)	(((info)->f_ino + (info)->f_dev) & (L_HSIZE-1))

LOCAL	LINKS	*links[L_HSIZE];

extern	FILE	*vpr;
extern	FILE	*listf;

extern	BOOL	pkglist;
extern	BOOL	multivol;
extern	long	hdrtype;
extern	BOOL	tape_isreg;
extern	dev_t	tape_dev;
extern	ino_t	tape_ino;
#define	is_tape(info)		((info)->f_dev == tape_dev && (info)->f_ino == tape_ino)

extern	int	bufsize;
extern	char	*bigptr;

extern	BOOL	havepat;
extern	dev_t	curfs;
extern	Ullong	maxsize;
extern	struct timespec	Newer;
extern	Ullong	tsize;
extern	BOOL	prblockno;
extern	BOOL	debug;
extern	int	dumplevel;
extern	int	verbose;
extern	BOOL	silent;
extern	BOOL	readnull;
extern	BOOL	cflag;
extern	BOOL	uflag;
extern	BOOL	nodir;
extern	BOOL	acctime;
extern	BOOL	dirmode;
extern	BOOL	paxfollow;
extern	BOOL	doacl;
extern	BOOL	nodesc;
extern	BOOL	nomount;
extern	BOOL	interactive;
extern	BOOL	nospec;
extern	int	Fflag;
extern	BOOL	abs_path;
extern	BOOL	nowarn;
extern	BOOL	match_tree;
extern	BOOL	force_hole;
extern	BOOL	sparse;
extern	BOOL	Ctime;
extern	BOOL	nodump;
extern	BOOL	nullout;
extern	BOOL	linkdata;
extern	BOOL	link_dirs;
extern	BOOL	dodump;
extern	BOOL	dometa;
extern	BOOL	dumpmeta;
extern	BOOL	lowmem;
extern	BOOL	do_subst;

extern	int	intr;

/*
 * Variables that control overwriting the stat() struct members
 * controlled by the pkglist= option.
 */
LOCAL	BOOL	statmod = FALSE;
LOCAL	uid_t	statuid = _BAD_UID;
LOCAL	gid_t	statgid = _BAD_GID;
LOCAL	mode_t	statmode = _BAD_MODE;

EXPORT	void	checklinks	__PR((void));
LOCAL	int	take_file	__PR((char *name, FINFO *info));
EXPORT	int	_fileread	__PR((int *fp, void *buf, int len));
EXPORT	void	create		__PR((char *name, BOOL Hflag, BOOL forceadd));
LOCAL	void	createi		__PR((char *sname, char *name, int namlen,
					FINFO *info, pathstore_t *pathp,
					struct pdirs *last));
EXPORT	void	createlist	__PR((void));
LOCAL	BOOL	get_metainfo	__PR((char *line));
EXPORT	BOOL	read_symlink	__PR((char *sname, char *name, FINFO *info, TCB *ptb));
LOCAL	LINKS	*find_link	__PR((FINFO *info));
EXPORT	BOOL	last_cpio_link	__PR((FINFO *info));
EXPORT	BOOL	xcpio_link	__PR((FINFO *info));
LOCAL	BOOL	acpio_link	__PR((FINFO *info));
EXPORT	void	flushlinks	__PR((void));
LOCAL	void	flush_link	__PR((LINKS *lp));
EXPORT	BOOL	read_link	__PR((char *name, int namlen, FINFO *info,
								TCB *ptb));
LOCAL	int	nullread	__PR((void *vp, char *cp, int amt));
EXPORT	void	put_file	__PR((int *fp, FINFO *info));
EXPORT	void	cr_file		__PR((FINFO *info,
					int (*)(void *, char *, int),
					void *arg, int amt, char *text));
LOCAL	void	put_dir		__PR((char *sname, char *dname, int namlen,
					FINFO *info, TCB *ptb,
					pathstore_t *pathp, struct pdirs *last));
LOCAL	void	toolong		__PR((char *dname, char	*name, size_t len));
LOCAL	BOOL	checkdirexclude	__PR((char *sname, char *name, int namlen, FINFO *info));
LOCAL	BOOL	checkexclude	__PR((char *sname, char *name, int namlen, FINFO *info));

#ifdef	USE_FIND
EXPORT	int	walkfunc	__PR((char *nm, struct stat *fs, int type, struct WALK *state));
#endif

EXPORT void
checklinks()
{
	register LINKS	*lp;
	register int	i;
	register int	used	= 0;
	register int	curlen;
	register int	maxlen	= 0;
	register int	nlinks	= 0;
	register int	ndirs	= 0;
	register int	nldirs	= 0;

	for (i = 0; i < L_HSIZE; i++) {
		if (links[i] == (LINKS *)NULL)
			continue;

		curlen = 0;
		used++;

		for (lp = links[i]; lp != (LINKS *)NULL; lp = lp->l_next) {
			curlen++;
			nlinks++;
			if ((lp->l_flags & L_ISDIR) != 0) {
				ndirs++;
				if ((lp->l_flags & L_ISLDIR) != 0)
					nldirs++;
			} else if (lp->l_nlink != 0) {
				/*
				 * The fact that UNIX uses '.' and '..' as hard
				 * links to directories on all known file
				 * systems is a design bug. It makes it hard to
				 * find hard links to directories. Note that
				 * POSIX neither requires '.' and '..' to be
				 * implemented as hard links nor that these
				 * directories are physical present in the
				 * directory content.
				 * As it is hard to find all links (we would
				 * need to stat all directories as well as all
				 * '.' and '..' entries, we only warn for non
				 * directories.
				 */
				if (cflag &&
				    !errhidden(E_MISSLINK, lp->l_name)) {
					if (!errwarnonly(E_MISSLINK, lp->l_name))
						xstats.s_misslinks++;
					errmsgno(EX_BAD,
						"Missing links to '%s'.\n",
								lp->l_name);
					(void) errabort(E_MISSLINK, lp->l_name,
									TRUE);
				}
			}
		}
		if (maxlen < curlen)
			maxlen = curlen;
	}
	if (debug) {
		if (link_dirs) {
			errmsgno(EX_BAD, "entries: %d hashents: %d/%d maxlen: %d\n",
						nlinks, used, L_HSIZE, maxlen);
			errmsgno(EX_BAD, "hardlinks total: %d linked dirs: %d/%d linked files: %d \n",
						nlinks+nldirs-ndirs, nldirs, ndirs, nlinks-ndirs);
		} else {
			errmsgno(EX_BAD, "hardlinks: %d hashents: %d/%d maxlen: %d\n",
						nlinks, used, L_HSIZE, maxlen);
		}
	}
}

/*
 * Returns:
 *	TRUE	take file
 *	FALSE	do not take file
 *	-1	pattern did not match
 */
LOCAL int
take_file(name, info)
	register char	*name;
	register FINFO	*info;
{
	if ((info->f_flags & F_FORCE_ADD) != 0)
		return (TRUE);
	if (nodump && (info->f_flags & F_NODUMP) != 0)
		return (FALSE);

	if (havepat && !match(name)) {
		return (-1);
			/* Bei Directories ist f_size == 0 */
	} else if (maxsize && info->f_size > maxsize) {
		return (FALSE);
	} else if (dumplevel > 0) {
		/*
		 * For now, we cannot reliably deal with sub-second granularity
		 * on all platforms. For this reason, we let some files be on
		 * two incrementals to make sure not to miss them completely.
		 */
		if (info->f_mtime >= Newer.tv_sec) {
			/* EMPTY */
			;
		} else if (info->f_ctime >= Newer.tv_sec) {
			if (dumpmeta)
				info->f_xftype = XT_META;
		} else {
			return (FALSE);
		}

	} else if (Newer.tv_sec && (Ctime ? info->f_ctime:info->f_mtime) <=
								Newer.tv_sec) {
		/*
		 * First the easy case: Don't take file if seconds are
		 * less than in the reference file.
		 */
		if ((Ctime ? info->f_ctime:info->f_mtime) < Newer.tv_sec)
			return (FALSE);
		/*
		 * If we do not have nanoseconds, we are done.
		 */
		if ((info->f_flags & F_NSECS) == 0)
			return (FALSE);
		/*
		 * Here we know that we have nanoseconds and that seconds
		 * are equal, so finally check nanoseconds.
		 */
		if ((Ctime ? info->f_cnsec:info->f_mnsec) <= Newer.tv_nsec)
			return (FALSE);
	} else if (uflag && !update_newer(info)) {
		return (FALSE);
	} else if (!multivol &&
		    tsize > 0 && tsize < (tarblocks(info->f_size)+1+2)) {
		if (!errhidden(E_FILETOOBIG, name)) {
			if (!errwarnonly(E_FILETOOBIG, name))
				xstats.s_toobig++;
			errmsgno(EX_BAD,
			"'%s' does not fit on tape. Not dumped.\n",
								name);
			(void) errabort(E_FILETOOBIG, name, TRUE);
		}
		return (FALSE);
	} else if (props.pr_maxsize > 0 && info->f_size > props.pr_maxsize) {
		if (!errhidden(E_FILETOOBIG, name)) {
			if (!errwarnonly(E_FILETOOBIG, name))
				xstats.s_toobig++;
			errmsgno(EX_BAD,
			"'%s' file too big for current mode. Not dumped.\n",
								name);
			(void) errabort(E_FILETOOBIG, name, TRUE);
		}
		return (FALSE);
	} else if (pr_unsuptype(info)) {
		if (!errhidden(E_SPECIALFILE, name)) {
			if (!errwarnonly(E_SPECIALFILE, name))
				xstats.s_isspecial++;
			errmsgno(EX_BAD,
			"'%s' unsupported file type '%s'. Not dumped.\n",
				name,  XTTONAME(info->f_xftype));
			(void) errabort(E_SPECIALFILE, name, TRUE);
		}
		return (FALSE);
	} else if (is_special(info) && nospec) {
		if (!errhidden(E_SPECIALFILE, name)) {
			if (!errwarnonly(E_SPECIALFILE, name))
				xstats.s_isspecial++;
			errmsgno(EX_BAD,
			"'%s' is not a file. Not dumped.\n", name);
			(void) errabort(E_SPECIALFILE, name, TRUE);
		}
		return (FALSE);
	} else if (tape_isreg && is_tape(info)) {
		errmsgno(EX_BAD, "'%s' is the archive. Not dumped.\n", name);
		return (FALSE);
	}
	if (is_file(info) && dometa) {
		/*
		 * This is the right place for this code although it does not
		 * look correct. Later in star-1.5 we decide here, based on
		 * mtime and ctime of the file, whether we archive a file at
		 * all and whether we only add the file's metadata.
		 */
		info->f_xftype = XT_META;
		if (pr_unsuptype(info)) {
			if (!errhidden(E_SPECIALFILE, name)) {
				if (!errwarnonly(E_SPECIALFILE, name))
					xstats.s_isspecial++;
				errmsgno(EX_BAD,
				"'%s' unsupported file type '%s'. Not dumped.\n",
				name,  XTTONAME(info->f_xftype));
				(void) errabort(E_SPECIALFILE, name, TRUE);
			}
			return (FALSE);
		}
	}
#ifdef	USE_ACL
	/*
	 * If we return (FALSE) here, the file would not be archived at all.
	 * This is not what we want, so ignore return code from get_acls().
	 */
	if (doacl)
		(void) get_acls(info);
#endif  /* USE_ACL */
	return (TRUE);
}

int
_fileread(fp, buf, len)
	register int	*fp;
	void	*buf;
	int	len;
{
	register int	fd = *fp;
	register int	ret;
		int	errcnt = 0;

retry:
	while ((ret = read(fd, buf, len)) < 0 && geterrno() == EINTR)
		/* LINTED */
		;
	if (ret < 0 && geterrno() == EINVAL && ++errcnt < 100) {
		off_t oo;
		off_t si;

		/*
		 * Work around the problem that we cannot read()
		 * if the buffer crosses 2 GB in non large file mode.
		 */
		oo = lseek(fd, (off_t)0, SEEK_CUR);
		if (oo == (off_t)-1)
			return (ret);
		si = lseek(fd, (off_t)0, SEEK_END);
		if (si == (off_t)-1)
			return (ret);
		if (lseek(fd, oo, SEEK_SET) == (off_t)-1)
			return (ret);
		if (oo >= si) {	/* EOF */
			ret = 0;
		} else if ((si - oo) <= len) {
			len = si - oo;
			goto retry;
		}
	}
	return (ret);
}

EXPORT void
create(name, Hflag, forceadd)
	register char	*name;
		BOOL	Hflag;
		BOOL	forceadd;
{
		FINFO	finfo;
	register FINFO	*info	= &finfo;
		BOOL	opaxfollow = paxfollow;	/* paxfollow supersedes follow */

	if (name[0] == '.' && name[1] == '/') {
		for (name++; name[0] == '/'; name++)
			/* LINTED */
			;
	}
	if (name[0] == '\0')
		name = ".";
	if (Hflag)
		paxfollow = Hflag;
	if (!getinfo(name, info)) {
		paxfollow = opaxfollow;
		if (!errhidden(E_STAT, name)) {
			if (!errwarnonly(E_STAT, name))
				xstats.s_staterrs++;
			errmsg("Cannot stat '%s'.\n", name);
			(void) errabort(E_STAT, name, TRUE);
		}
		return;
	}
	if (forceadd)
		info->f_flags |= F_FORCE_ADD;
	paxfollow = opaxfollow;
	createi(name, name, strlen(name), info,
		(pathstore_t *)0, (struct pdirs *)0);
}

LOCAL void
createi(sname, name, namlen, info, pathp, last)
		char	*sname;
	register char	*name;
		int	namlen;
	register FINFO	*info;
		pathstore_t *pathp;
		struct pdirs *last;
{
		char	lname[PATH_MAX+1];  /* This limit cannot be overruled */
		TCB	tb;
	register TCB	*ptb		= &tb;
		int	fd		= -1;
		BOOL	was_link	= FALSE;
		BOOL	do_sparse	= FALSE;

	if (hash_xlookup(name))
		return;

	info->f_sname = sname;
	info->f_name = name;	/* XXX Das ist auch in getinfo !!!?!!! */
	info->f_namelen = namlen;
	if (Fflag > 0 && !checkexclude(sname, name, namlen, info))
		return;

#ifdef	nonono_NICHT_BEI_CREATE	/* XXX */
	if (!abs_path &&	/* XXX VVV siehe skip_slash() */
		(info->f_name[0] == '/' /* || info->f_lname[0] == '/' */)) {
		skip_slash(info);
		info->f_namelen -= info->f_name - name;
		if (info->f_namelen == 0) {
			info->f_name = "./";
			info->f_namelen = 2;
		}
		/* XXX das gleiche mit f_lname !!!!! */
	}
#endif	/* nonono_NICHT_BEI_CREATE	XXX */
	info->f_lname = lname;	/* XXX nur Übergangsweise!!!!! */
	info->f_lnamelen = 0;

	if (prblockno)
		(void) tblocks();		/* set curblockno */

	if (do_subst && subst(info)) {
		if (info->f_name[0] == '\0') {
			if (verbose)
			fgtprintf(vpr,
				"'%s' substitutes to null string, skipping ...\n",
							name);
			return;
		}
	}

	if (statmod) {
		if (statmode != _BAD_MODE)
			info->f_mode = statmode;
		if (statuid != _BAD_UID)
			info->f_uid = statuid;
		if (statgid != _BAD_GID)
			info->f_gid = statgid;
	}

	if (verbose <= 1 &&
	    !(dirmode && is_dir(info)) &&
				(info->f_namelen <= props.pr_maxsname)) {
		/*
		 * Allocate TCB from the buffer to avoid copying TCB
		 * in the most frequent case.
		 * If we are writing directories after the files they
		 * contain, we cannot allocate the space for tcb
		 * from the buffer.
		 * With very long names we will have to write out
		 * other data before we can write the TCB, so we cannot
		 * alloc tcb from buffer too.
		 * If we are creating a long listing while archiving, the
		 * TCB will be overwritten by info_to_xhdr() and this would
		 * overwrite username/groupname that is later needed for
		 * vprint(), so we cannot allocate TCB from the buffer here.
		 */
		if ((ptb = (TCB *)get_block(props.pr_hdrsize)) == NULL)
			ptb = &tb;
		else
			info->f_flags |= F_TCB_BUF;
	}
	info->f_tcb = ptb;
	if ((props.pr_flags & PR_CPIO) == 0)
		filltcb(ptb);
	if (!name_to_tcb(info, ptb))	/* Name too long */
		return;

#ifndef	__PRE_CPIO__
	if (is_symlink(info) && !read_symlink(sname, name, info, ptb)) {
		return;
	}
#endif
	info_to_tcb(info, ptb);
	if (is_dir(info)) {
		/*
		 * If we have been requested to check for hard linked
		 * directories, first look for possible hard links.
		 */
		if (link_dirs && /* info->f_nlink > 1 && */ read_link(name, namlen, info, ptb))
			was_link = TRUE;

		if (was_link && !is_link(info))	/* link name too long */
			return;

		if (was_link && take_file(name, info) > 0) {
			put_tcb(ptb, info);
			vprint(info);
		} else {
			put_dir(sname, name, namlen, info, ptb, pathp, last);
		}
	} else if (take_file(name, info) <= 0) {	/* < TRUE */
		return;
	} else if (interactive && !ia_change(ptb, info)) {
		fgtprintf(vpr, "Skipping ...\n");
#ifdef	__PRE_CPIO__
	} else if (is_symlink(info) && !read_symlink(sname, name, info, ptb)) {
		/* EMPTY */
		;
#endif
	} else if (is_meta(info)) {
		/*
		 * XXX Currently only TAR supports meta files.
		 * XXX If we ever change this, we may need to remove the
		 * XXX ptb->dbuf references here.
		 */
		if (info->f_nlink > 1 && read_link(name, namlen, info, ptb))
			was_link = TRUE;

		if (was_link && !is_link(info))	/* link name too long */
			return;

		if (!was_link) {
			/*
			 * XXX We definitely do not want that other tar
			 * XXX implementations are able to read tar archives
			 * XXX that contain meta files.
			 * XXX If a tar implementation that does not understand
			 * XXX meta files extracts archives with meta files,
			 * XXX it will most likely destroy old files on disk.
			 */
			ptb->dbuf.t_linkflag = LF_META;
			info->f_flags &= ~F_SPLIT_NAME;
			if (ptb->dbuf.t_prefix[0] != '\0')
				fillbytes(ptb->dbuf.t_prefix, props.pr_maxprefix, '\0');
			if (props.pr_flags & PR_XHDR)
				info->f_xflags |= XF_PATH;
			else
				info->f_flags |= F_LONGNAME;
			ptb->dbuf.t_name[0] = 0;	/* Hide P-1988 name */
			info_to_tcb(info, ptb);
		}
		put_tcb(ptb, info);
		vprint(info);
		return;

	} else if (is_file(info) && info->f_size != 0 && !nullout &&
				(fd = _lfileopen(sname, "rb")) < 0) {
		if (!errhidden(E_OPEN, name)) {
			if (!errwarnonly(E_OPEN, name))
				xstats.s_openerrs++;
			errmsg("Cannot open '%s'.\n", name);
			(void) errabort(E_OPEN, name, TRUE);
		}
	} else {
		if (info->f_nlink > 1 && read_link(name, namlen, info, ptb) &&
		    !linkdata) {
			was_link = TRUE;
		}
		if (was_link && !is_link(info))	{ /* link name too long */
			if (fd >= 0)
				close(fd);
			return;
		}
		/*
		 * Special treatment for the idiosyncratic way of dealing with
		 * hard links in the SVr4 CRC cpio archive format.
		 * The link count is handled by calling read_link() in
		 * cpiotcb_to_info() before.
		 */
		if ((props.pr_flags & PR_SV_CPIO_LINKS) != 0 &&
		    info->f_nlink > 1) {
			if (!last_cpio_link(info)) {	/* Ign. all but last */
				if (fd >= 0)
					close(fd);
				return;
			}
			if (acpio_link(info)) {		/* Now extract all   */
				put_file(&fd, info);
				goto out;
			}
			/*
			 * If the link count is increasing, do it the same way
			 * as SVr4 cpio and archive the rest as plain files.
			 */
			if (is_file(info))
				info->f_rsize = info->f_size;
			info->f_xftype = info->f_rxftype;
		}

		if (!is_file(info) || was_link || info->f_rsize == 0) {
			/*
			 * Don't dump the content of hardlinks and empty files.
			 * Hardlinks currently have f_rsize == 0 !
			 */
			put_tcb(ptb, info);
			vprint(info);
			if (fd >= 0)
				close(fd);
			return;
		}

		/*
		 * In case we like to do sparse file handling via SEEK_HOLE we need
		 * an open fd in order to check for a sparse file.
		 */
		do_sparse = sparse && (props.pr_flags & PR_SPARSE);
		if (do_sparse && nullout &&
				(fd = _lfileopen(sname, "rb")) < 0) {
			if (!errhidden(E_OPEN, name)) {
				if (!errwarnonly(E_OPEN, name))
					xstats.s_openerrs++;
				errmsg("Cannot open '%s'.\n", name);
				(void) errabort(E_OPEN, name, TRUE);
			}
			return;
		}

		if (do_sparse && sparse_file(&fd, info)) {
			if (!silent)
				error("'%s' is sparse\n", info->f_name);
			put_sparse(&fd, info);
		} else if (do_sparse && force_hole) {
			/*
			 * Treat all files as sparse when -force-hole
			 * option is given. Files that do not contain
			 * any zeroed region will however still be
			 * archived as plain files by put_sparse().
			 */
			put_sparse(&fd, info);
		} else {
			put_tcb(ptb, info);
			vprint(info);
			put_file(&fd, info);
		}
		/*
		 * Reset access time of file.
		 * This is important when using star for dumps.
		 * N.B. this has been done after fclose()
		 * before _FIOSATIME has been used.
		 *
		 * If f == NULL, the file has not been accessed for read
		 * and access time need not be reset.
		 */
out:
		if (acctime && fd >= 0)
			rs_acctime(fd, info);
		if (fd >= 0)
			close(fd);
	}
}

EXPORT void
createlist()
{
	register int	nlen;
		char	*name;
		size_t	nsize = PATH_MAX;

	/*
	 * We need at least PATH_MAX+1 and add 512 to get better messages below
	 */
	name = ___malloc(nsize, "name buffer");

	for (nlen = 1; nlen >= 0; ) {
		if ((nlen = getdelim(&name, &nsize,
				readnull ? '\0' : '\n', listf)) < 0)
			break;

		if (nlen == 0)
			continue;
		if (!readnull) {
			if (name[nlen-1] == '\n')
				name[--nlen] = '\0';
			if (nlen == 0)
				continue;
		}
		if (pkglist) {
			if (!get_metainfo(name))
				continue;
			nlen = strlen(name);
		}
		if (intr)
			break;
		curfs = NODEV;
		create(name, FALSE, FALSE); /* XXX Liste doch wie Kommandozeile? */
	}
	free(name);
}

LOCAL BOOL
get_metainfo(line)
	char	*line;
{
	char	*p;
	char	*p2;
	uid_t	uid;
	gid_t	gid;
	long	lid;

	p = strchr(line, ' ');
	if (p == NULL) {
		statmod = FALSE;
		return (TRUE);
	}

	*p++ = '\0';
	statmod = TRUE;
	statmode = _BAD_MODE;
	statuid = _BAD_UID;
	statgid = _BAD_GID;
	if (*p == '?') {
		p2 = ++p;
	} else {
		p2 = astolb(p, &lid, 8);
		if (*p2 == ' ')
			statmode = lid;
	}
	if (*p2 == ' ') {
		p = ++p2;
	} else {
		errmsgno(EX_BAD, "%s: illegal mode\n", line);
		return (FALSE);
	}
	p2 = strchr(p, ' ');
	if (p2 != NULL) {
		*p2++ = '\0';
	} else {
		errmsgno(EX_BAD, "%s: illegal uid\n", line);
		return (FALSE);
	}
	if (!streql(p, "?") && ic_uidname(p, strlen(p), &uid))
		statuid = uid;
	p = p2;
	if (!streql(p, "?") && ic_gidname(p, strlen(p), &gid))
		statgid = gid;

	return (TRUE);
}

EXPORT BOOL
read_symlink(sname, name, info, ptb)
	char	*sname;
	char	*name;
	register FINFO	*info;
	TCB	*ptb;
{
	int	len;

	info->f_lname[0] = '\0';

#ifdef	HAVE_READLINK
	if ((len = lreadlink(sname, info->f_lname, PATH_MAX)) < 0) {
		if (!errhidden(E_READLINK, name)) {
			if (!errwarnonly(E_READLINK, name))
				xstats.s_rwerrs++;
			errmsg("Cannot read link '%s'.\n", name);
			(void) errabort(E_READLINK, name, TRUE);
		}
		return (FALSE);
	}
	info->f_lnamelen = len;
	/*
	 * string from readlink is not null terminated
	 */
	info->f_lname[len] = '\0';

	if (len > props.pr_maxlnamelen) {
		if (!errhidden(E_NAMETOOLONG, name)) {
			if (!errwarnonly(E_NAMETOOLONG, name))
				xstats.s_toolong++;
			errmsgno(EX_BAD,
			"%s: Symbolic link too long.\n", name);
			(void) errabort(E_NAMETOOLONG, name, TRUE);
		}
		return (FALSE);
	}
	if ((props.pr_flags & PR_CPIO) != 0)
		return (TRUE);

	if (len > props.pr_maxslname) {
		if (props.pr_flags & PR_XHDR)
			info->f_xflags |= XF_LINKPATH;
		else
			info->f_flags |= F_LONGLINK;
	}
	/*
	 * if linkname is not longer than props.pr_maxslname
	 * that's all to do with linkname
	 */
	strncpy(ptb->dbuf.t_linkname, info->f_lname, props.pr_maxslname);
	return (TRUE);
#else
	if (!errhidden(E_SPECIALFILE, name)) {
		if (!errwarnonly(E_SPECIALFILE, name))
			xstats.s_isspecial++;
		errmsgno(EX_BAD,
		"'%s' unsupported file type '%s'. Not dumped.\n",
				name,  XTTONAME(info->f_xftype));
		(void) errabort(E_SPECIALFILE, name, TRUE);
	}
	return (FALSE);
#endif
}

LOCAL LINKS *
find_link(info)
	register FINFO	*info;
{
	register LINKS	*lp;

	lp = links[l_hash(info)];

	for (; lp != (LINKS *)NULL; lp = lp->l_next) {
		if (lp->l_ino == info->f_ino && lp->l_dev == info->f_dev)
			return (lp);
	}
	return ((LINKS *)NULL);
}

/*
 * Return TRUE in case we found the last entry (the one that may have data)
 * for a specific file.
 */
EXPORT BOOL
last_cpio_link(info)
	register FINFO	*info;
{
	register LINKS	*lp;

	if ((lp = find_link(info)) != NULL) {
		if ((!cflag && info->f_size > 0) ||
		    lp->l_nlink <= 0 ||
		    (lp->l_flags & L_DATA) != 0)
			return (TRUE);
		return (FALSE);
	}
	return (TRUE);
}

/*
 * Extract all cpio CRC links.
 * XXX This should be in extract.c
 */
EXPORT BOOL
xcpio_link(info)
	register FINFO	*info;
{
	register LINKS	*lp;
		char	*name = info->f_name;
		char	*lname = info->f_lname;
		off_t	rsize = info->f_rsize;
		int	xftype = info->f_xftype;

	if ((lp = find_link(info)) != NULL) {
		/*
		 * We come here if the link count increases and we need to
		 * archive more "files" as expected.
		 */
		if ((lp->l_flags & L_DATA) != 0)
			return (FALSE);
	}
	info->f_xftype = info->f_rxftype;
	extracti(info, NULL);			/* Extract as real node */
	info->f_xftype = xftype;
	info->f_rsize = 0;

	if (lp != NULL) {
		register LNAME	*ln;

		lp->l_flags |= L_DATA;
		info->f_lname = name;
		info->f_name = lp->l_name;
		extracti(info, NULL);		/* Extract link info->f_lname */

		for (ln = lp->l_lnames; ln; ln = ln->l_lnext) {
			if (streql(name, ln->l_lname))
				continue;
			info->f_name = ln->l_lname;
			extracti(info, NULL);	/* Extract all other links */
		}
	}
	info->f_name = name;
	info->f_lname = lname;
	info->f_rsize = rsize;
	return (TRUE);
}

/*
 * Archive all cpio CRC links.
 *
 * We are called if our caller believes that the last link for a specific file
 * has been encountered.
 */
LOCAL BOOL
acpio_link(info)
	register FINFO	*info;
{
		TCB	tb;
	register TCB	*ptb = info->f_tcb;
	register LINKS	*lp;
		int	namelen = info->f_namelen;
		char	*name = info->f_name;
		char	*lname = info->f_lname;
		off_t	size = info->f_size;	/* real size of file	    */
#ifdef	__what_people_would_expect__
		off_t	rsize = info->f_rsize;	/* size as archived on tape */
#endif

	if ((lp = find_link(info)) != NULL) {
		/*
		 * We come here if the link count increases and we need to
		 * archive more "files" than expected.
		 */
		if ((lp->l_flags & L_DATA) != 0)
			return (FALSE);
	}

	info->f_size = 0;
	info->f_rsize = 0;

	if (lp != NULL) {
		register LNAME	*ln;

		lp->l_flags |= L_DATA;
		info->f_lname = name;
		info->f_namelen = strlen(lp->l_name);
		info->f_name = lp->l_name;
		info_to_tcb(info, ptb);
		put_tcb(ptb, info);
		vprint(info);

		for (ln = lp->l_lnames; ln; ln = ln->l_lnext) {
			if (streql(name, ln->l_lname))
				continue;
			info->f_namelen = strlen(ln->l_lname);
			info->f_name = ln->l_lname;
			info->f_flags &= ~F_TCB_BUF;
			if ((ptb = (TCB *)get_block(props.pr_hdrsize)) == NULL)
				ptb = &tb;
			else
				info->f_flags |= F_TCB_BUF;
			info->f_tcb = ptb;
			info_to_tcb(info, ptb);
			put_tcb(ptb, info);
			vprint(info);
		}
	}
	info->f_namelen = namelen;
	info->f_name  = name;
	info->f_lname = lname;
	info->f_size  = size;
#ifdef	__what_people_would_expect__
	info->f_rsize = rsize;
#else
	info->f_rsize = size;	/* Achieve that the last link archives data */
#endif
	info->f_xftype = info->f_rxftype;

	info->f_flags &= ~F_TCB_BUF;
	if ((ptb = (TCB *)get_block(props.pr_hdrsize)) == NULL)
		ptb = &tb;
	else
		info->f_flags |= F_TCB_BUF;
	info->f_tcb = ptb;
	info_to_tcb(info, ptb);
	put_tcb(ptb, info);
	vprint(info);
	return (TRUE);
}

/*
 * Flush all SVr4 cpio -Hcrc links that have not yet been archived.
 */
EXPORT void
flushlinks()
{
	register LINKS	*lp;
	register int	i;

	if ((props.pr_flags & PR_SV_CPIO_LINKS) == 0)
		return;

	for (i = 0; i < L_HSIZE; i++) {
		if (links[i] == (LINKS *)NULL)
			continue;

		for (lp = links[i]; lp != (LINKS *)NULL; lp = lp->l_next) {
			if ((lp->l_flags & (L_ISDIR|L_DATA)) == 0 &&
			    (lp->l_nlink > 0))
				flush_link(lp);
		}
	}
}

/*
 * Flush a single cpio -Hcrc link.
 */
LOCAL void
flush_link(lp)
	register LINKS	*lp;
{
		TCB	tb;
		TCB	*ptb;
		FINFO	finfo;
	register LNAME	*ln;
		int	fd = 1;
		BOOL	did_stat;
		char	*name;

	finfo.f_flags = 0;
	finfo.f_flags &= ~F_TCB_BUF;
	if ((ptb = (TCB *)get_block(props.pr_hdrsize)) == NULL)
		ptb = &tb;
	else
		finfo.f_flags |= F_TCB_BUF;
	did_stat = getinfo(lp->l_name, &finfo);
	name = lp->l_name;
	for (ln = lp->l_lnames; ln; ln = ln->l_lnext) {
		if (!did_stat) {
			did_stat = getinfo(ln->l_lname, &finfo);
			if (did_stat)
				name = ln->l_lname;
		}
		fd++;
	}
	if (!did_stat) {
		if (!errhidden(E_STAT, lp->l_name)) {
			if (!errwarnonly(E_STAT, lp->l_name))
				xstats.s_staterrs++;
			errmsg("Cannot stat '%s'.\n", lp->l_name);
			(void) errabort(E_STAT, lp->l_name, TRUE);
		}
		return;
	}
	finfo.f_name = lp->l_name;
	finfo.f_namelen = strlen(lp->l_name);
	finfo.f_lname = "";	/* XXX nur Übergangsweise!!!!! */
	finfo.f_lnamelen = 0;

	finfo.f_nlink = fd;
	finfo.f_xftype = XT_LINK;
	finfo.f_tcb = ptb;
	fd = -1;
	if (is_file(&finfo) && finfo.f_size != 0 && !nullout &&
				(fd = _lfileopen(name, "rb")) < 0) {
		if (!errhidden(E_OPEN, name)) {
			if (!errwarnonly(E_OPEN, name))
				xstats.s_openerrs++;
			errmsg("Cannot open '%s'.\n", name);
			(void) errabort(E_OPEN, name, TRUE);
		}
		return;
	}
	if (acpio_link(&finfo))
		put_file(&fd, &finfo);

	if (acctime && fd >= 0)
		rs_acctime(fd, &finfo);
	if (fd >= 0)
		close(fd);
}

/*
 * Cheating with st_dev & st_ino for CPIO:
 *	Do not use inode number 0 and start st_dev from an
 *	obscure value...
 */
#define	dev_from_linkno(n)	(0x5555 + ((n) / 0xFFFF))
#define	ino_from_linkno(n)	(1 +	  ((n) % 0xFFFF))

EXPORT BOOL
read_link(name, namlen, info, ptb)
	char	*name;
	int	namlen;
	register FINFO	*info;
	TCB	*ptb;
{
	register LINKS	*lp;
	static	Ulong	linkno = 0;

	if ((lp = find_link(info)) != NULL) {
		if (lp->l_namlen > props.pr_maxlnamelen) {
			if (!errhidden(E_NAMETOOLONG, lp->l_name)) {
				if (!errwarnonly(E_NAMETOOLONG, lp->l_name))
					xstats.s_toolong++;
				errmsgno(EX_BAD,
				"%s: Link name too long.\n",
							lp->l_name);
				(void) errabort(E_NAMETOOLONG, lp->l_name,
								TRUE);
			}
			return (TRUE);
		}
		if (lp->l_namlen > props.pr_maxslname) {
			if (props.pr_flags & PR_XHDR)
				info->f_xflags |= XF_LINKPATH;
			else
				info->f_flags |= F_LONGLINK;
		}
		if (--lp->l_nlink < 0) {
			if (!nowarn && (info->f_flags & F_EXTRACT) == 0)
				errmsgno(EX_BAD,
				"%s: Linkcount below zero (%ld)\n",
					lp->l_name, lp->l_nlink);
		}
		/*
		 * We found a hard link to a directory that is already
		 * known in the link cache. Mark it for later
		 * statistical analysis.
		 */
		if (lp->l_flags & L_ISDIR)
			lp->l_flags |= L_ISLDIR;
		/*
		 * if linkname is not longer than props.pr_maxslname
		 * that's all to do with linkname
		 */
		if ((props.pr_flags & PR_CPIO) == 0) {
			strncpy(ptb->dbuf.t_linkname, lp->l_name,
						props.pr_maxslname);
		}
		info->f_lname = lp->l_name;
		info->f_lnamelen = lp->l_namlen;
		info->f_xftype = XT_LINK;

		/*
		 * With POSIX-1988, f_rsize is 0 for hardlinks
		 *
		 * XXX Should we add a property for old tar
		 * XXX compatibility to keep the size field as before?
		 */
		if (!linkdata)
			info->f_rsize = (off_t)0;
		/*
		 * XXX This is the wrong place but the TCB has already
		 * XXX been set up (including size field) before.
		 * XXX We only call info_to_tcb() to change size to 0.
		 * XXX There should be a better way to deal with TCB.
		 */
		if ((info->f_flags & F_EXTRACT) == 0) {
			/*
			 * XXX Nicht bei extrakt
			 */
			info->f_dev = dev_from_linkno(lp->l_linkno);
			info->f_ino = ino_from_linkno(lp->l_linkno);
			info_to_tcb(info, ptb);
			info->f_dev = lp->l_dev;
			info->f_ino = lp->l_ino;
		}
		/*
		 * XXX Dies ist eine ungewollte Referenz auf den
		 * XXX TAR Control Block, aber hier ist der TCB
		 * XXX schon fertig und wir wollen nur den Typ
		 * XXX Modifizieren.
		 */
		if ((props.pr_flags & PR_CPIO) == 0)
			ptb->dbuf.t_linkflag = LNKTYPE;
		if ((props.pr_flags & PR_SV_CPIO_LINKS) != 0) {
			register LNAME	*ln;

			ln = (LNAME *)malloc(sizeof (*ln)+namlen);
			if (ln == NULL) {
				errmsg(
				"Cannot alloc new link name for '%s'.\n",
					name);
			} else {
				strlcpy(ln->l_lname, name, namlen+1);
				ln->l_lnext = lp->l_lnames;
				lp->l_lnames = ln;
			}
		}
		return (TRUE);
	}
	if ((lp = (LINKS *)malloc(sizeof (*lp)+namlen)) == (LINKS *)NULL) {
		errmsg("Cannot alloc new link for '%s'.\n", name);
	} else {
		register LINKS	**lpp = &links[l_hash(info)];

		lp->l_next = *lpp;
		*lpp = lp;
		lp->l_lnames = NULL;
		lp->l_linkno = linkno++;
		lp->l_ino = info->f_ino;
		lp->l_dev = info->f_dev;
		lp->l_nlink = info->f_nlink - 1;
		lp->l_namlen = namlen;
		if (is_dir(info))
			lp->l_flags = L_ISDIR;
		else
			lp->l_flags = 0;
		strlcpy(lp->l_name, name, namlen+1);

		if ((info->f_flags & F_EXTRACT) == 0) {
			/*
			 * XXX Nicht bei extrakt
			 */
			info->f_dev = dev_from_linkno(lp->l_linkno);
			info->f_ino = ino_from_linkno(lp->l_linkno);
			info_to_tcb(info, ptb);
			info->f_dev = lp->l_dev;
			info->f_ino = lp->l_ino;
		}
	}
	return (FALSE);
}

/* ARGSUSED */
LOCAL int
nullread(vp, cp, amt)
	void	*vp;
	char	*cp;
	int	amt;
{
	return (amt);
}

EXPORT void
put_file(fp, info)
	register int	*fp;
	register FINFO	*info;
{
	if (nullout) {
		cr_file(info, (int(*)__PR((void *, char *, int)))nullread,
							fp, 0, "reading");
	} else {
		cr_file(info, (int(*)__PR((void *, char *, int)))_fileread,
							fp, 0, "reading");
	}
}

EXPORT void
cr_file(info, func, arg, amt, text)
		FINFO	*info;
		int	(*func) __PR((void *, char *, int));
	register void	*arg;
		int	amt;
		char	*text;
{
	register int	amount;
	register off_t	blocks;
	register off_t	size;
	register int	i = 0;
	register off_t	n;
extern	m_stats	*stats;

	size = info->f_rsize;

	fifo_enter_critical();
	stats->cur_size = size;
	stats->cur_off = 0;
	fifo_leave_critical();

	if ((blocks = tarblocks(info->f_rsize)) == 0)
		return;
	if (amt == 0)
		amt = bufsize;
	do {
		amount = buf_wait(TBLOCK);
		amount = min(amount, amt);

		if ((props.pr_flags & PR_CPIO) == 0) {
			if ((i = (*func)(arg, bigptr, max(amount, TBLOCK))) <= 0)
				break;
		} else {
			if ((i = (*func)(arg, bigptr, amount)) <= 0)
				break;
		}

		size -= i;
		fifo_enter_critical();
		stats->cur_off += i;
		fifo_leave_critical();

		if (size < 0) {			/* File increased in size */
			n = tarblocks(size+i);	/* use expected size only */
		} else {
			n = tarblocks(i);
		}
		if ((props.pr_flags & PR_CPIO) == 0) {

			if (i % TBLOCK) {	/* Clear (better compression) */
				fillbytes(bigptr+i, TBLOCK - (i%TBLOCK), '\0');
			}
			buf_wake(n*TBLOCK);
		} else {
			if (size < 0)		/* File increased in size */
				buf_wake(size+i); /* use expected size only */
			else
				buf_wake(i);
			n = blocks;
		}
	} while ((blocks -= n) >= 0 && i > 0 && size >= 0);
	if (i < 0) {
		if (!errhidden(E_READ, info->f_name)) {
			if (!errwarnonly(E_READ, info->f_name))
				xstats.s_rwerrs++;
			errmsg("Error %s '%s'.\n", text, info->f_name);
			(void) errabort(E_READ, info->f_name, TRUE);
		}
	} else if ((blocks != 0 || size != 0) && func != nullread) {
		if (!errhidden(size < 0 ? E_GROW:E_SHRINK, info->f_name)) {
			if (!errwarnonly(size < 0 ? E_GROW:E_SHRINK, info->f_name))
				xstats.s_sizeerrs++;
			errmsgno(EX_BAD,
			"'%s': file changed size (%s).\n",
			info->f_name, size < 0 ? "increased":"shrunk");
			(void) errabort(size < 0 ? E_GROW:E_SHRINK,
							info->f_name, TRUE);
		}
	}
	/*
	 * If the file did shrink, fill up to expected size...
	 */
	if ((props.pr_flags & PR_CPIO) == 0) {
		while (--blocks >= 0)
			writeempty();
	} else {
		while (size > 0) {
			amount = buf_wait(1);
			amount = min(amount, size);
			fillbytes(bigptr, amount, '\0');
			buf_wake(amount);
			size -= amount;
		}
	}
	/*
	 * Honour CPIO padding
	 */
	if ((amount = props.pr_pad) != 0) {
		size = info->f_rsize;
		if (info->f_flags & F_LONGNAME)
			size += props.pr_hdrsize;
		amount = (amount + 1 - (size & amount)) & amount;
		while (amount-- > 0) {
			buf_wait(1);
			*bigptr = '\0';
			buf_wake(1);
		}
	}
}

#define	newfs(i)	((i)->f_dev != curfs)

LOCAL void
put_dir(sname, dname, namlen, info, ptb, pathp, last)
		char	*sname;
	register char	*dname;
	register int	namlen;
	FINFO	*info;
	TCB	*ptb;
	pathstore_t	*pathp;
	struct pdirs	*last;
{
	static	int	depth	= -10;
	static	int	dinit	= 0;
		FINFO	nfinfo;
	register FINFO	*ninfo	= &nfinfo;
		DIR	*d = NULL;
	struct	dirent	*dir;
#ifdef	HAVE_SEEKDIR
		long	offset	= 0L;
#endif
	pathstore_t	path;
	register char	*name;
		int	xlen;
		BOOL	putdir = FALSE;
		BOOL	direrr = FALSE;
		int	dlen;
		char	*dp = NULL;
	struct pdirs	thisd;
	struct pdirs	*pd = last;
		BOOL	didslash = FALSE;

	if (!dinit) {
#ifdef	_SC_OPEN_MAX
		depth += sysconf(_SC_OPEN_MAX);
#else
		depth += getdtablesize();
#endif
		dinit = 1;
	}

	if (nodump && (info->f_flags & F_NODUMP) != 0)
		return;

	info->f_dir = NULL;
	info->f_dirinos = NULL;
	info->f_dirlen = 0;
	info->f_dirents = 0;

	switch (take_file(dname, info)) {

	case -1:
		if (match_tree)
			return;
		break;

	case TRUE:
		putdir = TRUE;
	}

	if ((!nodesc || dodump > 0) && (!nomount || !newfs(info))) {
		/*
		 * If this is a mounted directory and we have been called
		 * with -M, it makes no sense to include the directorie's file
		 * name list with -dump.
		 * By not including this list, we also avoid error messages
		 * like:
		 *	star: Permission denied. Cannot open 'opt/SUNWddk'.
		 *
		 * which is a result of an automounted directory.
		 *
		 * Conclusion: try to use a filesystem snapshot whenever
		 * possible.
		 */
		if (!lowmem) {
			ino_t	*ino = NULL;
			int	nents;

			d = lopendir(sname);
			if (d) {
				dp = dfetchdir(d, sname,
					    &nents, &dlen, dodump?&ino:NULL);
#if	defined(HAVE_FSTATAT) && defined(HAVE_DIRFD)
				if (depth <= 0) {
					closedir(d);
					d = NULL;
				}
#else
				closedir(d);
				d = NULL;
#endif
			} else {
				dp = NULL;
			}
			if (dp == NULL) {
				if (!errhidden(E_OPEN, dname)) {
					if (!errwarnonly(E_OPEN, dname))
						xstats.s_openerrs++;
					errmsg("Cannot open '%s'.\n", dname);
					(void) errabort(E_OPEN, dname, TRUE);
				}
				direrr = TRUE;
			} else {
				info->f_dir = dp;
				info->f_dirinos = ino;
				info->f_dirlen = dlen;
				info->f_dirents = nents;
				/*
				 * Don't count list terminator null Byte
				 */
				dlen--;
			}
		} else if (!(d = lopendir(sname))) {
			if (!errhidden(E_OPEN, dname)) {
				if (!errwarnonly(E_OPEN, dname))
					xstats.s_openerrs++;
				errmsg("Cannot open '%s'.\n", dname);
				(void) errabort(E_OPEN, dname, TRUE);
			}
			direrr = TRUE;
		}
	}
	depth--;
	if (!nodir) {
		if (interactive && !ia_change(ptb, info)) {
			fgtprintf(vpr, "Skipping ...\n");
			if (d)
				closedir(d);
			if (dp)
				free(info->f_dir);
			if (info->f_dirinos)
				free(info->f_dirinos);
			depth++;
			return;
		}
		if (putdir) {
			if (!dirmode)
				put_tcb(ptb, info);
			vprint(info);
		}
	}
	if (direrr) {
		depth++;
		return;
	}

	/*
	 * Search parent dir structure for possible loops.
	 */
	thisd.p_last = last;
	thisd.p_dev  = info->f_dev;
	thisd.p_ino  = info->f_ino;

	while (pd) {
		if (pd->p_dev == info->f_dev &&
		    pd->p_ino == info->f_ino) {
			goto out;
		}
		pd = pd->p_last;
	}

	if (!nodesc && (!nomount || !newfs(info))) {
		if (sname != dname)
			comerrno(EX_BAD, "Panic: put_dir(): sname != dname\n");

		name = dname;
		if (pathp == NULL) {
			if (name[0] == '.' && name[1] == '/') {
				for (name++; name[0] == '/'; name++)
					/* LINTED */
					;
				namlen -= name - dname;
			}
			if (name[0] == '.' && name[1] == '\0') {
				name++;
				namlen--;
			}
			pathp = &path;
			if (init_pspace(PS_STDERR, pathp) < 0 ||
			    strlcpy_pspace(PS_STDERR, pathp, name, namlen) < 0) {
				toolong("", name, namlen);
				goto out;
			}
		}
		if (namlen && pathp->ps_path[pathp->ps_tail - 1] != '/') {
			char	*p;

			if ((pathp->ps_tail+2) > pathp->ps_size) {
				if (incr_pspace(PS_STDERR, pathp, 2) < 0) {
					toolong("", name, namlen);
					goto out;
				}
			}
			p = &pathp->ps_path[pathp->ps_tail++];
			*p++ = '/';
			*p++ = '\0';
			namlen++;
			didslash = TRUE;
		}

		while (!intr) {
			char	*snm;

			if (lowmem) {
				if ((dir = readdir(d)) == NULL)
					break;
				if (streql(dir->d_name, ".") ||
						streql(dir->d_name, ".."))
					continue;
				snm = dir->d_name;
				xlen = strlen(snm);
			} else {
				if (dlen <= 0)
					break;

				snm = &dp[1];
				xlen = strlen(snm);
				dp += xlen + 2;
				dlen -= xlen + 2;
			}

			if ((namlen+xlen+1) >= pathp->ps_size) {
				if (grow_pspace(PS_STDERR, pathp, namlen+xlen+1) < 0) {
					toolong(pathp->ps_path, snm, namlen+xlen);
					goto out;
				}
			}
			strcpy(&pathp->ps_path[pathp->ps_tail], snm);
			name = pathp->ps_path;

#if	defined(HAVE_FSTATAT) && defined(HAVE_DIRFD)
			/*
			 * We get the performance win only with the native
			 * fstatat() and not from the emulation.
			 */
			if (d ? !getinfoat(dirfd(d), snm, ninfo) :
				!getinfo(name, ninfo)) {
#else
			if (!getinfo(name, ninfo)) {
#endif
				if (!errhidden(E_STAT, name)) {
					if (!errwarnonly(E_STAT, name))
						xstats.s_staterrs++;
					errmsg("Cannot stat '%s'.\n", name);
					(void) errabort(E_STAT, name, TRUE);
				}
				continue;
			}
#ifdef	HAVE_SEEKDIR
			if (d && is_dir(ninfo) && depth <= 0) {
				seterrno(0);
				offset = telldir(d);
				if (geterrno()) {
					char	*p;

					if (pathp->ps_tail == 0) {
						dname = ".";
						p = NULL;
					} else {
						/*
						 * pathstore_t relocates ps_path
						 */
						dname = pathp->ps_path;
						p = &pathp->ps_path[pathp->ps_tail-1];
						*p = '\0';
					}
					if (!errhidden(E_OPEN, dname)) {
						if (!errwarnonly(E_OPEN, dname))
							xstats.s_openerrs++;
						errmsg(
						"WARNING: telldir does not work on '%s'.\n",
						dname);
						(void) errabort(E_OPEN, dname,
									TRUE);
					}
					if (p)
						*p = '/';
					/*
					 * XXX xstats.s_openerrs is wrong here.
					 * Avoid an endless loop on unseekable
					 * directories.
					 */
					/* closedir() is past end of loop */
					break;
				}
				closedir(d);
			}
#endif
			pathp->ps_tail += xlen;
			createi(name, name, xlen+namlen, ninfo, pathp, &thisd);
			pathp->ps_tail -= xlen;
			pathp->ps_path[pathp->ps_tail] = '\0';
#ifdef	HAVE_SEEKDIR
			if (d && is_dir(ninfo) && depth <= 0) {
				char	*p;

				if (pathp->ps_tail == 0) {
					sname = dname = ".";
					p = NULL;
				} else {
					/*
					 * pathstore_t relocates ps_path
					 */
					sname = dname = pathp->ps_path;
					p = &pathp->ps_path[pathp->ps_tail-1];
					*p = '\0';
				}
				if (!(d = lopendir(sname))) {
					if (!errhidden(E_OPEN, dname)) {
						if (!errwarnonly(E_OPEN, dname))
							xstats.s_openerrs++;
						errmsg("Cannot open '%s'.\n",
							dname);
						(void) errabort(E_OPEN, dname,
									TRUE);
					}
					if (p)
						*p = '/';
					break;
				}
				seterrno(0);
				seekdir(d, offset);
				if (geterrno()) {
					if (!errhidden(E_OPEN, dname)) {
						if (!errwarnonly(E_OPEN, dname))
							xstats.s_openerrs++;
						errmsg(
						"WARNING: seekdir does not work on '%s'.\n",
						dname);
						(void) errabort(E_OPEN, dname,
									TRUE);
					}
					/*
					 * XXX xstats.s_openerrs is wrong here.
					 * Avoid an endless loop on unseekable
					 * directories.
					 */
					if (p)
						*p = '/';
					break;
				}
				if (p)
					*p = '/';
			}
#endif
		}
	}
out:
	if (didslash)
		pathp->ps_path[--pathp->ps_tail] = '\0';
	if (d)
		closedir(d);
	if (dp)
		free(info->f_dir);
	if (info->f_dirinos)
		free(info->f_dirinos);
	if (pathp == &path)
		free_pspace(pathp);
	depth++;
	if (!nodir && dirmode && putdir)
		put_tcb(ptb, info);
}

/*
 * This is currently only called in case we did run out of memory.
 */
LOCAL void
toolong(dname, name, len)
	char	*dname;
	char	*name;
	size_t	len;
{
	if (!errhidden(E_NAMETOOLONG, name)) {
		if (!errwarnonly(E_NAMETOOLONG, name))
			xstats.s_toolong++;
		errmsgno(EX_BAD, "%s%s: Name too long (%lu), out of memory.\n",
			dname, name,
			(unsigned long)len);
		(void) errabort(E_NAMETOOLONG, name, TRUE);
	}
}

LOCAL BOOL
checkdirexclude(sname, name, namlen, info)
	char	*sname;
	char	*name;
	int	namlen;
	FINFO	*info;
{
	FINFO	finfo;
	char	pname[PATH_MAX+1];
	char	*pnamep = pname;
	size_t	pnamelen = sizeof (pname);
	size_t	pnlen;
	int	OFflag = Fflag;
	char	*p;
	BOOL	ret = TRUE;

	Fflag = 0;
	pnlen = namlen;
	pnlen += 2 + 8;		/* Null Byte + '/' + strlen(".exclude") */
	if (pnlen > pnamelen) {
		pnamep = __fjmalloc(stderr, pnlen, "name buffer", JM_RETURN);
		if (pnamep == NULL) {
			errmsg("Cannot check '%s' for exclude\n", name);
			goto notfound;
		}
	}
	strcpy(pnamep, name);
	p = &pnamep[namlen];
	if (p[-1] != '/') {
		*p++ = '/';
	}
	strcpy(p, ".mirror");
	if (!_getinfo(pnamep, &finfo)) {
		strcpy(p, ".exclude");
		if (!_getinfo(pnamep, &finfo))
			goto notfound;
	}
	if (is_file(&finfo)) {
		if (OFflag == 3) {
			nodesc++;
			if (!dirmode) {
				createi(sname, name, namlen, info,
					(pathstore_t *)0,
					(struct pdirs *)0);
			}
			create(pnamep, FALSE, FALSE);	/* Needed to strip off "./" */
			if (dirmode) {
				createi(sname, name, namlen, info,
					(pathstore_t *)0,
					(struct pdirs *)0);
			}
			nodesc--;
		}
		ret = FALSE;
	}
notfound:
	if (pnamep != pname)
		free(pnamep);
	Fflag = OFflag;
	return (ret);
}

LOCAL BOOL
checkexclude(sname, name, namlen, info)
	char	*sname;
	char	*name;
	int	namlen;
	FINFO	*info;
{
		int	len;
	const	char	*fn;

	if (Fflag <= 0)
		return (TRUE);

	fn = filename(name);

	if (is_dir(info)) {
		/*
		 * Exclude with -F -FF -FFFFF 1, 2, 5+
		 */
		if (Fflag < 3 || Fflag > 4) {
			if (streql(fn, "SCCS") ||	/* SCCS directory */
			    streql(fn, "RCS"))		/* RCS directory  */
				return (FALSE);
		}
		if (Fflag > 1 && streql(fn, "OBJ"))	/* OBJ directory  */
			return (FALSE);
		if (Fflag > 2 && !checkdirexclude(sname, name, namlen, info))
			return (FALSE);
		return (TRUE);
	}
	if ((len = strlen(fn)) < 3)		/* Will never match later */
		return (TRUE);

	if (Fflag > 1 && fn[len-2] == '.' && fn[len-1] == 'o')	/* obj files */
		return (FALSE);

	if (Fflag > 1 && is_file(info)) {
		if (streql(fn, "core") ||
		    streql(fn, "errs") ||
		    streql(fn, "a.out"))
			return (FALSE);
	}

	return (TRUE);
}

#ifdef	USE_FIND
/*
 * The callback function for treewalk()
 *
 * XXX errhidden() support not yet implemented.
 */
EXPORT int
walkfunc(nm, fs, type, state)
	char		*nm;
	struct stat	*fs;
	int		type;
	struct WALK	*state;
{
	if (intr) {
		state->flags |= WALK_WF_QUIT;
		return (0);
	}
	if (type == WALK_NS) {
		errmsg("Cannot stat '%s'.\n", nm);
		state->err = 1;
		return (0);
	} else if (type == WALK_SLN && (state->walkflags & WALK_PHYS) == 0) {
		errmsg("Cannot follow symlink '%s'.\n", nm);
		state->err = 1;
		return (0);
	} else if (type == WALK_DNR) {
		if (state->flags & WALK_WF_NOCHDIR)
			errmsg("Cannot chdir to '%s'.\n", nm);
		else
			errmsg("Cannot read '%s'.\n", nm);
		state->err = 1;
		return (0);
	}
	if (state->maxdepth >= 0 && state->level >= state->maxdepth)
		state->flags |= WALK_WF_PRUNE;
	if (state->mindepth >= 0 && state->level < state->mindepth)
		return (0);

	if (state->tree == NULL ||
	    find_expr(nm, nm + state->base, fs, state, state->tree)) {
		FINFO	finfo;

		finfo.f_sname = nm + state->base;
		finfo.f_name = nm;
		stat_to_info(fs, &finfo);
		createi(nm + state->base, nm, strlen(nm), &finfo,
			(pathstore_t *)0, (struct pdirs *)0);
	}
	return (0);
}
#endif
