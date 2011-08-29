/* @(#)restore.c	1.64 11/08/14 Copyright 2003-2011 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)restore.c	1.64 11/08/14 Copyright 2003-2011 J. Schilling";
#endif
/*
 *	Data base management for incremental restores
 *	needed to detect and execute rename() and unlink()
 *	operations between two incremental dumps.
 *
 *	Copyright (c) 2003-2011 J. Schilling
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

/*
 *	Problems:
 *
 *	-	lost+found will usually not extracted and thus
 *		get no new inode number.
 *		**** Solution: Use -U ****
 *
 *	-	chmod(1) only changes st_ctime but star will
 *		extract neither data nor permissions if only
 *		a chmod(1), chown(1) or smilar change did ocur.
 *		**** Solution: Use -U ****
 *
 *	-	unlink(dev) .... create(dev) and get same inode #
 *		**** Solution: sym_typecheck() ****
 *		The Solution creates a new problem that the new
 *		node created after sym_typecheck() may have a
 *		different inode number and we need to change the node.
 */

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include "star.h"
#include "props.h"
#include "table.h"
#include "diff.h"
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/dirent.h>	/* XXX Wegen S_IFLNK */
#include <schily/stat.h>	/* XXX Wegen S_IRWXU */
#include <schily/errno.h>	/* XXX Wegen ENOENT */
#include "restore.h"
#include "dumpdate.h"
#include "starsubs.h"
#include <schily/fetchdir.h>

#define	PADD_NODE_DEBUG

#define	PRINT_L_SYM		/* Include symbols for star_sym.c */

extern	BOOL	debug;
extern	int	xdebug;
extern	int	verbose;
extern	dev_t	curfs;
extern	BOOL	forcerestore;
extern	char	*vers;
extern	GINFO	*grip;		/* Global read info pointer	*/

LOCAL	char	*overs;		/* Star version used with last restore */
LOCAL	int	odtype;		/* Old Dump type */
LOCAL	GINFO	*ogp;		/* Global information from last restore	*/
EXPORT	BOOL	restore_valid;	/* TRUE when after we did read a valid header */
				/* Also needed by star_sym.c	*/
EXPORT	BOOL	is_star = TRUE;	/* TRUE with star and we check the version */
				/* We do not abort if this is for star_sym.c */

#define	HASH_ENTS	1024

LOCAL	imap_t	*iroot = NULL;		/* The root directory		  */
LOCAL	imap_t	*itmp = NULL;		/* The temp directory star-tmpdir */
LOCAL	imap_t	*isym = NULL;		/* The temp directory star-symtable */
LOCAL	imap_t	*imaps = NULL;
LOCAL	imap_t	**himaps = NULL;
LOCAL	imap_t	**hoimaps = NULL;
LOCAL	imap_t	**hnimaps = NULL;
#ifdef	DEBUG
LOCAL	int	nimp;
#endif

#ifdef	PADD_NODE_DEBUG
char	*padd_node_caller = NULL;
#endif

LOCAL	char	sym_tmpdir[]	= "star-tmpdir";
LOCAL	char	sym_symtable[]	= "star-symtable";
LOCAL	char	sym_oldsymtable[] = "star-symtable.old";
LOCAL	char	sym_symdump[]	= "star-symdump";
LOCAL	char	sym_lock[]	= "star-lock";

LOCAL	ino_t	lock_ino;

LOCAL	int	hashval		__PR((Uchar *str));
LOCAL	imap_t	*nfind_node	__PR((char *name, imap_t *idir));
LOCAL	imap_t	*pfind_node	__PR((char *name));
EXPORT	imap_t	*padd_node	__PR((char *name, ino_t oino, ino_t nino, Int32_t flags));
LOCAL	imap_t	*oifind_node	__PR((imap_t *idir, ino_t nino));
LOCAL	imap_t	*nifind_node	__PR((imap_t *idir, ino_t nino));
LOCAL	imap_t	*add_node	__PR((imap_t *cwd, char *name, ino_t oino, ino_t nino, Int32_t flags));
EXPORT	imap_t	*sym_addrec	__PR((FINFO *info));
EXPORT	void	sym_addstat	__PR((FINFO *info, imap_t *imp));
EXPORT	imap_t	*sym_dirprerpare __PR((FINFO *info, imap_t *idir));
LOCAL	void	move2tmp	__PR((char *dir, char *name, ino_t oino, ino_t nino));
LOCAL	void	move2dir	__PR((char *dir, char *name, ino_t oino));
EXPORT	imap_t	*sym_typecheck	__PR((FINFO *info, FINFO *cinfo, imap_t *imp));
LOCAL	BOOL	removefile	__PR((char *name));
LOCAL	BOOL	removetree	__PR((char *name));
EXPORT	void	sym_initmaps	__PR((void));
EXPORT	void	sym_open	__PR((char *name));
LOCAL	int	xgetline	__PR((FILE *f, char *buf, int len, char *name));
LOCAL	void	checknl		__PR((FILE *f));
LOCAL	void	sym_initsym	__PR((void));
LOCAL	void	tmpnotempty	__PR((void));
LOCAL	void	purgeent	__PR((imap_t *imp));
LOCAL	void	purgetree	__PR((imap_t *imp));
EXPORT	void	sym_init	__PR((GINFO *gp));
EXPORT	int	ngetline	__PR((FILE *f, char *buf, int len));
EXPORT	void	sym_close	__PR((void));
LOCAL	void	sym_dump	__PR((void));
LOCAL	void	writeheader	__PR((FILE *f));
LOCAL	void	readheader	__PR((FILE *f));
LOCAL	void	checkheader	__PR((void));
LOCAL	void	useforce	__PR((void));
LOCAL	char	*fullname	__PR((imap_t *imp, char *cp, char *ep, BOOL top));
LOCAL	void	printfullname	__PR((FILE *f, imap_t *imp));
LOCAL	void	printonesym	__PR((FILE *f, imap_t *imp));
LOCAL	void	printsyms	__PR((FILE *f, imap_t *imp));
#ifdef	PRINT_L_SYM
LOCAL	void	printoneLsym	__PR((FILE *f, imap_t *imp));
LOCAL	void	printLsyms	__PR((FILE *f, imap_t *imp));
EXPORT	void	printLsym	__PR((FILE *f));
#endif
#ifdef	__needed__
LOCAL	BOOL	dirdiskonly	__PR((FINFO *info, int *odep, char ***odp));
#endif

LOCAL int
hashval(str)
	register Uchar	*str;
{
	register int	sum = 0;
	register int	i;
	register int	c;

	for (i = 0; (c = *str++) != '\0'; i++)
		sum ^= (c << (i&7));
	return (sum);
}

/*
 * Find a node by path name component and directory node pointer
 */
LOCAL imap_t *
nfind_node(name, idir)
	char	*name;
	register imap_t	*idir;
{
	register imap_t	*imp;
	register char	*np;
	register char	*inp;
	register int	nhash;

	np = name;
	if (np[0] == '\0')
		return (idir);
	if (np[0] == '.') {
		if (np[1] == '\0')
			return (idir);
		if (np[1] == '.' && np[2] == '\0')
			return (idir->i_dparent);
	}
	nhash = hashval((Uchar *)name);
	imp = himaps[nhash%HASH_ENTS];

	for (; imp; imp = imp->i_hnext) {

		if (nhash != imp->i_hash)
			continue;
		if (imp->i_dparent != idir)
			continue;
		if (imp->i_flags & I_DELETE)
			continue;

		np = name;
		inp = imp->i_name;
		while (*inp) {
			if (*np++ != *inp++)
				goto nextmatch;
		}
		if (np[0] == '\0')
			break;
nextmatch:
		;
	}
	return (imp);
}

/*
 * Find a node by full path name
 */
LOCAL imap_t *
pfind_node(name)
	char	*name;
{
	register imap_t	*imp = iroot;
	register char	*np = name;
	register char	*sp;

	if (imp == NULL)
		return (imp);

	while (*np) {
		if ((sp = strchr(np, '/')) != NULL)
			*sp = '\0';
		if ((imp->i_flags & I_DIR) == 0) {
			errmsgno(EX_BAD,
				"pfind_node(): Not a directory '%s' for '%s' flags %X i_dir %p.\n",
				imp->i_name, name,
				imp->i_flags, imp->i_dir);
			return (NULL);
		}
		imp = nfind_node(np, imp);
		if (sp)
			*sp = '/';
		else
			break;
		np = &sp[1];
		if (imp == NULL)
			break;
	}
	return (imp);
}

/*
 * Add a new full path name and return a node pointer
 * Also used by star_sym.c
 */
EXPORT imap_t *
padd_node(name, oino, nino, flags)
	char	*name;
	ino_t	oino;
	ino_t	nino;
	Int32_t	flags;
{
	register imap_t	*imp = iroot;
	register imap_t	*idir;
	register char	*np = name;
	register char	*sp;

	if (imp == NULL) {
		if ((np[0] == '.' && np[1] == '\0') ||
		    (np[0] == '.' && np[1] == '/' && np[2] == '\0')) {
			imp = add_node(NULL, ".", oino, nino, flags);
			imp->i_dparent = imp;
			iroot = imp;
			return (imp);
		}
		comerrno(EX_BAD, "Panic no root to add '%s'.\n", name);
	}

	while (*np) {
		if ((sp = strchr(np, '/')) != NULL)
			*sp = '\0';

		if ((imp->i_flags & I_DIR) == 0) {
#ifdef	PADD_NODE_DEBUG
			errmsgno(EX_BAD, "padd_node(): caller '%s'.\n", padd_node_caller);
#endif
			errmsgno(EX_BAD,
				"padd_node(): Not a directory '%s' for '%s' flags %X i_dir %p.\n",
				imp->i_name, name,
				imp->i_flags, imp->i_dir);
			return (NULL);
		}
		idir = imp;
		imp = nfind_node(np, idir);
		if (imp == NULL) {
			if (sp != NULL && strchr(&sp[1], '/') != NULL)
				imp = add_node(idir, np, 0, 0, I_DIR);
			else
				imp = add_node(idir, np, oino, nino, flags);
		}
		if (sp)
			*sp = '/';
		else
			break;
		np = &sp[1];
	}
	if (imp->i_oino == 0 && oino != 0) {
		int	hv;

		imp->i_oino = oino;
		hv = oino % HASH_ENTS;
		imp->i_honext = hoimaps[hv];
		hoimaps[hv] = imp;
	}
	if (imp->i_nino == 0 && nino != 0) {
		int	hv;

		/*
		 * This is (most likely) a temporary node.
		 * We need to set up the flags acording to the right file.
		 */
		if (imp->i_flags != 0) {
			/*
			 * Not a temporarary node found?
			 */
#ifdef	PADD_NODE_DEBUG
			errmsgno(EX_BAD, "padd_node(): caller '%s'.\n", padd_node_caller);
#endif
			errmsgno(EX_BAD,
			"padd_node(): Panic: Flags 0x%X on temp node '%s' for '%s'.\n",
			imp->i_flags, imp->i_name, name);
		}
		imp->i_flags = flags;
		imp->i_nino = nino;
		hv = nino % HASH_ENTS;
		imp->i_hnnext = hnimaps[hv];
		hnimaps[hv] = imp;
	}
	return (imp);
}

/*
 * Find a node by old inode number
 */
LOCAL imap_t *
oifind_node(idir, oino)
	imap_t	*idir;
	ino_t	oino;
{
	register imap_t	*imp;

	imp = hoimaps[oino%HASH_ENTS];

	for (; imp; imp = imp->i_honext) {

		if (imp->i_flags & I_DELETE)
			continue;
		if (idir && idir != imp->i_dparent)
			continue;

		/*
		 * XXX Dies ist eine halbfertige Directory.
		 * XXX Hoffentlich brauchen wir diesen Knoten nie.
		 */
		if (imp->i_nino == 0)
			continue;
		if (oino == imp->i_oino)
			break;
	}
	return (imp);
}

/*
 * Find a node by new inode number
 */
LOCAL imap_t *
nifind_node(idir, nino)
	imap_t	*idir;
	ino_t	nino;
{
	register imap_t	*imp;

	imp = hnimaps[nino%HASH_ENTS];

	for (; imp; imp = imp->i_hnnext) {

		if (imp->i_flags & I_DELETE)
			continue;
		if (idir && idir != imp->i_dparent)
			continue;
		if (nino == imp->i_nino)
			break;
	}
	return (imp);
}

/*
 * Add a new node by path name component and directory node pointer
 * return a node pointer
 */
LOCAL imap_t *
add_node(cwd, name, oino, nino, flags)
	imap_t	*cwd;
	char	*name;
	ino_t	oino;
	ino_t	nino;
	Int32_t	flags;
{
	register imap_t	*imp;
	register int	hv;

	imp = ___malloc(sizeof (imap_t), "new imap");
	imp->i_name = ___savestr(name);
	imp->i_hash = hashval((Uchar *)imp->i_name);
	imp->i_oino = oino;
	imp->i_nino = nino;
	imp->i_flags = flags;
	imp->i_next = imaps;
	imaps = imp;

	imp->i_dir = NULL;
	imp->i_dnext = NULL;
	imp->i_dxnext = NULL;
	imp->i_dparent = cwd;
	if (cwd) {
		imp->i_dxnext = cwd->i_dir;
		cwd->i_dir = imp;
	}

	hv = imp->i_hash % HASH_ENTS;
	imp->i_hnext = himaps[hv];
	himaps[hv] = imp;

	if (oino) {
		hv = oino % HASH_ENTS;
		imp->i_honext = hoimaps[hv];
		hoimaps[hv] = imp;
	} else {
		imp->i_honext = NULL;
	}
	if (nino) {
		hv = nino % HASH_ENTS;
		imp->i_hnnext = hnimaps[hv];
		hnimaps[hv] = imp;
	} else {
		imp->i_hnnext = NULL;
	}
#ifdef	DEBUG
	nimp++;
#endif

	return (imp);
}

/*
 * Add a record for a file. If this is a directory, add entries for the
 * directory content.
 */
EXPORT imap_t *
sym_addrec(info)
	FINFO	*info;
{
	imap_t	*imp;
	BOOL	isold = TRUE;

#ifdef	XXX_DEBUG
	if (grip->dumplevel > 0) {
		error("sym_addrec(%s) Type %s\n",
			info->f_name, XTTONAME(info->f_rxftype));
	}
#endif
	if ((imp = pfind_node(info->f_name)) == NULL) {
		isold = FALSE;
#ifdef	PADD_NODE_DEBUG
		padd_node_caller = "sym_addrec";
#endif
		imp = padd_node(info->f_name, info->f_ino, (ino_t)0, 0);
#ifdef	PADD_NODE_DEBUG
		if (imp == NULL)
			errmsgno(EX_BAD, "padd_node(%s, %lld, 0, 0) = NULL\n",
				info->f_name, (Llong)info->f_ino);
#endif
	} else {
#ifdef	XXX_DEBUG
		if (grip->dumplevel > 0) {
			error("sym_addrec(%s) found dir: %d TYPECHANGE: %d\n",
				info->f_name,
				imp->i_flags & I_DIR,
				is_dir(info) != ((imp->i_flags & I_DIR) != 0));
		}
#endif
	}
	if (imp == NULL) {
		sym_dump();
		comerrno(EX_BAD,
			"Panic: cannot add node '%s'.\n",
			info->f_name);
	}

	if (is_dir(info)) {
		/*
		 * If the node for the name of this dir is an old node and
		 * does not have the DIR flag set, we may not set up the dir
		 * content as we later will move this node away and create a
		 * new one for the new dir that is going to be created.
		 */
		if (isold && (imp->i_flags & I_DIR) == 0)
			return (imp);

		imp->i_flags |= I_DIR;
	}

	if (is_dir(info)) {
		imap_t	*idir;
		char	*dp;
		ino_t	*ip;
		int	len;
		int	i;

		idir = imp;
		dp = info->f_dir;
		ip = info->f_dirinos;
		if (dp == NULL || ip == NULL)
			return (idir);

		i = 0;
		while (*dp) {
			dp++;
			len = strlen(dp);

			if ((imp = nfind_node(dp, idir)) == NULL) {
				imp = add_node(idir, dp, (ino_t)0, (ino_t)0, 0);
			}
			if (imp->i_oino == (ino_t)0) {
				int	hv;

				imp->i_oino = ip[i];
				hv = imp->i_oino % HASH_ENTS;
				imp->i_honext = hoimaps[hv];
				hoimaps[hv] = imp;
			}
			i++;
			dp += len+1;
		}
		return (idir);
	}
	return (imp);
}

/*
 * Add status information for a node.
 */
EXPORT void
sym_addstat(info, imp)
	FINFO	*info;
	imap_t	*imp;
{
	FINFO	finfo;
	BOOL	isold = TRUE;
	int	hv;

	if (imp == NULL && (imp = pfind_node(info->f_name)) == NULL) {
		isold = FALSE;
#ifdef	PADD_NODE_DEBUG
		padd_node_caller = "sym_addstat";
#endif
		imp = padd_node(info->f_name, info->f_ino, (ino_t)0, 0);
#ifdef	PADD_NODE_DEBUG
		if (imp == NULL)
			errmsgno(EX_BAD, "padd_node(%s, %lld, 0, 0) = NULL\n",
				info->f_name, (Llong)info->f_ino);
#endif
	}
	if (imp == NULL) {
		sym_dump();
		comerrno(EX_BAD,
			"Panic: cannot add node '%s'.\n",
			info->f_name);
	}
	if (imp->i_oino == 0) {
		if (imp != iroot) {
			errmsgno(EX_BAD,
			"WARNING: late old inode add for '%s'\n", info->f_name);
		}
		imp->i_oino = info->f_ino;
		hv = imp->i_oino % HASH_ENTS;
		imp->i_honext = hoimaps[hv];
		hoimaps[hv] = imp;
	}

	fillbytes((char *)&finfo, sizeof (finfo), '\0');
	_getinfo(info->f_name, &finfo);
	if (imp->i_nino != 0 && imp->i_nino != finfo.f_ino) {
		errmsgno(EX_BAD, "sym_addstat(): %s nino change from %lld to %lld flags %X\n",
			info->f_name,
			(Llong)imp->i_nino, (Llong)finfo.f_ino,
			imp->i_flags);
	}

	/*
	 * An nino change implies a hash change. We need to remove the node
	 * from the old hash list and put it into the new list.
	 */
	if (imp->i_nino != finfo.f_ino) {
		BOOL	newhash = FALSE;

		if ((imp->i_nino != 0) &&
		    ((imp->i_nino % HASH_ENTS) != (finfo.f_ino % HASH_ENTS))) {
			imap_t	*tnp;

			tnp = hnimaps[imp->i_nino % HASH_ENTS];
			for (; tnp; tnp = tnp->i_hnnext) {
				if (imp == tnp->i_hnnext) {
					tnp->i_hnnext = imp->i_hnnext;
					break;
				}
			}
			newhash = TRUE;
		}

		if (imp->i_nino == 0 || newhash) {
			hv = finfo.f_ino % HASH_ENTS;
			imp->i_hnnext = hnimaps[hv];
			hnimaps[hv] = imp;
		}
		imp->i_nino = finfo.f_ino;
	}

	if (is_dir(info)) {
		int	len = strlen(imp->i_name);

		if (imp->i_name[len-1] == '/')
			imp->i_name[len-1] = '\0';
		imp->i_flags |= I_DIR;
	}
}

/*
 * Prepare the directory for the following extraction.
 * This is done by first removing all files that are no longer present on the
 * current incremental and then trying to move/link missing files that are
 * a result of a rename or link operation.
 */
EXPORT imap_t *
sym_dirprepare(info, idir)
	FINFO	*info;
	imap_t	*idir;
{
	int	dlen;
	ino_t	*oino;
	char	**dname;
	char	*p;
	int	i;
	int	j;
	FINFO	finfo;
	char	*dp2;
	int	ents2;
	char	**dname2;
	ino_t	*ino2;
	ino_t	*oino2;
	char	*slashp = NULL;

	if (info->f_dir == NULL) {
		/*
		 * Most likely a mount point
		 */
		if (info->f_dev == curfs) {
			unlink(sym_lock);
			comerrno(EX_BAD, "Archive contains directory '%s' without name list\n", info->f_name);
		}
		return (idir);
	}

#ifdef	DIRP_DEBUG
	if (!_getinfo(info->f_name, &finfo) && geterrno() != ENOENT)
		errmsg("DEBUG1: stat for '%s' failed.\n",
			info->f_name);
#endif
	/*
	 * Try to work around a moving POSIX target.
	 * POSIX.1-1988 requires that stat("/etc/passwd/", ) works.
	 * While POSIX.1-2001 requires to return ENOTDIR.
	 */
	info->f_namelen = strlen(info->f_name);
	if (info->f_name[info->f_namelen-1] == '/') {
		slashp = &info->f_name[info->f_namelen-1];
		*slashp = '\0';
	}
	/*
	 * Check if the node on disk is really a directory.
	 * As we did already check the directory content of the higher direcory
	 * it should never happen that the on disk node is not a directory.
	 */
	fillbytes((char *)&finfo, sizeof (finfo), '\0');
	seterrno(0);
	(void) _getinfo(info->f_name, &finfo);

	if (!is_dir(&finfo)) {		/* Also catches the ENOENT case */
		/*
		 * There is either no file on disk, we cannot stat() it, or
		 * the file on the disk has the same name and inode number
		 * as the directory in the archive.
		 * XXX Should we rather use move2tmp()?
		 */
		imap_t	*imp;

		imp = idir;
		if (geterrno() != ENOENT) {
			if (unlink(info->f_name) < 0)
				comerr("Cannot unlink '%s'.\n", info->f_name);
		}
#ifdef	MKD_DEBUG
		{ extern char *mkdwhy; mkdwhy = "restore"; }
#endif
		if (!make_adir(info)) {
			if (slashp)
				*slashp = '/';
			return (imp);
		}
		if ((imp->i_flags & I_DIR) == 0) {
			purgeent(imp);
			imp = sym_addrec(info);
		}
		if (!_getinfo(info->f_name, &finfo)) {
			if (slashp)
				*slashp = '/';
			return (imp);
		}
		sym_addstat(&finfo, imp);
		idir = imp;
	}
	if (slashp)
		*slashp = '/';

	if ((dname = ___malloc(info->f_dirents * sizeof (char *), "sym_dirprepare name")) == NULL) {
		return (idir);
	}
	/*
	 * Put directory names from archive into array.
	 */
	p = info->f_dir;
	oino = info->f_dirinos;
	dlen = info->f_dirents;
	for (i = 0; i < dlen; i++) {
		dname[i] = &p[1];
		p += strlen(&p[1]) + 2;
	}

#ifdef	RES_DEBUG
	for (i = 0; i < dlen; i++) {
		error("INO %lld NAME %s\n",
			(Llong)info->f_dirinos[i],
			dname[i]);
	}
#endif

	dp2 = fetchdir(info->f_name, &ents2, 0, &ino2);
	if ((oino2 = ___malloc(ents2 * sizeof (ino_t), "sym_dirprepare oino2")) == NULL) {
		return (idir);
	}
	if ((dname2 = ___malloc(ents2 * sizeof (char *), "sym_dirprepare name2")) == NULL) {
		return (idir);
	}
	/*
	 * Put directory names from disk into array.
	 */
	p = dp2;
	for (i = 0; i < ents2; i++) {
		dname2[i] = &p[1];
		p += strlen(&p[1]) + 2;
	}
	for (i = 0; i < ents2; i++) {
		imap_t	*imp;
		imp = nifind_node((imap_t *)0, ino2[i]);
		if (imp == NULL) {
			if (ino2[i] != lock_ino) {
				errmsgno(EX_BAD,
				"Panic: No symbol entry for inode %lld (%s).\n",
				(Llong)ino2[i], dname2[i]);
			}
			oino2[i] = 0;
			continue;
		}
		if (imp->i_oino == (ino_t)0) {
			if (imp != itmp && imp != isym) {
				errmsgno(EX_BAD,
				"Panic: No old inode for inode %lld (%s).\n",
				(Llong)ino2[i], dname2[i]);
			}
		}
		oino2[i] = imp->i_oino;
	}

	if (xdebug)
		error("sym_dirprepare(%s)\n", info->f_name);
	/*
	 * Check for all files that are in the current on disk directory
	 * but no longer exist on the current incremental with the same
	 * name and inode number as before.
	 */
	for (i = 0; i < ents2; i++) {
		ino_t	in;
		if (oino2[i] == 0)
			continue;
		in = oino2[i];

		if (xdebug)
			error("Checking ino %lld (%s)...", (Llong)oino2[i], dname2[i]);
		for (j = 0; j < dlen; j++) {
			if (xdebug > 1)
				error("in %lld oino %lld\n", (Llong)in, (Llong)oino[j]);
			if (in == oino[j] && streql(dname2[i], dname[j]))
				break;
		}
		if (j >= dlen) {
			if (xdebug)
				error("RM\n");
				/* dir name, file name,  old inode#, new inode # */
			move2tmp(info->f_name, dname2[i], in, ino2[i]);
		} else {
			if (xdebug)
				error("found\n");
		}
	}

	if (xdebug)
		error("sym_dirprepare(%s) <<<<\n", info->f_name);
	/*
	 * Check for all files that are on the current incremental but
	 * are not in the current on disk directory  with the same
	 * and inode number.
	 */
	for (i = 0; i < dlen; i++) {
		ino_t	in;
		in = oino[i];

		if (xdebug)
			error("Checking ino %lld (%s)...", (Llong)oino[i], dname[i]);
		for (j = 0; j < ents2; j++) {
			if (in == oino2[j] && streql(dname2[j], dname[i]))
				break;
		}
		if (j >= ents2) {
			if (xdebug)
				error("LINK\n");
				/* dir name, file name,  old inode# */
			move2dir(info->f_name, dname[i], in);
		} else {
			if (xdebug)
				error("found\n");
		}
	}

	if (dname != NULL)
		free(dname);
	if (oino2 != NULL)
		free(oino2);
	if (dname2 != NULL)
		free(dname2);
	if (dp2 != NULL)
		free(dp2);
	if (ino2 != NULL)
		free(ino2);
	return (idir);
}

/*
 * Move a file to the temp directory. This file has been identified
 * to be no longer in the current directory pointed to by 'dir'.
 */
LOCAL void
move2tmp(dir, name, oino, nino)
	char	*dir;
	char	*name;
	ino_t	oino;
	ino_t	nino;
{
	char	path[2*PATH_MAX+1];
	char	tpath[128];
	imap_t	*onp;
	imap_t	*nnp;

	js_snprintf(path, sizeof (path), "%s/%s", dir, name);
	js_snprintf(tpath, sizeof (tpath), "%s/#%lld", sym_tmpdir, (Llong)nino);

	onp = pfind_node(path);
	if (onp == NULL) {
		sym_dump();
		comerrno(EX_BAD,
			"Panic: amnesia in inode data base for '%s'.\n",
			path);
	}
	nnp = nifind_node(itmp, nino);
	if (nnp) {			/* inode is already in star-tmpdir */
		if (xdebug)
			error("unlink(%s)\n", path);
		if (unlink(path) < 0)
			comerr("Cannot unlink '%s'.\n", path);
		purgeent(onp);
		return;
	}

	if (xdebug)
		error("rename(%s, %s)\n", path, tpath);
	if (rename(path, tpath) < 0)
		comerr("Cannot rename '%s' to '%s'.\n", path, tpath);

#ifdef	PADD_NODE_DEBUG
	padd_node_caller = "move2tmp";
#endif
	nnp = padd_node(tpath, onp->i_oino, onp->i_nino, onp->i_flags);
#ifdef	PADD_NODE_DEBUG
	if (nnp == NULL)
		errmsgno(EX_BAD, "padd_node(%s, %lld, %lld, %X) = NULL\n",
			tpath, (Llong)onp->i_oino,
			(Llong)onp->i_nino, onp->i_flags);
#endif
	if (xdebug)
		error("move2tmp() itmp %p onp %p nnp %p\n", itmp, onp, nnp);

	purgeent(onp);
	nnp->i_dir = onp->i_dir;
	for (onp = nnp->i_dir; onp; onp = onp->i_dxnext) {
		onp->i_dparent = nnp;
	}
}

/*
 * Try to move a file from the temp directory to the current directory
 * pointed to by 'dir'.
 */
LOCAL void
move2dir(dir, name, oino)
	char	*dir;
	char	*name;
	ino_t	oino;
{
	char	path[2*PATH_MAX+1];
	char	tpath[2*PATH_MAX+1];
	imap_t	*onp;
	imap_t	*nnp;
	char	*p;

	js_snprintf(path, sizeof (path), "%s/%s", dir, name);
	onp = oifind_node(itmp, oino);

#ifdef	RES_DEBUG
	if (onp)
		error("move2dir(%s, %s, %lld) = %s\n",
			dir, name, (Llong)oino, onp->i_name);
	else
		error("move2dir(%s, %s, %lld) = NOT FOUND\n",
			dir, name, (Llong)oino);
#endif

	if (onp) {
		js_snprintf(tpath, sizeof (tpath), "%s/%s",
						sym_tmpdir, onp->i_name);
		if (xdebug)
			error("rename(%s, %s)\n", tpath, path);
		if (rename(tpath, path) < 0) {
			errmsg("Cannot rename '%s' to '%s'.\n", tpath, path);
			/* XXX set error code */
			return;
		}
#ifdef	PADD_NODE_DEBUG
		padd_node_caller = "move2dir";
#endif
		nnp = padd_node(path, onp->i_oino, onp->i_nino, onp->i_flags);
#ifdef	PADD_NODE_DEBUG
		if (nnp == NULL)
			errmsgno(EX_BAD, "padd_node(%s, %lld, %lld, %X) = NULL\n",
				path, (Llong)onp->i_oino,
				(Llong)onp->i_nino, onp->i_flags);
#endif
		if (xdebug) {
			error("move2dir() itmp %p onp %p nnp %p\n",
							itmp, onp, nnp);
		}
		purgeent(onp);
		nnp->i_dir = onp->i_dir;
		for (onp = nnp->i_dir; onp; onp = onp->i_dxnext) {
			onp->i_dparent = nnp;
		}
		return;
	}

	/*
	 * XXX Wenn nicht in star-tmpdir, dann
	 *
	 * -	hard link zu File versuchen
	 *
	 * -	rename einer Directory versuchen und Flag setzen
	 *
	 * -	Ist Flag gesetzt, dann Hard Link auf Dir setzen.
	 */
	if (xdebug)
		error("Cannot rename '%s' from '%s'.\n", path, sym_tmpdir);

	onp = oifind_node((imap_t *)0, oino);
	if (onp == NULL) {
		if (xdebug)
			error("Cannot link/move any file to '%s'.\n",
								path);
		return;
	}
	p = fullname(onp, tpath, &tpath[sizeof (tpath)], TRUE);
	if (p == NULL) {
		/* XXX error code */
		errmsgno(EX_BAD, "Path name '");
		printfullname(stderr, onp);
		error("' too long, cannot rename\n");
		return;
	}
	if (xdebug)
		error("move2dir(%s, %s, %lld) found path => '%s'\n",
			dir, name, (Llong)oino, tpath);

	if ((onp->i_flags & (I_DIR|I_DID_RENAME)) == I_DIR) {
		if (xdebug)
			error("rename(%s, %s)\n", tpath, path);
		if (rename(tpath, path) < 0) {
			/* XXX error code */
			errmsg("Cannot rename(%s, %s)\n", tpath, path);
		} else {
#ifdef	PADD_NODE_DEBUG
			padd_node_caller = "move2dir 2";
#endif
			nnp = padd_node(path, onp->i_oino, onp->i_nino, onp->i_flags);
#ifdef	PADD_NODE_DEBUG
			if (nnp == NULL)
				errmsgno(EX_BAD, "padd_node(%s, %lld, %lld, %X) = NULL\n",
					path, (Llong)onp->i_oino,
					(Llong)onp->i_nino, onp->i_flags);
#endif
			purgeent(onp);
			nnp->i_flags |= I_DID_RENAME;
			if ((onp->i_flags & I_DIR) == 0)
				comerrno(EX_BAD, "Panic: Not a dir '%s'.\n", path);
			nnp->i_dir = onp->i_dir;
			for (onp = nnp->i_dir; onp; onp = onp->i_dxnext) {
				onp->i_dparent = nnp;
			}
		}
		return;
	}
	if (xdebug)
		error("link(%s, %s)\n", tpath, path);
#ifdef	HAVE_LINK
	if (link(tpath, path) < 0) {
#else
	if (1) {
#ifdef	ENOSYS
		seterrno(ENOSYS);
#else
		seterrno(EINVAL);
#endif
#endif
		/* XXX error code */
		errmsg("Cannot link(%s, %s)\n", tpath, path);
	} else {
#ifdef	PADD_NODE_DEBUG
		padd_node_caller = "move2dir 3";
#endif
		nnp = padd_node(path, onp->i_oino, onp->i_nino, onp->i_flags);
#ifdef	PADD_NODE_DEBUG
		if (nnp == NULL)
			errmsgno(EX_BAD, "padd_node(%s, %lld, %lld, %X) = NULL\n",
				path, (Llong)onp->i_oino,
				(Llong)onp->i_nino, onp->i_flags);
#endif
#ifdef	nonono
		/*
		 * XXX It is not clear how to handle new hard links
		 * XXX to directories correctly.
		 */
		nnp->i_dir = onp->i_dir;
		for (onp = nnp->i_dir; onp; onp = onp->i_dxnext) {
			onp->i_dparent = nnp;
		}
#endif
	}
}

/*
 * Check whether the target file exists and has a
 * different type or is a dev node with different
 * major/minor numbers. In this case, we need to
 * remove the file. This happend when the original
 * file has been removed and a new (different) file
 * with the same name did get the same inode number.
 */
EXPORT imap_t *
sym_typecheck(info, cinfo, imp)
	FINFO	*info;
	FINFO	*cinfo;
	imap_t	*imp;
{
	char	*slashp = NULL;
extern	BOOL	uncond;
extern	BOOL	nowarn;
	BOOL	ouncond = uncond;
	BOOL	onowarn = nowarn;

#ifdef	RES2_DEBUG
	error("sym_typecheck(%s) NEW type: %s\n",
			info->f_name,
			XTTONAME(info->f_rxftype));
#endif
#ifdef	RES2_DEBUG
	seterrno(0);
#endif
	/*
	 * Try to work around a moving POSIX target.
	 * POSIX.1-1988 requires that stat("/etc/passwd/", ) works.
	 * While POSIX.1-2001 requires to return ENOTDIR.
	 */
	info->f_namelen = strlen(info->f_name);
	if (info->f_name[info->f_namelen-1] == '/') {
		slashp = &info->f_name[info->f_namelen-1];
		*slashp = '\0';
	}
	fillbytes((char *)cinfo, sizeof (*cinfo), '\0');
	if (!_getinfo(info->f_name, cinfo)) {
		cinfo->f_rxftype = XT_NONE;
#ifdef	RES2_DEBUG
		errmsg("sym_typecheck(%s) ---> OLD FILE NOT found\n",
			info->f_name);
#endif
		if (slashp)
			*slashp = '/';
		return (imp);
	}

#ifdef	RES2_DEBUG
	error("sym_typecheck(%s) OLD FILE found, type: %s\n",
			info->f_name,
			XTTONAME(cinfo->f_rxftype));
#endif

	if (info->f_filetype == cinfo->f_filetype &&
	    !is_special(cinfo) && !is_symlink(cinfo)) {
		if (slashp)
			*slashp = '/';
		return (imp);
	}

	uncond = FALSE;
	nowarn = TRUE;
	if (is_symlink(cinfo) && same_symlink(cinfo))
		cinfo->f_flags |= F_SAME;
	if (is_special(cinfo) && same_special(cinfo))
		cinfo->f_flags |= F_SAME;
	uncond = ouncond;
	onowarn = nowarn;

	if ((is_symlink(cinfo) || is_special(cinfo)) &&
	    (cinfo->f_flags & F_SAME) == 0) {
#ifdef	RES2_DEBUG
		error("sym_typecheck(%s) REMOVE OLD\n",
			info->f_name);
#endif
		removefile(info->f_name);
		purgeent(imp);
		if (slashp)
			*slashp = '/';
		return (NULL);
	}

	if (info->f_type == cinfo->f_type) {
		if (slashp)
			*slashp = '/';
		return (imp);
	}

	if (imp == NULL)
		return (imp);

	if (is_dir(cinfo)) {
		imap_t	*cmp;

#ifdef	RES2_DEBUG
		error("sym_typecheck(%s) MOVE DIRCONT\n",
			info->f_name);
#endif
		for (cmp = imp->i_dir; cmp; cmp = cmp->i_dxnext) {
			if (cmp->i_flags & I_DELETE)
				continue;

			move2tmp(info->f_name, cmp->i_name, cmp->i_oino, cmp->i_nino);
		}
	}

#ifdef	RES2_DEBUG
	error("sym_typecheck(%s) REMOVE OLD\n",
			info->f_name);
#endif
	removefile(info->f_name);
	purgeent(imp);
	if (slashp)
		*slashp = '/';
	return (NULL);
}

LOCAL BOOL
removefile(name)
	char	*name;
{
	extern	BOOL	force_remove;
		BOOL	of = force_remove;
		BOOL	ret;

	if (xdebug)
		error("REMOVE %s\n", name);

	force_remove = TRUE;
	ret = remove_file(name, TRUE);
	force_remove = of;
	return (ret);
}

LOCAL BOOL
removetree(name)
	char	*name;
{
	extern	BOOL	force_remove;
	extern	BOOL	remove_recursive;
		BOOL	of = force_remove;
		BOOL	or = remove_recursive;
		BOOL	ret;

	if (xdebug)
		error("REMOVE tree %s\n", name);

	force_remove	 = TRUE;
	remove_recursive = TRUE;
	ret = remove_file(name, TRUE);
	force_remove	 = of;
	remove_recursive = or;
	return (ret);
}

/*
 * Needed separately by star_sym
 */
EXPORT void
sym_initmaps()
{
	if (himaps == NULL) {
		register int	hv;

		himaps  = ___malloc(HASH_ENTS * sizeof (imap_t *), "imap hash");
		hoimaps = ___malloc(HASH_ENTS * sizeof (imap_t *), "oimap hash");
		hnimaps = ___malloc(HASH_ENTS * sizeof (imap_t *), "nimap hash");
		for (hv = 0; hv < HASH_ENTS; hv++) {
			himaps[hv] = 0;
			hoimaps[hv] = 0;
			hnimaps[hv] = 0;
		}
	}
}

/*
 * Read in the old inode symbol table
 */
EXPORT void
sym_open(name)
	char	*name;
{
	char	buf[2*PATH_MAX+1];
	FILE	*f;
	int	amt;
	char	*p;
	Llong	ll;
	ino_t	oino;
	ino_t	nino;
	mode_t	old_umask;
static	char	td[] = "star-tmpdir/.";
	imap_t	*ilast = NULL;
	imap_t	*icwd = NULL;
	FINFO	finfo;

#define	PERM_BITS	(S_IRWXU|S_IRWXG|S_IRWXO)	/* u/g/o basic perm */

	if (name)
		f = fileopen(name, "r");
	else
		f = fileopen(sym_symtable, "r");

	fillbytes((char *)&finfo, sizeof (finfo), '\0');
	if (_getinfo(sym_symtable, &finfo) && !is_file(&finfo)) {
		errmsgno(EX_BAD, "'%s' is not a file.\n", sym_symtable);
		comerrno(EX_BAD, "Remove '%s' and try again.\n", sym_symtable);
	}
	sym_initmaps();

	old_umask = umask((mode_t)0);
	umask(PERM_BITS & ~S_IRWXU);
	if (!create_dirs(td)) {
		/*
		 * This also fails if star-tmpdir is not a directory.
		 */
		comerrno(EX_BAD, "Cannot create '%s'.\n", sym_tmpdir);
	}
	umask(old_umask);

	if (f == NULL)
		return;

	(void) xgetline(f, buf, sizeof (buf), sym_symtable);
	if (!streql(buf, vers)) {
		errmsgno(EX_BAD, "Restore version missmatch '%s' '%s'.\n",
			buf, vers);
		if (is_star) {
			comerrno(EX_BAD,
			"All restores need to be done with the same star version.\n");
		}
	}
	overs = ___savestr(buf);		/* Restore SCHILY.release */

	readheader(f);

	while ((amt = ngetline(f, buf, sizeof (buf))) > 0) {
		Int32_t	flags;

		checknl(f);

		if (buf[0] == '>' && buf[1] == '\0') {
			if (iroot == NULL) {
				iroot = icwd = ilast;
				icwd->i_dparent = icwd;
				if (icwd->i_name[0] != '.' || icwd->i_name[1] != '\0')
					comerrno(EX_BAD, "Bad root dir '%s' in '%s'.\n",
						icwd->i_name,
						sym_symtable);
			} else {
				icwd->i_dnext = ilast;
				if (ilast->i_dparent != icwd)
					comerrno(EX_BAD, "Bad parent dir.\n");
				ilast->i_dparent = icwd;
				icwd = ilast;
			}
			continue;
		}
		if (buf[0] == '<' && buf[1] == '\0') {
			icwd = icwd->i_dparent;
			icwd->i_dnext = NULL;
			continue;
		}

		p = buf;
		flags = 0;
		if (*p == 'D')
			flags |= I_DIR;

		p++;	/* Skip Flag */
		p = astollb(p, &ll, 10);
		if (*p != '\t') {
			comerrno(EX_BAD,
				"Missing TAB after old ino in '%s'.\n",
				sym_symtable);
		}
		oino = ll;
		p = astollb(p, &ll, 10);
		if (*p != '\t') {
			comerrno(EX_BAD,
				"Missing TAB after NEW ino '%s'.\n",
				sym_symtable);
		}
		nino = ll;
		ilast = add_node(icwd, ++p, oino, nino, flags);
#ifdef	__needed__
		if (0) {
		imap_t	*ip = iroot;

			for (; ip; ip = ip->i_dnext) {
				printf("%s/", ip->i_name);
				if (ip == icwd)
					break;
			}
			printf("%s\n", p);
		}
#endif
	}
	fclose(f);

#ifdef	OLDSYM_DEBUG
	printsyms(stderr, iroot);
	printLsyms(stderr, iroot);
#endif
}

LOCAL int
xgetline(f, buf, len, name)
	FILE	*f;
	char	*buf;
	int	len;
	char	*name;
{
	int	amt;

	seterrno(0);
	if ((amt = ngetline(f, buf, len)) < 0) {
		if (geterrno() == 0 && amt == EOF) {
			/*
			 * If errno is 0, this must be EOF from getc()
			 */
			amt = 0;
		} else {
			comerr("Cannot read '%s'.\n", name);
		}
	}
	if (amt == 0)
		comerrno(EX_BAD, "File '%s' too short.\n", name);
	if (amt >= len)
		comerrno(EX_BAD, "Line too long in '%s'.\n", name);
	checknl(f);
	return (amt);
}

LOCAL void
checknl(f)
	FILE	*f;
{
	if (getc(f) != '\n') {
		comerrno(EX_BAD,
			"Missing newline at end of record in '%s'.\n",
			sym_symtable);
	}
}

/*
 * Make sure "star-tmpdir" exists and is part of the inode symbol cache.
 * Make sure that "star-symtable" is part of the inode symbol cache if
 * it exists.
 */
LOCAL void
sym_initsym()
{
	char	tpath[PATH_MAX+1];
	FINFO	finfo;
	imap_t	*imp;
	char	*dp;
	int	ents;

	fillbytes((char *)&finfo, sizeof (finfo), '\0');
	if (!_getinfo(sym_tmpdir, &finfo)) {
		unlink(sym_lock);
		comerr("Cannot stat '%s'\n", sym_tmpdir);
	}

	if (iroot == NULL) {
#ifdef	PADD_NODE_DEBUG
		padd_node_caller = "sym_initsym";
#endif
		imp = padd_node(".", (ino_t)0, (ino_t)0, I_DIR);
	}
	if ((imp = pfind_node(sym_tmpdir)) == NULL) {
#ifdef	PADD_NODE_DEBUG
		padd_node_caller = "sym_initsym 2";
#endif
		imp = padd_node(sym_tmpdir, (ino_t)0, finfo.f_ino, I_DIR);
	}
	if (imp->i_nino != finfo.f_ino)
		errmsgno(EX_BAD, "sym_initsym(): %s nino change from %lld to %lld flags %X\n",
			sym_tmpdir,
			(Llong)imp->i_nino, (Llong)finfo.f_ino,
			imp->i_flags);

	imp->i_nino = finfo.f_ino;
	itmp = imp;

	for (imp = imp->i_dir; imp; imp = imp->i_dxnext) {
		if (imp->i_flags & I_DELETE)
			continue;

		js_snprintf(tpath, sizeof (tpath), "%s/%s",
						sym_tmpdir, imp->i_name);
		if (!_getinfo(tpath, &finfo))
			purgetree(imp);
		else
			tmpnotempty();
	}
	ents = 0;
	dp = fetchdir(sym_tmpdir, &ents, 0, (ino_t **)0);
	if (dp)
		free(dp);
	if (ents > 0)
		tmpnotempty();

	fillbytes((char *)&finfo, sizeof (finfo), '\0');
	if (_getinfo(sym_symtable, &finfo)) {
		if ((imp = pfind_node(sym_symtable)) == NULL) {
#ifdef	PADD_NODE_DEBUG
			padd_node_caller = "sym_initsym 3";
#endif
			imp = padd_node(sym_symtable, (ino_t)0, finfo.f_ino, 0);
		}
		isym = imp;
	}
}

LOCAL void
tmpnotempty()
{
	errmsgno(EX_BAD, "The directory '%s' is not empty.\n", sym_tmpdir);
	comerrno(EX_BAD, "Remove all files in '%s' and try again.\n", sym_tmpdir);
}

LOCAL void
purgeent(imp)
	imap_t	*imp;
{
	if (imp == NULL)
		return;
	imp->i_flags |= I_DELETE;
	free(imp->i_name);
	imp->i_name = NULL;
}

LOCAL void
purgetree(imp)
	imap_t	*imp;
{
	if (imp == NULL)
		return;

	for (; imp; imp = imp->i_dxnext) {

		if (imp->i_flags & I_DELETE)
			continue;

		purgetree(imp->i_dir);
		purgeent(imp);
	}
}

/*
 * Do some checks and initialisations before the symbols are going to be used
 */
EXPORT void
sym_init(gp)
	GINFO	*gp;
{
	FILE	*f;
	FINFO	finfo;
#ifdef	VERS_DEBUG
	extern	char	*vers;

	error("Star version '%s'\n", vers);
	error("imaps: %p level %d\n", imaps, gp->dumplevel);
#endif

	f = fileopen(sym_lock, "wce");
	if (f == NULL) {
		comerr("Cannot create '%s', restore is already running.\n",
			sym_lock);
	}
	fclose(f);
	fillbytes((char *)&finfo, sizeof (finfo), '\0');
	_getinfo(sym_lock, &finfo);
	lock_ino = finfo.f_ino;

	if (ogp == NULL)			/* If star-symtable not read */
		odtype = gp->dumptype;		/* Use this dump type */

	/*
	 * XXX imaps -> iroot ???
	 */
	if (gp->dumplevel == 0 && imaps != NULL) {
		errmsgno(EX_BAD, "Trying to extract a level 0 dump but '%s' exists.\n", sym_symtable);
		unlink(sym_lock);
		comerrno(EX_BAD, "Remove '%s' and try again.\n", sym_symtable);
	}
	if (gp->dumplevel > 0 && imaps == NULL) {
		errmsgno(EX_BAD, "Trying to extract a level %d dump but '%s' does not exist.\n",
				gp->dumplevel, sym_symtable);
		unlink(sym_lock);
		comerrno(EX_BAD, "Restore the level 0 dump first and try again.\n");
	}
	if (gp->dumptype != DT_FULL || odtype != DT_FULL) {
		if (gp->dumptype != DT_FULL) {
			errmsgno(EX_BAD,
			"WARNING: This dump is a '%s' dump and not a 'full' dump.\n",
			dt_name(gp->dumptype));
		} else {
			errmsgno(EX_BAD,
			"WARNING: Restore status is '%s' and not from a 'full' dump.\n",
			dt_name(odtype));
		}
		errmsgno(EX_BAD, "WARNING: An incremental restore may not work correctly.\n");
		if (!forcerestore) {
			useforce();
			/* NOTREACHED */
		}
		if (gp->dumptype < odtype)
			odtype = gp->dumptype;
	}

	checkheader();	/* Check Dump level & Dump date */
	sym_initsym();
}

/*
 * A special version of fgetline() that does not stop on '\n' but only on '\0'.
 */
EXPORT int
ngetline(f, buf, len)
	register	FILE	*f;
			char	*buf;
	register	int	len;
{
	register int	c	= '\0';
	register char	*bp	= buf;
	register int	nul	= '\0';

	for (;;) {
		if ((c = getc(f)) < 0)
			break;
		if (c == nul)
			break;
		if (--len > 0) {
			*bp++ = (char)c;
		} else {
			/*
			 * Read up to end of line
			 */
			while ((c = getc(f)) >= 0 && c != nul)
				/* LINTED */
				;
			break;
		}
	}
	*bp = '\0';
	/*
	 * If buffer is empty and we hit EOF, return EOF
	 */
	if (c < 0 && bp == buf)
		return (c);

	return (bp - buf);
}

/*
 * Write back the inode symbol table
 */
EXPORT void
sym_close()
{
	char	tpath[PATH_MAX+1];
	FINFO	finfo;
	FILE	*f;
	imap_t	*imp;
	BOOL	warned = FALSE;
	BOOL	tmpremove = TRUE;
	BOOL	ok = TRUE;
	int	err;
#ifdef	HAVE_FSYNC
	int	cnt;
#endif

	if (!restore_valid) {
		errmsgno(EX_BAD,
			"Invalid or empty dump, will not overwrite '%s'\n",
								sym_symtable);
		return;
	}
	if (_getinfo(sym_symtable, &finfo) &&
	    rename(sym_symtable, sym_oldsymtable) < 0) {
		errmsg("Cannot rename %s to %s.\n",
				sym_symtable, sym_oldsymtable);
	}
	f = filemopen(sym_symtable, "wct", S_IRUSR|S_IWUSR);
	if (f == NULL) {
		errmsg("Cannot create '%s'.\n", sym_symtable);
		return;
	}
	fillbytes((char *)&finfo, sizeof (finfo), '\0');
	if (!_getinfo(sym_symtable, &finfo))
		comerr("Cannot stat '%s'\n", sym_symtable);
	if ((imp = pfind_node(sym_symtable)) == NULL) {

#ifdef	PADD_NODE_DEBUG
		padd_node_caller = "sym_close";
#endif
		imp = padd_node(sym_symtable, (ino_t)0, finfo.f_ino, 0);
	}
	imp->i_nino = finfo.f_ino;

	if (isym == NULL)
		isym = imp;

	if (getenv("STAR_DEBUG"))
		tmpremove = FALSE;
	else if (itmp != NULL)
		error("Removing all in '%s'.\n", sym_tmpdir);
	if (itmp != NULL)
	for (imp = itmp->i_dir; imp; imp = imp->i_dxnext) {
		if (imp->i_flags & I_DELETE)
			continue;

		js_snprintf(tpath, sizeof (tpath), "%s/%s",
						sym_tmpdir, imp->i_name);
		fillbytes((char *)&finfo, sizeof (finfo), '\0');
		if (_getinfo(tpath, &finfo)) {
			if (is_dir(&finfo)) {
				if (rmdir(tpath) >= 0) {
					purgeent(imp);
				} else if (tmpremove) {
					/*
					 * This is a shortcut. We do not walk
					 * through the imap tree but rather
					 * remove anything we find via readdir.
					 */
					if (removetree(tpath))
						purgeent(imp);
				}
			} else {
				if (tmpremove || finfo.f_nlink > 1) {
					if (unlink(tpath) >= 0)
						purgeent(imp);
				}
			}
			if ((imp->i_flags & I_DELETE) == 0 && !warned) {
				if (tmpremove) {
					errmsgno(EX_BAD,
					"A problem occured, not all files in '%s' could be removed.\n",
					sym_tmpdir);
				}
				errmsgno(EX_BAD,
				"Don't forget to remove the files in '%s'.\n",
				sym_tmpdir);
				warned = TRUE;
			}
		}
	}

	writeheader(f);
	printsyms(f, iroot);

	if (fflush(f) != 0)
		ok = FALSE;
#ifdef	HAVE_FSYNC
	err = 0;
	cnt = 0;
	do {
		if (fsync(fdown(f)) != 0)
			err = geterrno();

		if (err == EINVAL)
			err = 0;
	} while (err == EINTR && ++cnt < 10);
	if (err != 0)
		ok = FALSE;
#endif
	if (fclose(f) != 0)
		ok = FALSE;
	if (ok) {
		unlink(sym_oldsymtable);
	} else {
		xstats.s_restore++;
		if (rename(sym_oldsymtable, sym_symtable) < 0)
			errmsg("Cannot rename %s to %s.\n",
					sym_oldsymtable, sym_symtable);
	}

	unlink(sym_lock);
}

LOCAL void
sym_dump()
{
	FILE	*f;

	f = filemopen(sym_symdump, "wct", S_IRUSR|S_IWUSR);
	if (f == NULL) {
		errmsg("Panic: cannot open '%s'\n", sym_symdump);
		return;
	}
	writeheader(f);
	printsyms(f, iroot);
	fclose(f);
}

/*
 * star 1.5 (i386-pc-solaris2.9)	last star
 * partial				last Dump
 * star 1.5 (i386-pc-solaris2.9)	last Dump star
 * exustar				last Dump archive type
 * hugo					last Dump host name
 * /tmp					last Dump filesys
 * partial				last Dump dumptype
 * 1					last Dump dumplevel
 * 0					last Dump reflevel
 * 1097092599.544044			last Dump dumpdate
 * 1096935113.887915			last Dump refdate
 */
LOCAL void
writeheader(f)
	FILE	*f;
{
	fprintf(f, "%s%c\n", vers, 0);
	fprintf(f, "%s%c\n", dt_name(odtype), 0);
	fprintf(f, "%s%c\n", grip->release, 0);
	fprintf(f, "%s%c\n", hdr_name(grip->archtype), 0);
	fprintf(f, "%s%c\n", grip->hostname, 0);
	fprintf(f, "%s%c\n", grip->filesys, 0);
	fprintf(f, "%s%c\n", dt_name(grip->dumptype), 0);
	fprintf(f, "%d%c\n", grip->dumplevel, 0);
	fprintf(f, "%d%c\n", grip->reflevel, 0);
	fprintf(f, "%10lld.%6.6lld%c\n",
		(Llong)grip->dumpdate.tv_sec,
		(Llong)grip->dumpdate.tv_usec,
		0);
	fprintf(f, "%10lld.%6.6lld%c\n",
		(Llong)grip->refdate.tv_sec,
		(Llong)grip->refdate.tv_usec,
		0);
}

LOCAL void
readheader(f)
	FILE	*f;
{
	char	buf[2*PATH_MAX+1];
	char	*p;
	Llong	ll;

	(void) xgetline(f, buf, sizeof (buf), sym_symtable);
	odtype = dt_type(buf);			/* Old Dump type */

	ogp = ___malloc(sizeof (*ogp), "ogp");
	fillbytes(ogp, sizeof (*ogp), '\0');
	(void) xgetline(f, buf, sizeof (buf), sym_symtable);
	ogp->release = ___savestr(buf);		/* Last dump SCHILY.release */
	(void) xgetline(f, buf, sizeof (buf), sym_symtable);
	ogp->archtype = hdr_type(buf);		/* Last dump SCHILY.archtype */
	(void) xgetline(f, buf, sizeof (buf), sym_symtable);
	ogp->hostname = ___savestr(buf);		/* " SCHILY.volhdr.hostname */
	(void) xgetline(f, buf, sizeof (buf), sym_symtable);
	ogp->filesys = ___savestr(buf);		/* " SCHILY.volhdr.filesys */
	(void) xgetline(f, buf, sizeof (buf), sym_symtable);
	ogp->dumptype = dt_type(buf);		/* " SCHILY.volhdr.dumptype */
	(void) xgetline(f, buf, sizeof (buf), sym_symtable);
	p = astollb(buf, &ll, 10);
	if (*p != '\0')
		comerrno(EX_BAD, "Bad dumplevel '%s' in '%s'.\n",
			buf, sym_symtable);
	ogp->dumplevel = ll;			/* " SCHILY.volhdr.dumplevel */
	(void) xgetline(f, buf, sizeof (buf), sym_symtable);
	p = astollb(buf, &ll, 10);
	if (*p != '\0')
		comerrno(EX_BAD, "Bad reflevel '%s' in '%s'.\n",
			buf, sym_symtable);
	ogp->reflevel = ll;			/* " SCHILY.volhdr.reflevel */
	(void) xgetline(f, buf, sizeof (buf), sym_symtable);
	if (!getdumptime(buf, &ogp->dumpdate))	/* " SCHILY.volhdr.dumpdate */
		exit(EX_BAD);
	(void) xgetline(f, buf, sizeof (buf), sym_symtable);
	if (!getdumptime(buf, &ogp->refdate))	/* " SCHILY.volhdr.refdate */
		exit(EX_BAD);
	ogp->gflags = GF_RELEASE | GF_ARCHTYPE | GF_HOSTNAME | GF_FILESYS |
			GF_DUMPTYPE | GF_DUMPLEVEL | GF_REFLEVEL |
			GF_DUMPDATE | GF_REFDATE;

}

LOCAL void
checkheader()
{
#ifdef	DEBUG
	extern	FILE	*vpr;

	fprintf(vpr, "Last restored dump:\n");
	verbose++;
	if (ogp)
		griprint(ogp);
	fprintf(vpr, "This dump:\n");
	griprint(grip);
	verbose--;
#endif

	error("Validating this dump against restored filesystem...\n");
	if (ogp == NULL) {			/* star-symtable not read */
		if (grip->dumplevel == 0) {
			restore_valid = TRUE;
			error("Dump level 0 on empty filesystem, starting restore.\n");
			return;
		}
	}
	if (!streql(grip->hostname, ogp->hostname))
		comerrno(EX_BAD, "Wrong dumphost '%s', last restored dump is from '%s'.\n",
			grip->hostname, ogp->hostname);
	if (!streql(grip->filesys, ogp->filesys))
		comerrno(EX_BAD, "Wrong filesys '%s', last restored dump is from '%s'.\n",
			grip->filesys, ogp->filesys);
	if ((grip->dumplevel == ogp->dumplevel) &&
	    (grip->reflevel == ogp->reflevel) &&
	    (grip->refdate.tv_sec == ogp->refdate.tv_sec) &&
	    (grip->dumpdate.tv_sec == ogp->dumpdate.tv_sec)) {
		comerrno(EX_BAD, "This dump has already been restored.\n");
	}
	if ((grip->dumplevel == ogp->dumplevel) &&
	    (grip->reflevel == ogp->reflevel) &&
	    (grip->refdate.tv_sec == ogp->refdate.tv_sec) &&
	    (grip->dumpdate.tv_sec > ogp->dumpdate.tv_sec)) {
		errmsgno(EX_BAD, "A level %d dump has already been restored.\n",
			grip->dumplevel);
		errmsgno(EX_BAD, "Is is sufficient to restore the most recent dump of each level.\n");
	} else {
		if (grip->reflevel != ogp->dumplevel) {
			errmsgno(EX_BAD,
				"Wrong reflevel %d, last restored dump level was %d.\n",
				grip->reflevel, ogp->dumplevel);
			if (!forcerestore) {
				errmsgno(EX_BAD,
				"Restore a level %d (or higher) dump with reflevel %d.\n",
				ogp->dumplevel+1, ogp->dumplevel);
				useforce();
				/* NOTREACHED */
			}
		}
		if (grip->refdate.tv_sec != ogp->dumpdate.tv_sec) {
			if (ogp->dumpdate.tv_sec > grip->refdate.tv_sec) {
				errmsgno(EX_BAD, "WARNING: refdate %s is older than last restored dump ",
					dumpdate(&grip->refdate));
				error("%s.\n", dumpdate(&ogp->dumpdate));
			} else {
				errmsgno(EX_BAD, "Wrong refdate %s, last restored dump was from ",
					dumpdate(&grip->refdate));
				error("%s.\n", dumpdate(&ogp->dumpdate));
				if (forcerestore)
					goto force;
				useforce();
				/* NOTREACHED */
			}
		}
	}
	restore_valid = TRUE;
	error("Dump is valid, starting restore.\n");
	return;
force:
	restore_valid = TRUE;
	error("Dump is not valid, starting restore because of -force-restore.\n");
}

LOCAL void
useforce()
{
	unlink(sym_lock);
	comerrno(EX_BAD, "Use -force-restore if you want to restore anyway.\n");
	/* NOTREACHED */
}

/*
 * Create a full path name for imp in cp
 */
LOCAL char *
fullname(imp, cp, ep, top)
	imap_t 	*imp;
	char	*cp;
	char	*ep;
	BOOL	top;
{
	int	len;

	if (imp == iroot)
		return (cp);

	if (imp == NULL) {
		errmsgno(EX_BAD, "Panic: fullname(NULL)\n");
		return (NULL);
	}
	cp = fullname(imp->i_dparent, cp, ep, FALSE);
	if (cp == NULL)
		return (NULL);

	if (imp->i_name == NULL) {
		errmsgno(EX_BAD, "Panic: fullname NULL i_name\n");
		return (NULL);
	}
	len = strlen(imp->i_name) + (top ? 0:1);
	if (cp + len >= ep)
		return (NULL);

	strcpy(cp, imp->i_name);
	if (!top) {
		cp += len-1;
		*cp++ = '/';
		*cp   = '\0';
	} else {
		cp += len;
	}
	return (cp);
}

/*
 * Print a full path name by parsing the tree of parent directories
 */
LOCAL void
printfullname(f, imp)
	FILE	*f;
	imap_t 	*imp;
{
	if (imp == iroot)
		return;
	printfullname(f, imp->i_dparent);
	fprintf(f, "/%s", imp->i_name);
}

/*
 * Print a single symbol to a file
 */
LOCAL void
printonesym(f, imp)
	FILE	*f;
	imap_t 	*imp;
{
	/*	Flags	Old ino	New Ino		Name */
	fprintf(f, "%c %lld	%lld	%s%c\n",
			imp->i_flags & I_DIR ? 'D':' ',
			(Llong)imp->i_oino, (Llong)imp->i_nino,
			imp->i_name,
			0);

	if (imp->i_oino == 0) {
		if (imp != itmp && imp != isym) {
			errmsgno(EX_BAD, "WARNING: No old inode number for ");
			printfullname(stderr, imp);
			fprintf(stderr, "\n");
		}
	}
	if (imp->i_nino == 0) {
		errmsgno(EX_BAD, "WARNING: No new inode number for ");
		printfullname(stderr, imp);
		fprintf(stderr, "\n");
	}
}

/*
 * Print all symbols to a file
 */
LOCAL void
printsyms(f, imp)
	FILE	*f;
	imap_t 	*imp;
{
#ifdef	DEBUG
	error("nimp %d\n", nimp);
#endif
	for (; imp; imp = imp->i_dxnext) {

		if (imp->i_flags & I_DELETE)
			continue;

		printonesym(f, imp);

		if (imp->i_flags & I_DIR && imp->i_dir) {
			fprintf(f, ">%c\n", 0);
			printsyms(f, imp->i_dir);
			fprintf(f, "<%c\n", 0);
		}
	}
}


#ifdef	PRINT_L_SYM
/*
 * Print a single symbol with long filenames to a file
 */
LOCAL void
printoneLsym(f, imp)
	FILE	*f;
	imap_t 	*imp;
{
	/*	Flags	Old ino	New Ino		Name */
	fprintf(f, "%c %lld	%lld	",
			imp->i_flags & I_DIR ? 'D':' ',
			(Llong)imp->i_oino, (Llong)imp->i_nino);
	printfullname(f, imp);
	fprintf(f, "\n");

	if (imp->i_oino == 0) {
		if (imp != itmp && imp != isym) {
			errmsgno(EX_BAD, "WARNING: No old inode number for ");
			printfullname(stderr, imp);
			fprintf(stderr, "\n");
		}
	}
}

/*
 * Print all symbols with long filenames to a file
 */
LOCAL void
printLsyms(f, imp)
	FILE	*f;
	imap_t 	*imp;
{
#ifdef	DEBUG
	error("nimp %d\n", nimp);
#endif
	for (; imp; imp = imp->i_dxnext) {

		if (imp->i_flags & I_DELETE)
			continue;

		printoneLsym(f, imp);

		if (imp->i_flags & I_DIR && imp->i_dir)
			printLsyms(f, imp->i_dir);
	}
}

EXPORT void
printLsym(f)
	FILE	*f;
{
	printLsyms(f, iroot);
}
#endif	/* PRINT_L_SYM */

#ifdef	__needed__
/* EXPORT BOOL */
LOCAL BOOL
dirdiskonly(info, odep, odp)
	FINFO	*info;
	int	*odep;
	char	***odp;
{
	register char	**ep1;	   /* Directory entry pointer array (arch) */
	register char	**ep2 = 0; /* Directory entry pointer array (disk) */
	register char	*dp2;	   /* Directory names string from disk	   */
	register char	**oa = 0;  /* Only in arch pointer array	   */
	register char	**od = 0;  /* Only on disk pointer array	   */
	register int	i;
		int	ents1 = -1;
		int	ents2;
		int	dlen = 0;  /* # of entries only on disk		*/
		int	alen = 0;  /* # of entries only in arch		*/
		BOOL	diffs = FALSE;

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
	dp2 = fetchdir(info->f_name, &ents2, 0, (ino_t **)0);
	if (dp2 == NULL) {
		diffs = TRUE;
		errmsg("Cannot read dir '%s'.\n", info->f_name);
		goto no_dircmp;
	}
	ep2 = sortdir(dp2, &ents2);		/* from disk */

	if (ents1 != ents2) {
		if (debug || verbose > 2) {
			error("Archive ents: %d Disk ents: %d '%s'\n",
					ents1, ents2, info->f_name);
		}
		diffs = TRUE;
	}

	if (cmpdir(ents1, ents2, ep1, ep2, NULL, NULL, &alen, &dlen) > 0)
		diffs = TRUE;

	oa = ___malloc(alen * sizeof (char *), "dir diff array");
	od = ___malloc(dlen * sizeof (char *), "dir diff array");
	cmpdir(ents1, ents2, ep1, ep2, oa, od, &alen, &dlen);

#ifdef	DEBUG
	for (i = 0; i < dlen; i++) {
		error("Only on disk '%s': '%s'\n",
				info->f_name, od[i] + 1);
	}
	for (i = 0; i < alen; i++) {
		error("Only in archive '%s': '%s'\n",
				info->f_name, oa[i] + 1);
	}
#endif

no_dircmp:
	if (odep)
		*odep = dlen;

	if (dp2)
		free(dp2);
	if (ep1)
		free(ep1);
	if (ep2)
		free(ep2);
	if (odp)
		*odp = od;
	else if (od)
		free(od);
	if (oa)
		free(oa);

	return (diffs);
}
#endif	/* __needed__ */
