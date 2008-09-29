/* @(#)extract.c	1.132 08/09/26 Copyright 1985-2008 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)extract.c	1.132 08/09/26 Copyright 1985-2008 J. Schilling";
#endif
/*
 *	extract files from archive
 *
 *	Copyright (c) 1985-2008 J. Schilling
 */
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

#include <schily/mconfig.h>
#include <stdio.h>
#include <schily/standard.h>
#include "star.h"
#include "props.h"
#include "table.h"
#include <schily/dirent.h>	/* XXX Wegen S_IFLNK */
#include <schily/unistd.h>
#include <schily/fcntl.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/stdlib.h>
#include <schily/errno.h>
#ifdef	USE_FIND
#include <schily/walk.h>
#endif

#ifdef	JOS
#	define	mkdir	makedir
#endif
#include "dirtime.h"
#include "restore.h"
#include "starsubs.h"
#include "checkerr.h"
#include <schily/fetchdir.h>

#define	ROOT_UID	0

#if defined(ENOTEMPTY) && ENOTEMPTY != EEXIST
#define	is_eexist(err)	((err) == EEXIST || (err) == ENOTEMPTY)
#else
#define	is_eexist(err)	((err) == EEXIST)
#endif
#if defined(EMISSDIR)
#define	is_enoent(err)	((err) == ENOENT || (err) == EMISSDIR)
#else
#define	is_enoent(err)	((err) == ENOENT)
#endif

/*
 * Used for _make_copy()/make_copies()/copy_file()
 */
#define	HIDE_ENOENT	0x01

/*
 * Used by the what paramater of same_file()
 */
#define	IS_COPY		0
#define	IS_LINK		1

extern	FILE	*vpr;

extern	char	*listfile;

extern	int	bufsize;
extern	char	*bigptr;

extern	uid_t	dir_uid;
extern	gid_t	dir_gid;
extern	uid_t	my_uid;
extern	BOOL	no_fsync;
extern	BOOL	havepat;
extern	dev_t	curfs;
extern	int	xdebug;
extern	int	verbose;
extern	BOOL	prblockno;
extern	BOOL	nflag;
extern	BOOL	interactive;
extern	BOOL	noxdir;
extern	BOOL	follow;
extern	BOOL	nospec;
extern	BOOL	xdir;
extern	BOOL	xdot;
extern	BOOL	uncond;
extern	BOOL	keep_old;
extern	BOOL	refresh_old;
extern	BOOL	abs_path;
extern	BOOL	allow_dotdot;
extern	BOOL	secure_links;
extern	BOOL	nowarn;
extern	BOOL	force_hole;
extern	BOOL	to_stdout;
extern	BOOL	force_remove;
extern	BOOL	remove_first;
extern	BOOL	remove_recursive;
extern	BOOL	do_install;
extern	BOOL	copyhardlinks;
extern	BOOL	copysymlinks;
extern	BOOL	copydlinks;
extern	BOOL	hardlinks;
extern	BOOL	symlinks;
extern	BOOL	dorestore;
extern	BOOL	dometa;
extern	BOOL	xmeta;
extern	BOOL	lowmem;
extern	BOOL	do_subst;

#ifdef	USE_FIND
extern	BOOL	dofind;
#endif

LOCAL	void	init_umask	__PR((void));
EXPORT	void	extract		__PR((char *vhname));
EXPORT	BOOL	extracti	__PR((FINFO *info, imap_t *imp));
EXPORT	BOOL	newer		__PR((FINFO *info, FINFO *cinfo));
EXPORT	BOOL	same_symlink	__PR((FINFO *info));
LOCAL	BOOL	_create_dirs	__PR((char *name));
LOCAL	void	_dir_setownwer	__PR((char *name));
EXPORT	BOOL	create_dirs	__PR((char *name));
EXPORT	BOOL	make_adir	__PR((FINFO *info));
LOCAL	BOOL	make_dir	__PR((FINFO *info));
LOCAL	BOOL	make_link	__PR((FINFO *info));
LOCAL	BOOL	make_symlink	__PR((FINFO *info));
LOCAL	BOOL	emul_symlink	__PR((FINFO *info));
LOCAL	BOOL	emul_link	__PR((FINFO *info));
LOCAL	BOOL	same_file	__PR((FINFO *info, int what, BOOL do_follow));
LOCAL	BOOL	make_copy	__PR((FINFO *info, BOOL do_symlink, int eflags));
#ifdef COPY_LINKS_DELAYED
LOCAL	void	add_copy	__PR((FINFO *info, BOOL do_symlink));
LOCAL	void	make_copies	__PR((void));
LOCAL	BOOL	_make_dcopy	__PR((FINFO *info, BOOL do_symlink, int *retp, int eflags));
#endif
LOCAL	BOOL	_make_copy	__PR((FINFO *info, BOOL do_symlink, int eflags));
LOCAL	int	copy_file	__PR((char *from, char *to, BOOL do_symlink, int eflags));
LOCAL	BOOL	make_fifo	__PR((FINFO *info));
LOCAL	BOOL	make_special	__PR((FINFO *info));
LOCAL	BOOL	file_tmpname	__PR((FINFO *info, char *xname));
LOCAL	FILE	*file_open	__PR((FINFO *info, char *name));
LOCAL	BOOL	get_file	__PR((FINFO *info));
LOCAL	BOOL	install_rename	__PR((FINFO *info, char *xname));
LOCAL	BOOL	name_exists	__PR((char *name));
LOCAL	void	remove_tmpname	__PR((char *name));
LOCAL	BOOL	get_ofile	__PR((FILE *f, FINFO *info));
LOCAL	int	void_func	__PR((void *vp, char *p, int amount));
EXPORT	BOOL	void_file	__PR((FINFO * info));
LOCAL	BOOL	void_bad	__PR((FINFO * info));
EXPORT	int	xt_file		__PR((FINFO * info,
					int (*)(void *, char *, int),
					void *arg, int amt, char *text));
EXPORT	void	skip_slash	__PR((FINFO * info));
LOCAL	BOOL	has_dotdot	__PR((char *name));

#ifdef COPY_LINKS_DELAYED
typedef	struct _mcq {
	struct _mcq *next;
	FINFO	info;
	BOOL	do_symlink;
} MCQ;

LOCAL	MCQ	*mcq_1st	= NULL;
LOCAL	MCQ	*mcq_last	= NULL;

#endif

/*
 * This is used to allow extracting archives as non root when they
 * contain read only directories. It tries to stay as close as possible
 * to the user's umask when creating intermediate directories.
 * We do not modify the umask in a way that would even grant unepected
 * permissions to others for a short time.
 */
LOCAL	mode_t	old_umask;
LOCAL	mode_t	mode_mask;

#define	PERM_BITS	(S_IRWXU|S_IRWXG|S_IRWXO)	/* u/g/o basic perm */

LOCAL void
init_umask()
{
	old_umask = umask((mode_t)0);
	mode_mask = PERM_BITS & ~old_umask;
	if (my_uid != ROOT_UID)
		umask(old_umask & ~S_IRWXU);
	else
		umask(old_umask);
}

EXPORT void
extract(vhname)
	char	*vhname;
{
#ifdef	USE_FIND
extern	struct WALK walkstate;
#endif
		FINFO	finfo;
		TCB	tb;
		char	name[PATH_MAX+1];
		char	lname[PATH_MAX+1];
	register TCB 	*ptb = &tb;
		BOOL	restore_init = FALSE;
		imap_t	*imp = NULL;

	init_umask();		/* Needed to extract read only directories */

	if (dorestore)		/* With incremental restore, we need to open */
		sym_open(NULL);	/* the 'star-symtable' first.		    */

	fillbytes((char *)&finfo, sizeof (finfo), '\0');

#ifdef	USE_FIND
	if (dofind) {
		walkopen(&walkstate);
		walkgethome(&walkstate);	/* Needed in case we chdir */
	}
#endif
	finfo.f_tcb = ptb;
	for (;;) {
		if (get_tcb(ptb) == EOF)
			break;
		finfo.f_name = name;
		finfo.f_lname = lname;
		if (tcb_to_info(ptb, &finfo) == EOF)
			break;
		if (xdebug > 0)
			dump_info(&finfo);
		if (prblockno)
			(void) tblocks();		/* set curblockno */

		if (is_volhdr(&finfo)) {
			if (!get_volhdr(&finfo, vhname)) {
				excomerrno(EX_BAD,
				"Volume Header '%s' does not match '%s'.\n",
							finfo.f_name, vhname);
			}
			void_file(&finfo);
			continue;
		}
#ifdef	USE_FIND
		if (dofind && !findinfo(&finfo)) {
			void_file(&finfo);
			continue;
		}
#endif
		if (finfo.f_flags & F_BAD_META) {
			if (!void_bad(&finfo))
				break;
			continue;
		}

		if (!abs_path &&	/* XXX VVV siehe skip_slash() */
		    (finfo.f_name[0] == '/' /* || finfo.f_lname[0] == '/'*/))
			skip_slash(&finfo);

		if (dorestore) {
			extern	GINFO	*grip;	/* Global read info pointer */

			if (!restore_init) {
				curfs = finfo.f_dev;
				sym_init(grip);
				restore_init = TRUE;
			}

			imp = sym_addrec(&finfo);
			if (is_dir(&finfo) && grip->dumplevel > 0)
				imp = sym_dirprepare(&finfo, imp);
		}

		/*
		 * Special treatment for the idiosyncratic way of dealing with
		 * hard links in the SVr4 CRC cpio archive format.
		 * The link count is handled by calling read_link() in
		 * cpiotcb_to_info() before.
		 */
		if ((props.pr_flags & PR_SV_CPIO_LINKS) != 0 &&
		    !is_dir(&finfo) &&
		    (is_link(&finfo) || finfo.f_nlink > 1)) {
			if (!last_cpio_link(&finfo))	/* Ign. all but last */
				continue;
			if (xcpio_link(&finfo))		/* Now extract all   */
				continue;
		}
		extracti(&finfo, imp);
	}
#ifdef	USE_FIND
	if (dofind) {
		walkhome(&walkstate);
		walkclose(&walkstate);
		free(walkstate.twprivate);
	}
#endif

#ifdef COPY_LINKS_DELAYED
	if (copyhardlinks || copysymlinks)
		make_copies();
#endif
	dirtimes("", (struct timeval *)0, (mode_t)0);
	if (dorestore)
		sym_close();
}

/*
 * Extract one file from archive
 */
EXPORT BOOL
extracti(info, imp)
		FINFO	*info;
		imap_t	*imp;
{
		FINFO	cinfo;
		TCB	tb;
	register TCB 	*ptb = &tb;
		char	*name = info->f_name;

	if (listfile && !hash_lookup(info->f_name)) {
		void_file(info);
		return (FALSE);
	}
	if (hash_xlookup(info->f_name)) {
		void_file(info);
		return (FALSE);
	}
	if (havepat && !match(info->f_name)) {
		void_file(info);
		return (FALSE);
	}
	if (!is_file(info) && to_stdout) {
		void_file(info);
		return (FALSE);
	}
	if (is_special(info) && nospec) {
		if (!errhidden(E_SPECIALFILE, info->f_name)) {
			if (!errwarnonly(E_SPECIALFILE, info->f_name))
				xstats.s_isspecial++;
			errmsgno(EX_BAD,
				"'%s' is not a file. Not created.\n",
							info->f_name);
			(void) errabort(E_SPECIALFILE, info->f_name, TRUE);
		}
		void_file(info);
		return (FALSE);
	}
	/*
	 * If uncond is set, then newer() doesn't call getinfo(&cinfo)
	 */
	if (newer(info, &cinfo) && !(xdir && is_dir(info))) {
		void_file(info);
		return (FALSE);
	}
	if (is_symlink(info) && same_symlink(info)) {
		void_file(info);
		return (FALSE);
	}
	if (do_subst && subst(info)) {
		if (info->f_name[0] == '\0') {
			if (verbose)
				fprintf(vpr,
				"'%s' substitutes to null string, skipping ...\n",
							name);
			void_file(info);
			return (FALSE);
		}
	}
	if (interactive && !ia_change(ptb, info)) {
		if (!nflag)
			fprintf(vpr, "Skipping ...\n");
		void_file(info);
		return (FALSE);
	}
	if (!(interactive || allow_dotdot) && has_dotdot(info->f_name)) {
		if (!errhidden(E_SECURITY, info->f_name)) {
			if (!errwarnonly(E_SECURITY, info->f_name))
				xstats.s_security++;
			errmsgno(EX_BAD,
			"'%s' contains '..', skipping ...\n", info->f_name);
			(void) errabort(E_SECURITY, info->f_name, TRUE);
		}
		void_file(info);
		return (FALSE);
	}
	if (secure_links && (is_link(info) || is_symlink(info)) &&
	    (info->f_lname[0] == '/' || has_dotdot(info->f_lname))) {
		if (!errhidden(E_LSECURITY, info->f_lname)) {
			if (!errwarnonly(E_LSECURITY, info->f_lname))
				xstats.s_lsecurity++;
			errmsgno(EX_BAD,
			"'%s' potentially insecure link, skipping ...\n", info->f_name);
			(void) errabort(E_LSECURITY, info->f_lname, TRUE);
		}
		void_file(info);
		return (FALSE);
	}
	vprint(info);
	if (dorestore) {
		/*
		 * Check whether the target file exists and has a
		 * different type or is a dev node with different
		 * major/minor numbers. In this case, we need to
		 * remove the file. This happend when the original
		 * file has been removed and a new (different) file
		 * with the same name did get the same inode number.
		 */
		imp = sym_typecheck(info, &cinfo, imp);

	} else if (remove_first && !dometa) {
		/*
		 * With keep_old we do not come here.
		 *
		 * Even if the archive and the current node are both
		 * directories call remove_file() because the new dir
		 * may get our ownership this way if we are not root.
		 *
		 * In order to avoid annoying messages, call remove_file()
		 * only if the file exists.
		 */
		if (name_exists(info->f_name))
			(void) remove_file(info->f_name, TRUE);
	}
	if (is_dir(info)) {
#ifdef	MKD_DEBUG
		{ extern char *mkdwhy; mkdwhy = "extract"; }
#endif
		if (!make_adir(info)) {
			void_file(info);
			return (FALSE);
		}
		void_file(info);
	} else if (is_link(info)) {
		if (!make_link(info)) {
			void_file(info);
			return (FALSE);
		}
		void_file(info);
	} else if (is_symlink(info)) {
		if (dorestore && imp) {
			/*
			 * Do not create a new link if the old one is the same.
			 */
			if (cinfo.f_rxftype != XT_NONE && (cinfo.f_flags & F_SAME))
				goto set_modes;
		}
		if (!make_symlink(info)) {
			void_file(info);
			return (FALSE);
		}
		void_file(info);
	} else if (is_special(info)) {
		if (dorestore && imp) {
			/*
			 * Do not create a new node if the old one is the same.
			 */
			if (cinfo.f_rxftype != XT_NONE && (cinfo.f_flags & F_SAME))
				goto set_modes;
		}
		if (is_door(info)) {
			if (!nowarn) {
				errmsgno(EX_BAD,
				"WARNING: Extracting door '%s' as plain file.\n",
						info->f_name);
			}
			if (!get_file(info))
				return (FALSE);
		} else if (is_fifo(info)) {
			if (!make_fifo(info)) {
				void_file(info);
				return (FALSE);
			}
			void_file(info);
		} else {
			if (!make_special(info)) {
				void_file(info);
				return (FALSE);
			}
			void_file(info);
		}
	} else if (is_meta(info)) {
		FINFO	finfo;

		/*
		 * Make sure not to overwrite existing files.
		 */
		if (xmeta && !_getinfo(info->f_name, &finfo)) {
			if (!get_file(info))
				return (FALSE);
		} else {
			void_file(info);
		}
	} else if (!get_file(info)) {
		return (FALSE);
	}

#ifdef COPY_LINKS_DELAYED
	if ((copyhardlinks && is_link(info)) ||
	    (copysymlinks && is_symlink(info)))
		return (TRUE);
#endif
set_modes:
	if (!to_stdout)
		setmodes(info);
#ifdef	TEST_DEBUG
if (info->f_mode == 0700)
error("-->setmode(%s, %llo)\n", info->f_name, (Ullong)info->f_mode);
#endif
	if (dorestore)
		sym_addstat(info, imp);
	return (TRUE);
}

/*
 * Return TRUE if the file on disk is newer than the file on the archive.
 */
EXPORT BOOL
newer(info, cinfo)
	FINFO	*info;
	FINFO	*cinfo;
{

	if (uncond)
		return (FALSE);
	if (!_getinfo(info->f_name, cinfo)) {
		if (refresh_old) {
			errmsgno(EX_BAD, "file '%s' does not exists.\n", info->f_name);
			return (TRUE);
		}
		return (FALSE);
	}
	if (keep_old) {
		if (!nowarn)
			errmsgno(EX_BAD, "file '%s' exists.\n", info->f_name);
		return (TRUE);
	}
	if (xdot) {
		if (info->f_name[0] == '.' &&
		    (info->f_name[1] == '\0' ||
		    (info->f_name[1] == '/' &&
		    info->f_name[2] == '\0'))) {

			return (FALSE);
		}
	}

	/*
	 * XXX nsec beachten wenn im Archiv!
	 */
	if (cinfo->f_mtime >= info->f_mtime) {

	isnewer:
		if (xdir && is_dir(info))	/* Be silent, we handle it later */
			return (TRUE);
		if (!nowarn)
			errmsgno(EX_BAD, "current '%s' newer.\n", info->f_name);
		return (TRUE);
	} else if ((cinfo->f_mtime % 2) == 0 && (cinfo->f_mtime + 1) == info->f_mtime) {
		/*
		 * The DOS FAT filestem does only support a time granularity
		 * of 2 seconds. So we need to be a bit more generous.
		 * XXX We should be able to test the filesytem type.
		 */
		goto isnewer;
	}
	return (FALSE);
}

EXPORT BOOL
same_symlink(info)
	FINFO	*info;
{
	FINFO	finfo;
	char	lname[PATH_MAX+1];
	TCB	tb;

	finfo.f_lname = lname;
	finfo.f_lnamelen = 0;

	if (uncond || !_getinfo(info->f_name, &finfo))
		return (FALSE);

	/*
	 * Bei symlinks gehen nicht: lchmod lchtime & teilweise lchown
	 */
#ifdef	S_IFLNK
	if (!is_symlink(&finfo))	/* File on disk */
		return (FALSE);

	fillbytes(&tb, sizeof (TCB), '\0');
	info_to_tcb(&finfo, &tb);	/* XXX ist das noch nötig ??? */
					/* z.Zt. wegen linkflag/uname/gname */

	if (read_symlink(info->f_name, info->f_name, &finfo, &tb)) {
		if (streql(info->f_lname, finfo.f_lname)) {
			if (!nowarn)
				errmsgno(EX_BAD, "current '%s' is same symlink.\n",
								info->f_name);
			return (TRUE);
		}
	}
#ifdef	XXX
	/*
	 * XXX nsec beachten wenn im Archiv!
	 */
	if (finfo.f_mtime >= info->f_mtime) {
		if (!nowarn)
			errmsgno(EX_BAD, "current '%s' newer.\n", info->f_name);
		return (TRUE);
	}
#endif	/* XXX */

#endif
	return (FALSE);
}

EXPORT BOOL
same_special(info)
	FINFO	*info;
{
	FINFO	finfo;

	if (uncond || !_getinfo(info->f_name, &finfo))
		return (FALSE);

	if (info->f_xftype != finfo.f_xftype)
		return (FALSE);

	if (is_bdev(info) || is_cdev(info)) {
		if (info->f_rdevmaj != finfo.f_rdevmaj)
			return (FALSE);
		if (info->f_rdevmin != finfo.f_rdevmin)
			return (FALSE);
	}
	return (TRUE);
}

/*
 * Create intermediate directories.
 * If the user is not root and the umask is degenerated or read-only,
 * we add 0700 to the granted permissions. For this reason, we may need
 * to correct the permissins of intermediate directories later from the
 * directory stack.
 */
LOCAL BOOL
_create_dirs(name)
	register char	*name;
{
	mode_t	mode;

	mode = mode_mask;				/* used to be 0777 */
	if (my_uid != ROOT_UID)
		mode |= S_IRWXU;	/* Make sure we will be able write */

	if (mkdir(name, mode) < 0) {
		if (create_dirs(name) &&
		    mkdir(name, mode) >= 0) {
			_dir_setownwer(name);
			if (mode != mode_mask)
				sdirmode(name, mode_mask); /* Push umask */
			return (TRUE);
		}
		return (FALSE);
	}
	_dir_setownwer(name);
	if (mode != mode_mask)
		sdirmode(name, mode_mask);	/* Push umask on dirstack */
	return (TRUE);
}

/*
 * Set the owner/group of intermedia directories.
 * Be very careful not to overwrite sgid directory semantics.
 */
LOCAL void
_dir_setownwer(name)
	char	*name;
{
	FINFO	dinfo;

	if (my_uid != ROOT_UID)
		return;

	if (dir_uid == _BAD_UID && dir_gid == _BAD_GID)
		return;

	if (!_getinfo(name, &dinfo) || !is_dir(&dinfo))
		return;

	if (dir_uid != _BAD_UID)
		dinfo.f_uid = dir_uid;
	if (dir_gid != _BAD_GID)
		dinfo.f_gid = dir_gid;

	chown(name, dinfo.f_uid, dinfo.f_gid);
}

EXPORT BOOL
create_dirs(name)
	register char	*name;
{
	register char	*np;
	register char	*dp;
		int	err;
		int	olderr = 0;

	if (noxdir) {
		errmsgno(EX_BAD, "Directories not created.\n");
		return (FALSE);
	}
	np = dp = name;
	do {
		if (*np == '/')
			dp = np;
	} while (*np++);
	if (dp == name) {
		/*
		 * Do not create the very last directory
		 */
		return (TRUE);
	}
	*dp = '\0';
	if (access(name, 0) < 0) {
		if (_create_dirs(name)) {
			*dp = '/';
			return (TRUE);
		}
		err = geterrno();
		if ((err == EACCES || is_eexist(err))) {
			olderr = err;
			goto exists;
		}
		*dp = '/';
		return (FALSE);
	} else {
		FINFO	dinfo;

	exists:
		if (_getinfo(name, &dinfo)) {
			if (is_dir(&dinfo)) {
				*dp = '/';
				return (TRUE);
			}

			if (remove_file(name, FALSE)) {
				if (_create_dirs(name)) {
					*dp = '/';
					return (TRUE);
				}
				*dp = '/';
				return (FALSE);
			} else {
				*dp = '/';
				return (FALSE);
			}
		}
		*dp = '/';
		if (olderr == EACCES)
			seterrno(olderr);
		return (FALSE);
	}
}

#ifdef	MKD_DEBUG
EXPORT char	*mkdwhy;
#endif

EXPORT BOOL
make_adir(info)
	FINFO	*info;
{
	if (is_link(info))
		return (make_link(info));
	else
		return (make_dir(info));
}

/*
 * This function is used only to create directories found on the archive.
 * Intermediate directories are created using create_dirs().
 */
LOCAL BOOL
make_dir(info)
	FINFO	*info;
{
	FINFO	dinfo;
	mode_t	mode;
	int	err;

	if (dometa)
		return (TRUE);

	mode = osmode(info->f_mode);	/* Convert TAR modes to OS modes */
	mode &= mode_mask;		/* Apply current umask */
	if (my_uid != ROOT_UID)
		mode |= S_IRWXU;	/* Make sure we will be able write */

	if (_getinfo(info->f_name, &dinfo) && is_dir(&dinfo))
		return (TRUE);

	if (create_dirs(info->f_name)) {
		if (_getinfo(info->f_name, &dinfo) && is_dir(&dinfo))
			return (TRUE);
		if (mkdir(info->f_name, mode) >= 0)
			return (TRUE);
		err = geterrno();
		if ((err == EACCES || is_eexist(err)) &&
				remove_file(info->f_name, FALSE)) {
			if (mkdir(info->f_name, mode) >= 0)
				return (TRUE);
		}
	}
#ifdef	MKD_DEBUG
	errmsgno(EX_BAD, "make_dir(%s) called from '%s'\n", info->f_name, mkdwhy);
#endif
	if (!errhidden(E_OPEN, info->f_name)) {
		if (!errwarnonly(E_OPEN, info->f_name))
			xstats.s_openerrs++;
		errmsg("Cannot make dir '%s'.\n", info->f_name);
		(void) errabort(E_OPEN, info->f_name, TRUE);
	}
	return (FALSE);
}

LOCAL BOOL
make_link(info)
	FINFO	*info;
{
	int	err;
#ifdef	USE_FFLAGS
	FINFO	linfo;
	Ulong	oldflags = 0L;
#endif
	char	xname[PATH_MAX+1];
	char	*name = info->f_name;

	/*
	 * void_file() is needed for CPIO and done by our callers.
	 */

	if (dometa)
		return (TRUE);

#ifdef	HAVE_LINK_NOFOLLOW
	/*
	 * This OS allows hard links to symlinks and does not follow symlinks
	 * when making hard links to symlinks. We may not follow symlinks while
	 * we check if source & sestination are the same.
	 */
	if (same_file(info, IS_LINK, FALSE)) {
#else
	/*
	 * This OS either does not allow hard links to symlinks or follows
	 * symlinks if possible before making a hard link to a symlink.
	 * We need not follow symlinks while we check if source & sestination
	 * are the same.
	 */
	if (same_file(info, IS_LINK, TRUE)) {
#endif
		/*
		 * As it seems that from/to for the hard link are already
		 * identical files, return TRUE to indicate success with
		 * creating the hard link.
		 */
		return (TRUE);
	}

	if (copyhardlinks)
		return (make_copy(info, FALSE, 0));
	else if (hardlinks)
		return (emul_link(info));

#ifdef	HAVE_LINK
	xname[0] = '\0';
	if (do_install && name_exists(name)) {
		if (!file_tmpname(info, xname))
			return (FALSE);
		name = xname;
	}
	if (link(info->f_lname, name) >= 0)
		goto ok;
	err = geterrno();
	if (info->f_rsize > 0 && is_enoent(err))
		return (get_file(info));
#ifdef	USE_FFLAGS
	/*
	 * SF_IMMUTABLE might be set on the source-file. Clear the flags
	 * and try again.
	 */
	if (_getinfo(info->f_lname, &linfo) && (linfo.f_xflags & XF_FFLAGS)) {
		oldflags = linfo.f_fflags;
		linfo.f_fflags = 0L;
		set_fflags(&linfo);
		if (link(info->f_lname, name) >= 0)
			goto restore_flags;
		err = geterrno();
	}
#endif
	if (create_dirs(info->f_name)) {
		if (link(info->f_lname, name) >= 0)
			goto restore_flags;
		err = geterrno();
	}
	if ((err == EACCES || is_eexist(err)) &&
			remove_file(name, FALSE)) {
		if (link(info->f_lname, name) >= 0)
			goto restore_flags;
	}
	if (!errhidden(E_OPEN, info->f_name)) {
		if (!errwarnonly(E_OPEN, info->f_name))
			xstats.s_openerrs++;
		errmsg("Cannot link '%s' to '%s'.\n",
				info->f_name, info->f_lname);
		(void) errabort(E_OPEN, info->f_name, TRUE);
	}
	if (do_install)
		remove_tmpname(xname);
	return (FALSE);

restore_flags:
#ifdef	USE_FFLAGS
	if (oldflags != 0L) {
		linfo.f_fflags = oldflags;
		set_fflags(&linfo);
	}
#endif
ok:
	if (do_install)
		return (install_rename(info, xname));
	return (TRUE);

#else	/* HAVE_LINK */
	if (!errhidden(E_SPECIALFILE, info->f_name)) {
		if (!errwarnonly(E_SPECIALFILE, info->f_name))
			xstats.s_isspecial++;
		errmsgno(EX_BAD,
		"Not supported. Cannot link '%s' to '%s'.\n",
						info->f_name, info->f_lname);
		(void) errabort(E_SPECIALFILE, info->f_name, TRUE);
	}
	return (FALSE);
#endif	/* HAVE_LINK */
}

LOCAL BOOL
make_symlink(info)
	FINFO	*info;
{
	int	err;
	char	xname[PATH_MAX+1];
	char	*name = info->f_name;

	if (dometa)
		return (TRUE);

	if (copysymlinks)
		return (make_copy(info, TRUE, 0));
	else if (symlinks)
		return (emul_symlink(info));

#ifdef	S_IFLNK
	xname[0] = '\0';
	if (do_install && name_exists(name)) {
		if (!file_tmpname(info, xname))
			return (FALSE);
		name = xname;
	}
	if (sxsymlink(name, info) >= 0)
		goto ok;
	err = geterrno();
	if (create_dirs(info->f_name)) {
		if (sxsymlink(name, info) >= 0)
			goto ok;
		err = geterrno();
	}
	/*
	 * XXX at least with same symlinks we should return success
	 */
	if ((err == EACCES || is_eexist(err)) &&
			remove_file(name, FALSE)) {
		if (sxsymlink(name, info) >= 0)
			goto ok;
	}
	if (!errhidden(E_OPEN, info->f_name)) {
		if (!errwarnonly(E_OPEN, info->f_name))
			xstats.s_openerrs++;
		errmsg("Cannot create symbolic link '%s' to '%s'.\n",
						info->f_name, info->f_lname);
		(void) errabort(E_OPEN, info->f_name, TRUE);
	}
	if (do_install)
		remove_tmpname(xname);
	return (FALSE);
ok:
	if (do_install)
		return (install_rename(info, xname));
	return (TRUE);
#else	/* S_IFLNK */
	if (!errhidden(E_SPECIALFILE, info->f_name)) {
		if (!errwarnonly(E_SPECIALFILE, info->f_name))
			xstats.s_isspecial++;
		errmsgno(EX_BAD,
		"Not supported. Cannot create symbolic link '%s' to '%s'.\n",
						info->f_name, info->f_lname);
		(void) errabort(E_SPECIALFILE, info->f_name, TRUE);
	}
	return (FALSE);
#endif	/* S_IFLNK */
}

LOCAL BOOL
emul_symlink(info)
	FINFO	*info;
{
	errmsgno(EX_BAD, "Option -symlinks not yet implemented.\n");
	errmsgno(EX_BAD, "Cannot create symbolic link '%s' to '%s'.\n",
						info->f_name, info->f_lname);
	return (FALSE);
}

LOCAL BOOL
emul_link(info)
	FINFO	*info;
{
	errmsgno(EX_BAD, "Option -hardlinks not yet implemented.\n");
	errmsgno(EX_BAD, "Cannot link '%s' to '%s'.\n", info->f_name, info->f_lname);
#ifdef	HAVE_LINK
	return (FALSE);
#else
	return (FALSE);
#endif	/* S_IFLNK */
}

LOCAL BOOL
same_file(info, what, do_follow)
	FINFO	*info;
	int	what;
	BOOL	do_follow;
{
	FINFO	finfo;
	FINFO	linfo;
	BOOL	ofollow = follow;
	BOOL	ret = FALSE;

	follow = do_follow;
	if (_getinfo(info->f_name, &finfo) && _getinfo(info->f_lname, &linfo)) {
		if (finfo.f_dev == linfo.f_dev && finfo.f_ino == linfo.f_ino) {
			if (what == IS_COPY) {
				if (!errhidden(E_SAMEFILE, info->f_lname)) {
					if (!errwarnonly(E_SAMEFILE, info->f_lname))
						xstats.s_samefile++;
					errmsgno(EX_BAD,
					"copy_file: '%s' from/to identical, skipping ...\n",
						info->f_name);
					(void) errabort(E_SAMEFILE,
							info->f_lname, TRUE);
				}
			} else {
				/*
				 * If in restore mode, we do not like to see
				 * this informational message. The hard link
				 * is already present and this is all we need.
				 * If -force-remove has been specified (default
				 * for "tar") we do not like to see this message
				 * either.
				 */
				if (!nowarn && !dorestore && !force_remove) {
					errmsgno(EX_BAD,
					"Notice: link_file: '%s' from/to identical, skipping ...\n",
						info->f_name);
				}
			}
			ret = TRUE;
		}
	}
	follow = ofollow;
	return (ret);
}

LOCAL BOOL
make_copy(info, do_symlink, eflags)
	FINFO	*info;
	BOOL	do_symlink;
	int	eflags;
{
#ifdef COPY_LINKS_DELAYED
	if (!lowmem) {
		add_copy(info, do_symlink);
		return (TRUE);
	} else {
		return (_make_copy(info, do_symlink, eflags));
	}
#else
	return (_make_copy(info, do_symlink, eflags));
#endif
}

LOCAL BOOL
_make_copy(info, do_symlink, eflags)
	FINFO	*info;
	BOOL	do_symlink;
	int	eflags;
{
	int	ret;
	int	err;

#ifdef COPY_LINKS_DELAYED
	if (!lowmem && copydlinks) {
		if (_make_dcopy(info, do_symlink, &ret, eflags))
			return (ret);
	}
#endif
	/*
	 * As we can only copy plain files, we need to follow symlinks when
	 * we check if source & destination are the same file.
	 */
	if (same_file(info, IS_COPY, TRUE)) {
		return (FALSE);
	}

	if ((ret = copy_file(info->f_lname, info->f_name, do_symlink, eflags)) >= 0)
		return (TRUE);
	err = geterrno();
	if (ret != -2 && create_dirs(info->f_name)) {
		if (copy_file(info->f_lname, info->f_name, do_symlink, eflags) >= 0)
			return (TRUE);
		err = geterrno();
	}
	if ((err == EACCES || is_eexist(err) || err == EISDIR) &&
			remove_file(info->f_name, FALSE)) {
		if (copy_file(info->f_lname, info->f_name, do_symlink, eflags) >= 0)
			return (TRUE);
	}
	if (!errhidden(E_OPEN, info->f_name) &&
	    ((eflags & HIDE_ENOENT) == 0 || geterrno() != ENOENT)) {
		if (!errwarnonly(E_OPEN, info->f_name))
			xstats.s_openerrs++;
		errmsg("Cannot create link copy '%s' from '%s'.\n",
					info->f_name, info->f_lname);
		(void) errabort(E_OPEN, info->f_name, TRUE);
	}
	return (FALSE);
}

#ifdef COPY_LINKS_DELAYED
LOCAL void
add_copy(info, do_symlink)
	FINFO	*info;
	BOOL	do_symlink;
{
	MCQ	*mcqp	   = ___malloc(sizeof (MCQ), "make_copy()");
	char	*f_namep   = ___savestr(info->f_name);
	char	*f_lnamep  = ___savestr(info->f_lname);

	mcqp->next	   = NULL;
	mcqp->do_symlink   = do_symlink;
	mcqp->info	   = *info;
	mcqp->info.f_name  = f_namep;
	mcqp->info.f_lname = f_lnamep;

	if (mcq_last) {
		mcq_last->next = mcqp;
		mcq_last = mcqp;
	} else {
		mcq_1st = mcqp;
		mcq_last = mcqp;
	}
}

LOCAL void
make_copies()
{
	int	eflags	= HIDE_ENOENT;

	do {
		MCQ	*mcqp		= mcq_1st;
		MCQ	*mcqp_prev	= NULL;
		int	mcq_removed	= 0;

		while (mcqp) {
			MCQ	*mcqp_save	= mcqp;
			BOOL	ret		= _make_copy(&mcqp->info, mcqp->do_symlink, eflags);

			if (ret) {
				if (!to_stdout)
					setmodes(&mcqp->info);
				if (dorestore)
					sym_addstat(&mcqp->info, NULL);
			}
			mcqp = mcqp->next;

			if (ret || (eflags & HIDE_ENOENT) == 0 || geterrno() != ENOENT) {

				if (mcqp_prev)
					mcqp_prev->next = mcqp;
				if (mcq_1st == mcqp_save)
					mcq_1st = mcqp;
				if (mcq_last == mcqp_save)
					mcq_last = mcqp;

				free(mcqp_save->info.f_name);
				free(mcqp_save->info.f_lname);
				free(mcqp_save);

				mcq_removed++;

			} else {
				mcqp_prev = mcqp_save;
			}
		}
		if (!mcq_removed)
			eflags = 0;	/* queue has not decreased - last attempt */

	} while (mcq_1st);
}

LOCAL BOOL
_make_dcopy(info, do_symlink, retp, eflags)
	FINFO	*info;
	BOOL	do_symlink;
	int	*retp;
	int	eflags;
{
	char	nbuf[PATH_MAX+1];
	char	*dir = info->f_lname;
	char	*dp;
	int	nents;
	int	ret = TRUE;
	FINFO	ninfo;

	if (do_symlink && info->f_lname[0] != '/') {

		char	*p = strrchr(info->f_name, '/');
		int	len;

		if (p) {
			len = p - info->f_name + 1;
			strncpy(nbuf, info->f_name, len);
			if ((len + strlen(info->f_lname)) > PATH_MAX) {
				if (!errhidden(E_NAMETOOLONG, info->f_lname)) {
					if (!errwarnonly(E_NAMETOOLONG, info->f_lname))
						xstats.s_toolong++;
					errmsgno(EX_BAD,
					"Name too long. Cannot copy from '%s'.\n",
					info->f_lname);
					(void) errabort(E_NAMETOOLONG,
							info->f_lname, TRUE);
				}
				if (retp)
					*retp = FALSE;
				return (TRUE);
			}
			strcpy(&nbuf[len], info->f_lname);
			dir = nbuf;
		}
	}

	if ((dp = fetchdir(dir, &nents, NULL, NULL)) == NULL)
		return (FALSE);

	if (!_getinfo(info->f_name, &ninfo)) {
		_getinfo(dir, &ninfo);
		ninfo.f_name = info->f_name;
		make_dir(&ninfo);
	}

	while (nents > 0) {
		char	*name;
		int	nlen;

		name = &dp[1];
		nlen = strlen(name);

		ninfo.f_name = ___malloc(strlen(info->f_name) +
					1 + nlen + 1, "make_copy()");

		strcpy(ninfo.f_name, info->f_name);
		if (ninfo.f_name[strlen(ninfo.f_name)-1] != '/')
			strcat(ninfo.f_name, "/");
		strcat(ninfo.f_name, name);

		ninfo.f_lname = ___malloc(3 +
					strlen(info->f_lname) +	1 +
					nlen + 1, "make_copy()");

		ninfo.f_lname[0] = '\0';
		if (do_symlink)
			strcpy(ninfo.f_lname, "../");
		strcat(ninfo.f_lname, info->f_lname);
		if (ninfo.f_lname[strlen(ninfo.f_lname)-1] != '/')
			strcat(ninfo.f_lname, "/");
		strcat(ninfo.f_lname, name);

		if (!_make_copy(&ninfo, do_symlink, eflags))
			ret = FALSE;

		free(ninfo.f_lname);
		free(ninfo.f_name);

		nents--;
		dp += nlen +2;
	}
	if (retp)
		*retp = ret;
	return (TRUE);
}
#endif	/* COPY_LINKS_DELAYED */

LOCAL int
copy_file(from, to, do_symlink, eflags)
	char	*from;
	char	*to;
	BOOL	do_symlink;
	int	eflags;
{
	FINFO	finfo;
	FILE	*fin;
	FILE	*fout;
	int	cnt = -1;
	char	buf[8192];
	char	nbuf[PATH_MAX+1];

	/*
	 * When tar archives hard links, both names (from/to) are relative to
	 * the current directory. With symlinks this does not work. Symlinks
	 * are always evaluated relative to the directory they reside in.
	 * For this reason, we cannot simply open the from/to files if we
	 * like to emulate a symbolic link. To emulate the behavior of a
	 * symbolic link, we concat the the directory part of the 'to' name
	 * (which is the path that becomes the sombolic link) to the complete
	 * 'from' name (which is the path the symbolic linkc pints to) in case
	 * the 'from' name is a relative path name.
	 */
	if (do_symlink && from[0] != '/') {
		char	*p = strrchr(to, '/');
		int	len;

		if (p) {
			len = p - to + 1;
			strncpy(nbuf, to, len);
			if ((len + strlen(from)) > PATH_MAX) {
				if (!errhidden(E_NAMETOOLONG, from)) {
					if (!errwarnonly(E_NAMETOOLONG, from))
						xstats.s_toolong++;
					errmsgno(EX_BAD,
					"Name too long. Cannot copy from '%s'.\n",
					from);
					(void) errabort(E_NAMETOOLONG, from,
									TRUE);
				}
				return (-2);
			}
			strcpy(&nbuf[len], from);
			from = nbuf;
		}
	}
	if (!_getinfo(from, &finfo)) {

		if (!errhidden(E_STAT, from) &&
		    ((eflags & HIDE_ENOENT) == 0 || geterrno() != ENOENT)) {
			if (!errwarnonly(E_STAT, from))
				xstats.s_staterrs++;
			errmsg("Cannot stat '%s'.\n", from);
			(void) errabort(E_STAT, from, TRUE);
		}
		return (-2);
	}
	if (!is_file(&finfo)) {
		errmsgno(EX_BAD, "Not a file. Cannot copy from '%s'.\n", from);
		seterrno(EINVAL);
		return (-2);
	}

	if ((fin = fileopen(from, "rub")) == 0) {
		errmsg("Cannot open '%s'.\n", from);
	} else {
		if ((fout = fileopen(to, "wtcub")) == 0) {
/*			errmsg("Cannot create '%s'.\n", to);*/
			return (-1);
		} else {
			while ((cnt = ffileread(fin, buf, sizeof (buf))) > 0)
				ffilewrite(fout, buf, cnt);
			fclose(fout);
		}
		fclose(fin);
	}
	return (cnt);
}

LOCAL BOOL
make_fifo(info)
	FINFO	*info;
{
	mode_t	mode;
	int	err;
	char	xname[PATH_MAX+1];
	char	*name = info->f_name;

	if (dometa)
		return (TRUE);

#ifdef	HAVE_MKFIFO
	xname[0] = '\0';
	if (do_install && name_exists(name)) {
		if (!file_tmpname(info, xname))
			return (FALSE);
		name = xname;
	}
	mode = osmode(info->f_mode);
	mode &= mode_mask;
	if (mkfifo(name, mode) >= 0)
		goto ok;
	err = geterrno();
	if (create_dirs(info->f_name)) {
		if (mkfifo(name, mode) >= 0)
			goto ok;
		err = geterrno();
	}
	if ((err == EACCES || is_eexist(err)) &&
			remove_file(name, FALSE)) {
		if (mkfifo(name, mode) >= 0)
			goto ok;
	}
	if (!errhidden(E_OPEN, info->f_name)) {
		if (!errwarnonly(E_OPEN, info->f_name))
			xstats.s_openerrs++;
		errmsg("Cannot make fifo '%s'.\n", info->f_name);
		(void) errabort(E_OPEN, info->f_name, TRUE);
	}
	if (do_install)
		remove_tmpname(xname);
	return (FALSE);
ok:
	if (do_install)
		return (install_rename(info, xname));
	return (TRUE);
#else
#ifdef	HAVE_MKNOD
	return (make_special(info));
#endif
	if (!errhidden(E_SPECIALFILE, info->f_name)) {
		if (!errwarnonly(E_SPECIALFILE, info->f_name))
			xstats.s_isspecial++;
		errmsgno(EX_BAD,
			"Not supported. Cannot make fifo '%s'.\n",
							info->f_name);
		(void) errabort(E_SPECIALFILE, info->f_name, TRUE);
	}
	return (FALSE);
#endif
}

LOCAL BOOL
make_special(info)
	FINFO	*info;
{
	mode_t	mode;
	dev_t	dev;
	int	err;
	char	xname[PATH_MAX+1];
	char	*name = info->f_name;

	if (dometa)
		return (TRUE);

#ifdef	HAVE_MKNOD
	xname[0] = '\0';
	if (do_install && name_exists(name)) {
		if (!file_tmpname(info, xname))
			return (FALSE);
		name = xname;
	}
	mode = osmode(info->f_mode);
	mode &= mode_mask;
	mode |= info->f_type;	/* Add file type bits */
	dev = info->f_rdev;
	if (mknod(name, mode, dev) >= 0)
		goto ok;
	err = geterrno();
	if (create_dirs(info->f_name)) {
		if (mknod(name, mode, dev) >= 0)
			goto ok;
		err = geterrno();
	}
	if ((err == EACCES || is_eexist(err)) &&
			remove_file(name, FALSE)) {
		if (mknod(name, mode, dev) >= 0)
			goto ok;
	}
	if (!errhidden(E_OPEN, info->f_name)) {
		if (!errwarnonly(E_OPEN, info->f_name))
			xstats.s_openerrs++;
		errmsg("Cannot make %s '%s'.\n",
					is_fifo(info)?"fifo":"special",
							info->f_name);
		(void) errabort(E_OPEN, info->f_name, TRUE);
	}
	if (do_install)
		remove_tmpname(xname);
	return (FALSE);
ok:
	if (do_install)
		return (install_rename(info, xname));
	return (TRUE);
#else
	if (!errhidden(E_SPECIALFILE, info->f_name)) {
		if (!errwarnonly(E_SPECIALFILE, info->f_name))
			xstats.s_isspecial++;
		errmsgno(EX_BAD, "Not supported. Cannot make %s '%s'.\n",
					is_fifo(info)?"fifo":"special",
							info->f_name);
		(void) errabort(E_SPECIALFILE, info->f_name, TRUE);
	}
	return (FALSE);
#endif
}

/*
 * Create a temporary path name for the extraction in -install mode.
 */
LOCAL BOOL
file_tmpname(info, xname)
	FINFO	*info;
	char	*xname;
{
	register char	*xp = xname;
	register char	*np;
	register char	*dp;

	np = info->f_name;
	dp = xp;
	do {
		if ((*xp++ = *np) == '/')
			dp = xp;
	} while (*np++);
	if ((dp - xname) >= (PATH_MAX-6)) {
		errmsgno(ENAMETOOLONG,
				"Cannot make temporary name for '%s'.\n",
				info->f_name);
		return (FALSE);
	}
	strcpy(dp, "XXXXXX");
	seterrno(0);
	mktemp(xname);
	if (xname[0] == '\0') {
		errmsg("Cannot make temporary name for '%s'.\n",
				info->f_name);
		return (FALSE);
	}
	return (TRUE);
}

LOCAL FILE *
file_open(info, name)
	FINFO	*info;
	char	*name;
{
	return (filemopen(name, "wctub", osmode(info->f_mode) & mode_mask));
}

/*
 * Rename the temporary path to the official path name when in -install mode.
 */
LOCAL BOOL
install_rename(info, xname)
	FINFO	*info;
	char	*xname;
{
	int	err;
	BOOL	oforce_remove = force_remove;

	/*
	 * If xname is empty, then we do not need to rename the file as
	 * there was no temporary name.
	 */
	if (xname[0] == '\0')
		return (TRUE);

	if (rename(xname, info->f_name) >= 0)
		return (TRUE);
	err = geterrno();
	/*
	 * EISDIR is the error code if we try to rename a non-directory to the
	 * name of an existing directory. In this case we silently remove this
	 * directory if it is empty.
	 */
	if (err == EISDIR)
		force_remove = TRUE;
	if ((err == EACCES || is_eexist(err) || err == EISDIR) &&
					remove_file(info->f_name, FALSE)) {
		if (rename(xname, info->f_name) >= 0) {
			force_remove = oforce_remove;
			return (TRUE);
		}
	}
	force_remove = oforce_remove;
	/*
	 * Rename to the official name did not work, remove the temporary name
	 */
	remove_tmpname(xname);
	return (FALSE);
}

LOCAL BOOL
name_exists(name)
	char	*name;
{
	FINFO	finfo;
	BOOL	ofollow = follow;

	follow = FALSE;
	if (!_getinfo(name, &finfo)) {
		follow = ofollow;
		return (FALSE);
	}
	follow = ofollow;
	return (TRUE);
}

/*
 * remove_tmpname() is used to remove the temporary file used with -install
 * in case that the extraction did fail. For this reason make the remove
 * silent and unconditionally.
 */
LOCAL void
remove_tmpname(name)
	char	*name;
{
	BOOL	oforce_remove = force_remove;
	BOOL	oremove_recursive = remove_recursive;

	/*
	 * Rename to the official name did not work, remove the temporary name
	 * in case that the temporary file still exists.
	 */
	if (name[0] == '\0')
		return;
	if (!name_exists(name))
		return;
	/*
	 * In order to avoid annoying messages, call remove_file()
	 * only if the file exists.
	 */
	force_remove = TRUE;
	remove_recursive = TRUE;
	remove_file(name, FALSE);
	remove_recursive = oremove_recursive;
	force_remove = oforce_remove;
}

LOCAL BOOL
get_file(info)
		FINFO	*info;
{
		FILE	*f;
		int	err;
		char	xname[PATH_MAX+1];
		char	*name = info->f_name;

	if (dometa) {
		void_file(info);
		return (TRUE);
	}

	if (to_stdout) {
		f = stdout;
		goto ofile;
	}
	xname[0] = '\0';
	if (do_install && name_exists(name)) {
		if (!file_tmpname(info, xname))
			return (FALSE);
		name = xname;
	}
	if ((f = file_open(info, name)) == (FILE *)NULL) {
		err = geterrno();
		if (err == EMISSDIR && create_dirs(info->f_name)) {
			if ((f = file_open(info, name)) != (FILE *)NULL) {
				goto ofile;
			}
			err = geterrno();
		}
		if ((err == EACCES || is_eexist(err) || err == EISDIR) &&
					remove_file(name, FALSE)) {
			if ((f = file_open(info, name)) != (FILE *)NULL) {
				goto ofile;
			}
		}

		if (!errhidden(E_OPEN, info->f_name)) {
			if (!errwarnonly(E_OPEN, info->f_name))
				xstats.s_openerrs++;
			errmsg("Cannot create '%s'.\n", info->f_name);
			(void) errabort(E_OPEN, info->f_name, TRUE);
		}
		void_file(info);
		return (FALSE);
	}
ofile:
	if (!get_ofile(f, info)) {
		if (!to_stdout && do_install)
			remove_tmpname(xname);
		return (FALSE);
	}
	if (!to_stdout && do_install)
		return (install_rename(info, xname));
	return (TRUE);
}

LOCAL BOOL
get_ofile(f, info)
		FILE	*f;
		FINFO	*info;
{
		int	err;
		int	ret;

	file_raise(f, FALSE);

#if	defined(F_GETFL) && defined(O_DSYNC)
	/*
	 * Try to write file data as soon as possible to avoid
	 * longer wait when fsync() is called later.
	 */
	if (!no_fsync) {
		int	fl;

		fl = fcntl(fdown(f), F_GETFL, 0);
		fl |= O_DSYNC;
		fcntl(fdown(f), F_SETFL, fl);
	}
#endif

	if (is_sparse(info)) {
		ret = get_sparse(f, info);
	} else if (force_hole) {
		ret = get_forced_hole(f, info);
	} else {
		ret = xt_file(info, (int(*)__PR((void *, char *, int)))ffilewrite,
						f, 0, "writing");
	}
	if (ret < 0) {
		snulltimes(info->f_name, info);
		die(EX_BAD);
	}
	if (!to_stdout) {
#ifdef	HAVE_FSYNC
		int	cnt;
#endif
		if (ret == FALSE)
			xstats.s_rwerrs--;	/* Compensate overshoot below */

		if (fflush(f) != 0)
			ret = FALSE;
#ifdef	HAVE_FSYNC
		err = 0;
		cnt = 0;
		do {
			if (!no_fsync && fsync(fdown(f)) != 0)
				err = geterrno();

			if (err == EINVAL)
				err = 0;
		} while (err == EINTR && ++cnt < 10);
		if (err != 0)
			ret = FALSE;
#endif
		if (fclose(f) != 0)
			ret = FALSE;
		if (ret == FALSE) {
			xstats.s_rwerrs++;
			snulltimes(info->f_name, info);
		}
	}
	return (ret);
}

/* ARGSUSED */
LOCAL int
void_func(vp, p, amount)
	void	*vp;
	char	*p;
	int	amount;
{
	return (amount);
}

EXPORT BOOL
void_file(info)
		FINFO	*info;
{
	int	ret = TRUE;
	Ullong	llsize = info->f_llsize;
	off_t	size   = info->f_rsize;

	/*
	 * handle botch in gnu sparse file definitions
	 */
	if (props.pr_flags & PR_GNU_SPARSE_BUG)
		if (gnu_skip_extended(info) < 0)
			die(EX_BAD);

	if (info->f_flags & F_DATA_SKIPPED)
		return (ret);

	/*
	 * Try to do our best to skip even files with a size that
	 * is more then off_t may handle on the local machine.
	 */
	do {
		if (info->f_flags & F_BAD_SIZE) {
			if (llsize > 1024*1024*1024)
				info->f_rsize = 1024*1024*1024;
			else
				info->f_rsize = llsize;
		}

		ret = xt_file(info, void_func, 0, 0, "void");
		if (ret < 0)
			die(EX_BAD);

		llsize -= info->f_rsize;

	} while ((info->f_flags & F_BAD_SIZE) && llsize > 0);

	info->f_rsize = size;
	info->f_flags |= F_DATA_SKIPPED;

	return (ret);
}

LOCAL BOOL
void_bad(info)
		FINFO	*info;
{
	int	ret;

	if (!nowarn)
		errmsgno(EX_BAD,
			"WARNING: bad metadata for '%s', skipping...\n",
			info->f_name);
	ret = void_file(info);
	return (ret);
}

/*
 * Extract file using callback function "func"
 * Returns:
 *	TRUE	Extract OK
 *	FALSE	Extract not OK, may continue
 *	-1	An error occured, max not continue
 */
EXPORT int
xt_file(info, func, arg, amt, text)
		FINFO	*info;
		int	(*func) __PR((void *, char *, int));
		void	*arg;
		int	amt;
		char	*text;
{
	register int	amount; /* XXX ??? */
	register off_t	size;
	register int	tasize;
		BOOL	ret = TRUE;

	size = info->f_rsize;
	if (amt == 0)
		amt = bufsize;
	while (size > 0) {

		if ((props.pr_flags & PR_CPIO) == 0) {
			amount = buf_rwait(TBLOCK);
			if (amount < TBLOCK) {
				goto waseof;
			}
			amount = (amount / TBLOCK) * TBLOCK;
			amount = min(size, amount);
			amount = min(amount, amt);
			tasize = tarsize(amount);
		} else {
			amount = buf_rwait(1);	/* Request what is available */
			if (amount <= 0) {
				goto waseof;
			}
			amount = min(size, amount);
			amount = min(amount, amt);
			tasize = amount;
		}

		if ((*func)(arg, bigptr, amount) != amount) {
			ret = FALSE;
			if (!errhidden(E_WRITE, info->f_name)) {
				if (!errwarnonly(E_WRITE, info->f_name))
					xstats.s_rwerrs++;
				errmsg("Error %s '%s'.\n", text, info->f_name);
				(void) errabort(E_WRITE, info->f_name, TRUE);
			}
			/*
			 * func -> void_func() to skip the rest of the file.
			 */
			func = void_func;
		}

		size -= amount;
		buf_rwake(tasize);
	}
	info->f_flags |= F_DATA_SKIPPED;
	/*
	 * Honour CPIO padding
	 */
	if ((amount = props.pr_pad) != 0) {
		size = info->f_rsize;
		if (info->f_flags & F_LONGNAME)
			size += props.pr_hdrsize;
		amount = (amount + 1 - (size & amount)) & amount;
		if (amount > 0) {
			buf_rwait(amount);
			buf_rwake(amount);
		}
	}
	return (ret);
waseof:
	errmsgno(EX_BAD, "Tar file too small (amount: %d bytes).\n", amount);
	errmsgno(EX_BAD, "Unexpected EOF on input.\n");
	return (-1);
}

EXPORT void
skip_slash(info)
	FINFO	*info;
{
	static	BOOL	warned = FALSE;

	if (!warned && !nowarn) {
		errmsgno(EX_BAD, "WARNING: skipping leading '/' on filenames.\n");
		warned = TRUE;
	}
	/*
	 * XXX
	 * XXX ACHTUNG: ia_change kann es nötig machen, den String umzukopieren
	 * XXX denn sonst ist die Länge des Speicherplatzes unbestimmt!
	 *
	 * XXX ACHTUNG: mir ist noch unklar, ob es richtig ist, auch in jedem
	 * XXX Fall Führende slashes vom Linknamen zu entfernen.
	 * XXX Bei Hard-Link ist das sicher richtig und ergibt sich auch
	 * XXX automatisch, wenn man nur vor dem Aufruf von skip_slash()
	 * XXX auf f_name[0] == '/' abfragt.
	 */
	while (info->f_name[0] == '/')
		info->f_name++;

	/*
	 * Don't strip leading '/' from targets of symlinks.
	 */
	if (is_symlink(info))
		return;

	while (info->f_lname[0] == '/')
		info->f_lname++;
}

LOCAL BOOL
has_dotdot(name)
	char	*name;
{
	register char	*p = name;

	while (*p) {
		if ((p[0] == '.' && p[1] == '.') &&
		    (p[2] == '/' || p[2] == '\0')) {
			return (TRUE);
		}
		do {
			if (*p++ == '\0')
				return (FALSE);
		} while (*p != '/');
		p++;
		while (*p == '/')	/* Skip multiple slashes */
			p++;
	}
	return (FALSE);
}
