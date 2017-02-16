/* @(#)list.c	1.78 17/02/16 Copyright 1985, 1995, 2000-2016 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)list.c	1.78 17/02/16 Copyright 1985, 1995, 2000-2016 J. Schilling";
#endif
/*
 *	List the content of an archive
 *
 *	Copyright (c) 1985, 1995, 2000-2016 J. Schilling
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
#include <schily/dirent.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/time.h>
#include <schily/string.h>
#include <schily/schily.h>
#ifdef	USE_FIND
#include <schily/walk.h>
#endif
#include <schily/strar.h>

#include "table.h"

/*
 * Definitions for the t_mode field
 */
#define	TSUID		04000	/* Set UID on execution */
#define	TSGID		02000	/* Set GID on execution */
#define	TSVTX		01000	/* On directories, restricted deletion flag */
#define	TUREAD		00400	/* Read by owner */
#define	TUWRITE		00200	/* Write by owner special */
#define	TUEXEC		00100	/* Execute/search by owner */
#define	TGREAD		00040	/* Read by group */
#define	TGWRITE		00020	/* Write by group */
#define	TGEXEC		00010	/* Execute/search by group */
#define	TOREAD		00004	/* Read by other */
#define	TOWRITE		00002	/* Write by other */
#define	TOEXEC		00001	/* Execute/search by other */

#define	TALLMODES	07777	/* The low 12 bits mentioned in the standard */

#define	is_dir(i)	((i)->f_rxftype == XT_DIR)
#define	is_symlink(i)	((i)->f_rxftype == XT_SLINK)
#define	is_link(i)	((i)->f_rxftype == XT_LINK)
#define	is_special(i)	((i)->f_rxftype > XT_DIR)


extern	FILE	*vpr;
extern	Llong	curblockno;

extern	time_t	sixmonth;		/* 6 months before limit (ls)	*/
extern	time_t	now;			/* now limit (ls)		*/

extern	BOOL	havepat;
extern	int	iftype;
extern	BOOL	paxls;
extern	int	xdebug;
extern	BOOL	numeric;
extern	int	verbose;
extern	BOOL	prblockno;
extern	BOOL	tpath;
extern	BOOL	cflag;
extern	BOOL	xflag;
extern	BOOL	interactive;

extern	BOOL	acctime;
extern	BOOL	no_dirslash;
extern	BOOL	Ctime;
extern	BOOL	prinodes;

extern	BOOL	listnew;
extern	BOOL	listnewf;

extern	BOOL	do_subst;

#ifdef	USE_FIND
extern	BOOL	dofind;
#endif

/*--------------------------------------------------------------------------*/
#define	paxls		0
#define	tpath		0
#define	no_dirslash	0
#define	numeric		0
#define	acctime		0
#define	interactive	0
#define	prinodes	0
#define	list_file	strar_list_file
#define	vprint		strar_vprint
#define	vpr		info->f_list;
#define	cflag		(info->f_cmdflags & CMD_CREATE)
#define	xflag		(info->f_cmdflags & CMD_XTRACT)
#define	Ctime		(info->f_cmdflags & CMD_CTIME)
#define	verbose		(info->f_cmdflags & CMD_VERBOSE)
/*--------------------------------------------------------------------------*/

LOCAL	void	modstr		__PR((FINFO *info, char *s, mode_t  mode));
EXPORT	void	list_file	__PR((FINFO *info));
EXPORT	void	vprint		__PR((FINFO *info));

/*
 * Convert POSIX.1 TAR mode/permission flags into string.
 */
LOCAL void
#ifdef	PROTOTYPES
modstr(FINFO *info, char *s, register mode_t  mode)
#else
modstr(info, s, mode)
		FINFO	*info;
		char	*s;
	register mode_t	mode;
#endif
{
	register char	*mstr = "xwrxwrxwr";
	register char	*str = s;
	register int	i;

	for (i = 9; --i >= 0; ) {
		if (mode & (1 << i))
			*str++ = mstr[i];
		else
			*str++ = '-';
	}
#ifdef	USE_ACL
	*str++ = ' ';
#endif
#ifdef	USE_XATTR
	*str++ = '\0';				/* Don't claim space for '@' */
#endif
	*str = '\0';
	str = s;
	if (mode & TSVTX) {
		if (mode & TOEXEC) {
			str[8] = 't';		/* Sticky & exec. by others  */
		} else {
			str[8] = 'T';		/* Sticky but !exec. by oth  */
		}
	}
	if (mode & TSGID) {
		if (mode & TGEXEC) {
			str[5] = 's';		/* Sgid & executable by grp  */
		} else {
			if (is_dir(info))
				str[5] = 'S';	/* Sgid directory	    */
			else
				str[5] = 'l';	/* Mandatory lock file	    */
		}
	}
	if (mode & TSUID) {
		if (mode & TUEXEC)
			str[2] = 's';		/* Suid & executable by own. */
		else
			str[2] = 'S';		/* Suid but not executable   */
	}
	i = 9;
#ifdef	USE_ACL
	if ((info->f_xflags & (XF_ACL_ACCESS|XF_ACL_DEFAULT|XF_ACL_ACE)) != 0)
		str[i++] = '+';
#endif
#ifdef	USE_XATTR
	if ((info->f_xflags & XF_XATTR) != 0)
		str[i++] = '@';
#endif
	i++;	/* Make lint believe that we always use i. */
}

EXPORT void
list_file(info)
	register FINFO	*info;
{
		FILE	*f;
		time_t	*tp;
		char	*tstr;
		char	mstr[12]; /* 9 UNIX chars + ACL '+' XATTR '@' + nul */
		char	lstr[22]; /* ' ' + link count as string - 64 bits */
	static	char	nuid[21]; /* uid as 64 bit long */
	static	char	ngid[21]; /* gid as 64 bit long */
		char	*add = "";

	f = vpr;
	if (cflag)
		fprintf(f, "a ");
	else if (xflag)
		fprintf(f, "x ");

	if (prinodes && info->f_ino > 0)
		fprintf(f, "%7llu ", (Llong)info->f_ino);
	if (cflag && is_dir(info) && !no_dirslash) {
		int	len = info->f_namelen;

		if (len == 0)
			len = strlen(info->f_name);
		if (info->f_name[len-1] != '/')
			add = "/";
	}
	if (verbose) {
		register Uint	xft = info->f_xftype;

		tp = acctime ? &info->f_atime :
				(Ctime ? &info->f_ctime : &info->f_mtime);
		tstr = ctime(tp);
		if (numeric || info->f_uname == NULL) {
			sprintf(nuid, "%lld", (Llong)info->f_uid);
			info->f_uname = nuid;
			info->f_umaxlen = sizeof (nuid)-1;
		}
		if (numeric || info->f_gname == NULL) {
			sprintf(ngid, "%lld", (Llong)info->f_gid);
			info->f_gname = ngid;
			info->f_gmaxlen = sizeof (ngid)-1;
		}

		if (!paxls) {
			if (is_special(info))
				fprintf(f, "%3lu %3lu",
					info->f_rdevmaj, info->f_rdevmin);
			else
				fprintf(f, "%7llu", (Llong)info->f_size);
		}
		modstr(info, mstr, info->f_mode);

		if (paxls && info->f_nlink == 0 && is_link(info)) {
			info->f_nlink = 2;
		}
		if (paxls || info->f_nlink > 0) {
			/*
			 * UNIX ls uses %3d for the link count
			 * and does not claim space for ACL '+'
			 */
			js_sprintf(lstr, " %2llu", (Ullong)info->f_nlink);
		} else {
			lstr[0] = 0;
		}

		if (xft == XT_LINK)
			xft = info->f_rxftype;
		if (!paxls) {
			fprintf(f,
				" %s%s%s %3.*s/%-3.*s %.12s %4.4s ",
#ifdef	OLD
				typetab[info->f_filetype & 07],
#else
				XTTOSTR(xft),
#endif
				mstr,
				lstr,
				(int)info->f_umaxlen, info->f_uname,
				(int)info->f_gmaxlen, info->f_gname,
				&tstr[4], &tstr[20]);
		} else {
			fprintf(f,
				"%s%s%s %-8.*s %-8.*s ",
#ifdef	OLD
				typetab[info->f_filetype & 07],
#else
				XTTOSTR(xft),
#endif
				mstr,
				lstr,
				(int)info->f_umaxlen, info->f_uname,
				(int)info->f_gmaxlen, info->f_gname);
			if (is_special(info))
				fprintf(f, "%3lu %3lu",
					info->f_rdevmaj, info->f_rdevmin);
			else
				fprintf(f, "%7llu", (Llong)info->f_size);
			if ((*tp < sixmonth) || (*tp > now)) {
				fprintf(f, " %.6s  %4.4s ",
					&tstr[4], &tstr[20]);
			} else {
				fprintf(f, " %.12s ",
					&tstr[4]);
			}
		}
	}
	fprintf(f, "%s%s", info->f_name, add);
	if (tpath) {
		fprintf(f, "\n");
		return;
	}
	if (is_link(info)) {
		if (is_dir(info))
			fprintf(f, " directory");
		fprintf(f, " %s %s",
			paxls ? "==" : "link to",
			info->f_lname);
	}
	if (is_symlink(info))
		fprintf(f, " -> %s", info->f_lname);
	fprintf(f, "\n");
}

EXPORT void
vprint(info)
	FINFO	*info;
{
		FILE	*f;
	char	*mode;
	char	*add = "";

	if (verbose || interactive) {
		if (verbose > 1) {
			list_file(info);
			return;
		}

		f = vpr;

		if (cflag)
			mode = "a ";
		else if (xflag)
			mode = "x ";
		else
			mode = "";

		if (cflag && is_dir(info) && !no_dirslash) {
			int	len = info->f_namelen;

			if (len == 0)
				len = strlen(info->f_name);
			if (info->f_name[len-1] != '/')
				add = "/";
		}
		if (tpath) {
			fprintf(f, "%s%s\n", info->f_name, add);
			return;
		}
		if (is_dir(info)) {
			if (is_link(info)) {
				fprintf(f, "%s%s%s directory %s %s\n",
					mode, info->f_name, add,
					paxls ? "==" : "link to",
					info->f_lname);
			} else {
				fprintf(f, "%s%s%s directory\n", mode,
							info->f_name, add);
			}
		} else if (is_link(info)) {
			fprintf(f, "%s%s %s %s\n",
				mode, info->f_name,
				paxls ? "==" : "link to",
				info->f_lname);
		} else if (is_symlink(info)) {
			fprintf(f, "%s%s %s %s\n",
				mode, info->f_name,
				paxls ? "->" : "symbolic link to",
				info->f_lname);
		} else if (is_special(info)) {
			fprintf(f, "%s%s special\n", mode, info->f_name);
		} else {
			fprintf(f, "%s%s %lld bytes\n",
				mode, info->f_name, (Llong)info->f_size);
		}
	}
}
