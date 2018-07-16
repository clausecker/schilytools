/* @(#)diff.c	1.98 18/07/15 Copyright 1993-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)diff.c	1.98 18/07/15 Copyright 1993-2018 J. Schilling";
#endif
/*
 *	List differences between a (tape) archive and
 *	the filesystem
 *
 *	Copyright (c) 1993-2018 J. Schilling
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
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/fcntl.h>
#include "star.h"
#include "props.h"
#include "table.h"
#include "diff.h"
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include <schily/dirent.h>	/* XXX Wegen S_IFLNK */
#include "starsubs.h"
#include "checkerr.h"
#include <schily/fetchdir.h>
#ifdef	USE_FIND
#include <schily/walk.h>
#endif

#include <schily/nlsdefs.h>

typedef	struct {
	FILE	*cmp_file;
	char	*cmp_buf;
	int	cmp_diffs;
} cmp_t;

extern	FILE	*tarf;
extern	char	*listfile;
extern	int	version;

extern	long	bigcnt;
extern	int	bigsize;
extern	char	*bigptr;

extern	BOOL	havepat;
extern	long	hdrtype;
extern	BOOL	debug;
extern	BOOL	no_stats;
extern	BOOL	abs_path;
extern	int	verbose;
extern	BOOL	tpath;
extern	BOOL	interactive;

#ifdef USE_ACL
extern	BOOL	doacl;
#else
#define	doacl	FALSE
#endif
#ifdef USE_XATTR
extern	BOOL	doxattr;
#else
#define	doxattr	FALSE
#endif
#ifdef USE_FFLAGS
extern	BOOL	dofflags;
#endif

#ifdef	USE_FIND
extern	BOOL	dofind;
#endif

EXPORT	void	diff		__PR((void));
LOCAL	void	diff_tcb	__PR((FINFO *info));
LOCAL	BOOL	linkeql		__PR((char *n1, char *n2));
LOCAL	BOOL	patheql		__PR((char *n1, char *n2));
LOCAL	BOOL	dirdiffs	__PR((FILE *f, FINFO *info));
LOCAL	int	cmp_func	__PR((cmp_t *cmp, char *p, int amount));
LOCAL	BOOL	cmp_file	__PR((FINFO *info));
EXPORT	void	prdiffopts	__PR((FILE *f, char *label, int flags));
LOCAL	void	prdopt		__PR((FILE *f, char *name, int printed));

EXPORT void
diff()
{
#ifdef	USE_FIND
extern	struct WALK walkstate;
#endif
		FINFO	finfo;
		TCB	tb;
	register TCB 	*ptb = &tb;

	fillbytes((char *)&finfo, sizeof (finfo), '\0');

	if (init_pspace(PS_STDERR, &finfo.f_pname) < 0)
		return;
	if (init_pspace(PS_STDERR, &finfo.f_plname) < 0)
		return;

	finfo.f_tcb = ptb;

	/*
	 * We first have to read a control block to know what type of
	 * tar archive we are reading from.
	 */
	if (get_tcb(ptb) == EOF)
		return;

	diffopts &= ~props.pr_diffmask;
	if (!no_stats)
		prdiffopts(stderr, "diffopts=", diffopts);

#ifdef	USE_FIND
	if (dofind) {
		walkopen(&walkstate);
		walkgethome(&walkstate);	/* Needed in case we chdir */
	}
#endif
	for (;;) {
		finfo.f_name = finfo.f_pname.ps_path;
		finfo.f_lname = finfo.f_plname.ps_path;
		if (tcb_to_info(ptb, &finfo) == EOF)
			break;
#ifdef	USE_FIND
		if (dofind && !findinfo(&finfo)) {
			void_file(&finfo);
			goto cont;
		}
#endif

		if (listfile && !hash_lookup(finfo.f_name)) {
			void_file(&finfo);
			goto cont;
		}
		if (hash_xlookup(finfo.f_name)) {
			void_file(&finfo);
			goto cont;
		}
		if (havepat && !match(finfo.f_name)) {
			void_file(&finfo);
			goto cont;
		}
		diff_tcb(&finfo);
	cont:
		if (get_tcb(ptb) == EOF)
			break;
	}
#ifdef	USE_FIND
	if (dofind) {
		walkhome(&walkstate);
		walkclose(&walkstate);
		free(walkstate.twprivate);
	}
#endif
}

LOCAL void
diff_tcb(info)
	register FINFO	*info;
{
		TCB	tb;
		FINFO	finfo;
		FINFO	linfo;
		FILE	*f;
		int	diffs = 0;
		BOOL	do_void = FALSE;	/* Make GCC happy */

	f = tarf == stdout ? stderr : stdout; /* XXX FILE *vpr is the same */

	if (!abs_path &&	/* XXX VVV siehe skip_slash() */
	    (info->f_name[0] == '/' /* || info->f_lname[0] == '/' */))
		skip_slash(info);

	if (is_volhdr(info)) {
		void_file(info);
		return;
	}
	/*
	 * Use getinfo() if we like to compare ACLs/xattr too.
	 */
	if (((doacl || doxattr)? !getinfo(info->f_name, &finfo):
				!_getinfo(info->f_name, &finfo))) {
		if (!errhidden(E_STAT, info->f_name)) {
			if (!errwarnonly(E_STAT, info->f_name))
				xstats.s_staterrs++;
			errmsg("Cannot stat '%s'.\n", info->f_name);
			(void) errabort(E_STAT, info->f_name, TRUE);
		}
		void_file(info);
		return;
	}
	/*
	 * We cannot compare the link count if this is a CPIO archive
	 * and the link count is < 2. Even if the link count is >= 2, it
	 * may not be exact.
	 */
	if ((props.pr_flags & PR_CPIO) && info->f_nlink < 2)
		info->f_nlink = 0;
	if (info->f_nlink == 0)		/* If archive has no link counts    */
		finfo.f_nlink = 0;	/* always suppress nlink in listing. */

	fillbytes(&tb, sizeof (TCB), '\0');

	info_to_tcb(&finfo, &tb);	/* XXX ist das noch nötig ??? */
					/* z.Zt. wegen linkflag/uname/gname */

	if ((diffopts & D_PERM) &&
			(info->f_mode & 07777) != (finfo.f_mode & 07777)) {
		if ((diffopts & D_SYMPERM) != 0 || !is_symlink(&finfo))
			diffs |= D_PERM;
	/*
	 * XXX Diff ACLs not yet implemented.
	 */

	}

	if ((diffopts & D_NLINK) && info->f_nlink > 0 &&
			info->f_nlink != finfo.f_nlink) {
		diffs |= D_NLINK;
	}

	if ((diffopts & D_UID) && info->f_uid != finfo.f_uid) {
		diffs |= D_UID;
	}
	if ((diffopts & D_GID) && info->f_gid != finfo.f_gid) {
		diffs |= D_GID;
	}

	/*
	 * Note that uname/gname in the old star header are not always
	 * null terminated.
	 */
	if ((diffopts & D_UNAME) && info->f_uname && finfo.f_uname) {
		if (strncmp(info->f_uname, finfo.f_uname, info->f_umaxlen))
			diffs |= D_UNAME;
	}
	if ((diffopts & D_GNAME) && info->f_gname && finfo.f_gname) {
		if (strncmp(info->f_gname, finfo.f_gname, info->f_gmaxlen))
			diffs |= D_GNAME;
	}

	/*
	 * XXX hier kann es bei ustar/cpio inkompatibel werden!
	 *
	 * Z.Zt. hat nur das STAR Format auch bei Hardlinks den Filetype.
	 *	Soll man die teilweise bei fehlerhaften USTAR
	 *	Implementierungen vorhandenen Filetype Bits verwenden?
	 *
	 * XXX CPIO EXUSTAR, ... haben auch den Filetyp.
	 */
	if ((diffopts & D_TYPE) &&
	    (info->f_filetype != finfo.f_filetype ||
	    (is_special(info) && info->f_type != finfo.f_type)) &&
	    (!fis_link(info) || H_TYPE(hdrtype) == H_STAR)) {

		if (fis_meta(info) && is_file(&finfo)) {
			/* EMPTY */
			;
		} else {
			if (debug) {
				fgtprintf(f,
				"%s: different filetype  %llo != %llo\n",
				info->f_name,
				(Ullong)info->f_type, (Ullong)finfo.f_type);
			}
			diffs |= D_TYPE;
		}
	}

	/*
	 * nsec beachten wenn im Archiv!
	 */
	if ((diffopts & D_ATIME) != 0) {
		if (info->f_atime != finfo.f_atime) {
			diffs |= D_ATIME;
		} else if ((diffopts & D_ANTIME) != 0) {
			if ((info->f_xflags & XF_ATIME) &&
			    (finfo.f_flags & F_NSECS) &&
			    info->f_ansec != finfo.f_ansec) {
				diffs |= D_ANTIME;
				if ((info->f_ansec % 1000 == 0 ||
				    finfo.f_ansec % 1000 == 0) &&
				    (info->f_ansec / 1000 ==
				    finfo.f_ansec / 1000))
					diffs &= ~D_ANTIME;
			}
		}
	}
	if ((diffopts & D_MTIME) != 0) {
		if ((diffopts & D_LMTIME) != 0 || !is_symlink(&finfo)) {
			if (info->f_mtime != finfo.f_mtime) {
				diffs |= D_MTIME;
			} else if ((diffopts & D_MNTIME) != 0) {
				if ((info->f_xflags & XF_MTIME) &&
				    (finfo.f_flags & F_NSECS) &&
				    info->f_mnsec != finfo.f_mnsec) {
					diffs |= D_MNTIME;
					if ((info->f_mnsec % 1000 == 0 ||
					    finfo.f_mnsec % 1000 == 0) &&
					    (info->f_mnsec / 1000 ==
					    finfo.f_mnsec / 1000))
					diffs &= ~D_MNTIME;
				}
			}
		}
	}
	if ((diffopts & D_CTIME) != 0) {
		if (info->f_ctime != finfo.f_ctime) {
			diffs |= D_CTIME;
		} else if ((diffopts & D_CNTIME) != 0) {
			if ((info->f_xflags & XF_CTIME) &&
			    (finfo.f_flags & F_NSECS) &&
			    info->f_cnsec != finfo.f_cnsec) {
				diffs |= D_CNTIME;
				if ((info->f_cnsec % 1000 == 0 ||
				    finfo.f_cnsec % 1000 == 0) &&
				    (info->f_cnsec / 1000 ==
				    finfo.f_cnsec / 1000))
				diffs &= ~D_CNTIME;
			}
		}
	}

	if ((diffopts & D_DIR) && is_dir(info) && info->f_dir &&
	    is_dir(&finfo)) {
		if (dirdiffs(f, info))
			diffs |= D_DIR;
	}

	if ((diffopts & D_HLINK) && is_link(info)) {
		if (!_getinfo(info->f_lname, &linfo)) {
			if (!errhidden(E_STAT, info->f_lname)) {
				if (!errwarnonly(E_STAT, info->f_lname))
					xstats.s_staterrs++;
				errmsg("Cannot stat '%s'.\n", info->f_lname);
				(void) errabort(E_STAT, info->f_lname, TRUE);
			}
			linfo.f_ino = (ino_t)0;
		}
		if ((finfo.f_ino != linfo.f_ino) ||
		    (finfo.f_dev != linfo.f_dev)) {
			if (debug || verbose)
				fgtprintf(f, "%s: not linked to %s\n",
					info->f_name, info->f_lname);

			diffs |= D_HLINK;
		}
	}
#ifdef	S_IFLNK
	/*
	 * In case of a hardlinked symlink, we currently do not have the symlink
	 * target path and thus cannot check the synlink target.
	 */
	if (!is_link(info))
	if (((diffopts & (D_SLINK|D_SLPATH)) || verbose) && is_symlink(&finfo)) {
		if (init_pspace(PS_STDERR, &finfo.f_plname) < 0)
			;
		else if (read_symlink(info->f_name, info->f_name, &finfo, &tb)) {
			if ((diffopts & D_SLINK) && is_symlink(info) &&
			    !linkeql(info->f_lname, finfo.f_lname)) {
				diffs |= D_SLINK;
			}
			if ((diffopts & D_SLPATH) && is_symlink(info) &&
			    !streql(info->f_lname, finfo.f_lname)) {
				diffs |= D_SLPATH;
			}
		}
	}
#endif

	/*
	 * Only plain files may have holes, so only open plain files.
	 * We cannot compare the "sparseness" of hardlinks that do not
	 * include data in the archive.
	 */
	if (diffopts & D_SPARS && is_a_file(&finfo) &&
	    !(is_link(info) && info->f_size == 0)) {
		/*
		 * This is not the right place to check for SEEK_HOLE bit it is
		 * the only way to avoid opening the file in case it does not
		 * make sense.
		 */
#if	defined(SEEK_HOLE) && defined(SEEK_DATA)
		int	fd;

#ifndef	O_NDELAY
#define	O_NDELAY	0
#define	NDELAY
#endif
		if ((fd = open(finfo.f_name, O_RDONLY|O_NDELAY)) >= 0) {
			if (sparse_file(&fd, &finfo))
				finfo.f_flags |= F_SPARSE;
			else
				finfo.f_flags &= ~F_SPARSE;
			close(fd);
		}
#ifdef	NDELAY
#undef	O_NDELAY
#endif
#endif
		if (is_sparse(info) != ((finfo.f_flags & F_SPARSE) != 0)) {
			if (debug || verbose) {
				fgtprintf(f, "%s: %s not sparse\n",
					info->f_name,
					is_sparse(info) ? "target":"source");
			}
			diffs |= D_SPARS;
		}
	}

	if ((diffopts & D_SIZE) && !is_link(info) &&
	    is_file(info) && is_file(&finfo) &&
	    info->f_size != finfo.f_size) {

		diffs |= D_SIZE;
	}
	/*
	 * Rdev makes only sense with char & blk devices.
	 * Rdev is usually 0 for other special file types but at least
	 * the SunOS/Solaris 'tmpfs' has random values in rdev.
	 */
	if ((diffopts & D_RDEV) && is_dev(info) && is_dev(&finfo) &&
	    info->f_rdev != finfo.f_rdev) {
		diffs |= D_RDEV;
	}

	/*
	 * XXX Wann geht das evt. mit Hardlinks CPIO, neues Tar....
	 */
	if ((diffopts & D_DATA) && !is_meta(info) &&
	    (info->f_rsize > (off_t)0 || !is_link(info)) &&
	    is_file(info) && is_file(&finfo)) {

		    /* avoid permission denied error */
		if (info->f_size > (off_t)0 &&
		    info->f_size == finfo.f_size) {
			if (!cmp_file(info)) {
				diffs |= D_DATA;
			}
			do_void = FALSE;
		} else if (info->f_size != finfo.f_size) {
			diffs |= D_DATA;
			do_void = TRUE;
		}
	} else {
		do_void = TRUE;
	}

#ifdef USE_ACL
	if (doacl && (diffopts & D_ACL)) {
		if ((info->f_xflags & XF_ACL_ACCESS) !=
		    (finfo.f_xflags & XF_ACL_ACCESS)) {
			diffs |= D_ACL;
		} else if ((info->f_xflags & XF_ACL_ACCESS) != 0) {
			if (strcmp(info->f_acl_access, finfo.f_acl_access))
				diffs |= D_ACL;
		}
		if ((info->f_xflags & XF_ACL_DEFAULT) !=
		    (finfo.f_xflags & XF_ACL_DEFAULT)) {
			diffs |= D_ACL;
		} else if ((info->f_xflags & XF_ACL_DEFAULT) != 0) {
			if (strcmp(info->f_acl_default, finfo.f_acl_default))
				diffs |= D_ACL;
		}
		if ((info->f_xflags & XF_ACL_ACE) !=
		    (finfo.f_xflags & XF_ACL_ACE)) {
			diffs |= D_ACL;
		} else if ((info->f_xflags & XF_ACL_ACE) != 0) {
			if (strcmp(info->f_acl_ace, finfo.f_acl_ace))
				diffs |= D_ACL;
		}
	}
#endif

#ifdef USE_XATTR
	if (doxattr && (diffopts & D_XATTR)) {
		if ((info->f_xflags & XF_XATTR) !=
		    (finfo.f_xflags & XF_XATTR)) {
			diffs |= D_XATTR;
		} else if ((info->f_xflags & XF_XATTR) != 0) {
			register star_xattr_t	*x1 = info->f_xattr;
			register star_xattr_t	*x2 = finfo.f_xattr;

			for (; x1->name && x2->name; x1++, x2++) {
				if (strcmp(x1->name, x2->name))
					break;
				if (x1->value_len != x2->value_len)
					break;
				if (memcmp(x1->value, x2->value, x2->value_len))
					break;
			}
			if (x1->name || x2->name)
				diffs |= D_XATTR;
		}
	}
#endif

#ifdef	USE_FFLAGS
	if (dofflags && (diffopts & D_FFLAGS)) {
		if (info->f_fflags != finfo.f_fflags)
			diffs |= D_FFLAGS;
	}
#endif

	if (diffs) {
		if (errhidden(E_DIFF, info->f_name))
			goto out;

		if (tpath) {
			fprintf(f, "%s\n", info->f_name);
		} else {
			fprintf(f, "%s: ", info->f_name);
			prdiffopts(f, "different ", diffs);
		}
	}

	if (verbose && diffs) {
		list_file(info);
		list_file(&finfo);
	}
	if (diffs)
		errabort(E_DIFF, info->f_name, TRUE);
out:
	if (do_void)
		void_file(info);
}

LOCAL BOOL
linkeql(n1, n2)
	register char	*n1;
	register char	*n2;
{
	FINFO	l1info;
	FINFO	l2info;
extern	BOOL	follow;
	BOOL	ofollow = follow;

	/*
	 * If the names are identical, return TRUE
	 */
	if (streql(n1, n2))
		return (TRUE);
	/*
	 * Not equal if only one of both names is an abs. path name.
	 */
	if (*n1 != *n2 && (*n1 == '/' || *n2 == '/'))
		return (FALSE);
	/*
	 * If the names point to the same inode, return TRUE
	 */
	follow = TRUE;
	if (_getinfo(n1, &l1info) && _getinfo(n2, &l2info)) {
		follow = ofollow;
		if (l1info.f_dev == l2info.f_dev &&
		    l1info.f_ino == l2info.f_ino) {
			return (TRUE);
		} else {
			return (FALSE);
		}
	}
	follow = ofollow;

	return (patheql(n1, n2));
}

LOCAL BOOL
patheql(n1, n2)
	register char	*n1;
	register char	*n2;
{
	while (n1[0] == '.' && n1[1] == '/') {
		n1 += 2;
		while (n1[0] == '/')
			n1++;
	}
	while (n2[0] == '.' && n2[1] == '/') {
		n2 += 2;
		while (n2[0] == '/')
			n2++;
	}
	for (; n1[0] != '\0' && n2[0]  != '\0'; n1++, n2++) {
		if (n1[0] == '/') {
			while (n1[1] == '.' && n1[2] == '/') {
				n1 += 2;
				while (n1[1] == '/')
					n1++;
			}
		}
		if (n2[0] == '/') {
			while (n2[1] == '.' && n2[2] == '/') {
				n2 += 2;
				while (n2[1] == '/')
					n2++;
			}
		}
		if (n1[0] != n2[0])
			break;
	}
	return (n1[0] == n2[0]);
}

LOCAL BOOL
dirdiffs(f, info)
	FILE	*f;
	FINFO	*info;
{
	register char	**ep1;	   /* Directory entry pointer array (arch) */
	register char	**ep2 = 0; /* Directory entry pointer array (disk) */
	register char	*dp2;	   /* Directory names string from disk	   */
	register char	**oa = 0;  /* Only in arch pointer array	   */
	register char	**od = 0;  /* Only on disk pointer array	   */
	register int	i;
		int	ents1 = -1;
		int	ents2;
		int	dlen = 0;
		int	alen = 0;
		BOOL	diffs = FALSE;
		DIR	*dirp;
		int	err;

	/*
	 * Old archives had only one nul at the end
	 * xheader.c already increments info->f_dirlen in this case
	 * but a newline may appear to be the last char.
	 * Note that we receicve the space from the xheader
	 * extract buffer.
	 */
	i = info->f_dirlen;
	if (info->f_dir[i-1] != '\0')
		info->f_dir[i-1] = '\0';	/* Kill '\n' */

	ep1 = sortdir(info->f_dir, &ents1);	/* from archive */
	dirp = lopendir(info->f_name);
	if (dirp == NULL) {
		dp2 = NULL;
		err = geterrno();
	} else {
		dp2 = dfetchdir(dirp, info->f_name, &ents2, 0, NULL);
		err = geterrno();
		closedir(dirp);
	}
	if (dp2 == NULL) {
		diffs = TRUE;
		errmsgno(err, "Cannot read dir '%s'.\n", info->f_name);
		goto no_dircmp;
	}
	ep2 = sortdir(dp2, &ents2);		/* from disk */

	if (ents1 != ents2) {
		if (debug || verbose > 2) {
			fgtprintf(f, "Archive ents: %d Disk ents: %d '%s'\n",
					ents1, ents2, info->f_name);
		}
		diffs = TRUE;
	}

	if (cmpdir(ents1, ents2, ep1, ep2, NULL, NULL, &alen, &dlen) > 0)
		diffs = TRUE;

	oa = ___malloc(alen * sizeof (char *), "dir diff array");
	od = ___malloc(dlen * sizeof (char *), "dir diff array");
	cmpdir(ents1, ents2, ep1, ep2, oa, od, &alen, &dlen);

	if (debug || verbose > 1) {
		for (i = 0; i < dlen; i++) {
			fgtprintf(f, "Only on disk '%s': '%s'\n",
					info->f_name, od[i] + 1);
		}
		for (i = 0; i < alen; i++) {
			fgtprintf(f, "Only in archive '%s': '%s'\n",
					info->f_name, oa[i] + 1);
		}
	}

no_dircmp:
	if (dp2)
		free(dp2);
	if (ep1)
		free(ep1);
	if (ep2)
		free(ep2);
	if (od)
		free(od);
	if (oa)
		free(oa);

	return (diffs);
}

#define	vp_cmp_func	((int(*)__PR((void *, char *, int)))cmp_func)

LOCAL int
cmp_func(cmp, p, amount)
	register cmp_t	*cmp;
	register char	*p;
		int	amount;
{
	register int	cnt;

	/*
	 * If we already found diffs we save time and only pass tape ...
	 */
	if (cmp->cmp_diffs)
		return (amount);

	cnt = ffileread(cmp->cmp_file, cmp->cmp_buf, amount);
	if (cnt != amount)
		cmp->cmp_diffs++;

	if (cmpbytes(cmp->cmp_buf, p, cnt) < cnt)
		cmp->cmp_diffs++;
	return (cnt);
}

static char	*diffbuf;

LOCAL BOOL
cmp_file(info)
	FINFO	*info;
{
	FILE	*f;
	cmp_t	cmp;

	if (!diffbuf) {
		/*
		 * If we have no diffbuf, we cannot diff - abort.
		 */
		diffbuf = ___malloc((size_t)bigsize, "diff buffer");
#ifdef	__notneeded
		if (diffbuf == (char *)0) {
			void_file(info);
			return (FALSE);
		}
#endif
	}

	if ((f = lfilemopen(info->f_name, "rub", S_IRWALL)) == (FILE *)NULL) {
		if (!errhidden(E_OPEN, info->f_name)) {
			if (!errwarnonly(E_OPEN, info->f_name))
				xstats.s_openerrs++;
			errmsg("Cannot open '%s'.\n", info->f_name);
			(void) errabort(E_OPEN, info->f_name, TRUE);
		}
		void_file(info);
		return (FALSE);
	} else
		file_raise(f, FALSE);

	if (is_sparse(info))
		return (cmp_sparse(f, info));

	cmp.cmp_file = f;
	cmp.cmp_buf = diffbuf;
	cmp.cmp_diffs = 0;
	if (xt_file(info, vp_cmp_func, &cmp, bigsize, "reading") < 0)
		die(EX_BAD);
	fclose(f);
	return (cmp.cmp_diffs == 0);
}

EXPORT void
prdiffopts(f, label, flags)
	FILE	*f;
	char	*label;
	int	flags;
{
	int	printed = 0;

	fprintf(f, "%s", label);
	if (flags & D_PERM)
		prdopt(f, "perm", printed++);
	if (flags & D_SYMPERM)
		prdopt(f, "symperm", printed++);
	/*
	 * XXX Diff ACLs not yet implemented.
	 */
	if (flags & D_TYPE)
		prdopt(f, "type", printed++);
	if (flags & D_NLINK)
		prdopt(f, "nlink", printed++);
	if (flags & D_UID)
		prdopt(f, "uid", printed++);
	if (flags & D_GID)
		prdopt(f, "gid", printed++);
	if (flags & D_UNAME)
		prdopt(f, "uname", printed++);
	if (flags & D_GNAME)
		prdopt(f, "gname", printed++);
	if (flags & D_SIZE)
		prdopt(f, "size", printed++);
	if (flags & D_DATA)
		prdopt(f, "data", printed++);
	if (flags & D_RDEV)
		prdopt(f, "rdev", printed++);
	if (flags & D_HLINK)
		prdopt(f, "hardlink", printed++);
	if (flags & D_SLINK)
		prdopt(f, "symlink", printed++);
	if (flags & D_SLPATH)
		prdopt(f, "sympath", printed++);
	if (flags & D_SPARS)
		prdopt(f, "sparse", printed++);
	if (flags & D_ATIME)
		prdopt(f, "atime", printed++);
	if (flags & D_MTIME)
		prdopt(f, "mtime", printed++);
	if (flags & D_CTIME)
		prdopt(f, "ctime", printed++);
	if (flags & D_LMTIME)
		prdopt(f, "lmtime", printed++);
	if (flags & D_ANTIME)
		prdopt(f, "ansecs", printed++);
	if (flags & D_MNTIME)
		prdopt(f, "mnsecs", printed++);
	if (flags & D_CNTIME)
		prdopt(f, "cnsecs", printed++);
	if (flags & D_DIR)
		prdopt(f, "dir", printed++);
#ifdef USE_ACL
	if (flags & D_ACL)
		prdopt(f, "acl", printed++);
#endif
#ifdef USE_XATTR
	if (flags & D_XATTR)
		prdopt(f, "xattr", printed++);
#endif
#ifdef	USE_FFLAGS
	if (flags & D_FFLAGS)
		prdopt(f, "fflags", printed++);
#endif
	fprintf(f, "\n");
}

LOCAL void
prdopt(f, name, printed)
	FILE	*f;
	char	*name;
	int	printed;
{
	if (printed)
		fprintf(f, ",");
	fprintf(f, "%s", name);
}
