/* @(#)list.c	1.83 19/04/07 Copyright 1985, 1995, 2000-2019 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)list.c	1.83 19/04/07 Copyright 1985, 1995, 2000-2019 J. Schilling";
#endif
/*
 *	List the content of an archive
 *
 *	Copyright (c) 1985, 1995, 2000-2019 J. Schilling
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
#include <schily/dirent.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include "starsubs.h"
#ifdef	USE_FIND
#include <schily/walk.h>
#endif

extern	FILE	*tarf;
extern	FILE	*vpr;
extern	char	*listfile;
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

EXPORT	void	list		__PR((void));
LOCAL	void	modstr		__PR((FINFO *info, char *s, mode_t  mode));
EXPORT	void	list_file	__PR((FINFO *info));
EXPORT	void	vprint		__PR((FINFO *info));

EXPORT void
list()
{
#ifdef	USE_FIND
extern	struct WALK walkstate;
#endif
		FINFO	finfo;
		FINFO	newinfo;
		TCB	tb;
		TCB	newtb;
	register TCB 	*ptb = &tb;

	fillbytes((char *)&finfo, sizeof (finfo), '\0');
	fillbytes((char *)&newinfo, sizeof (newinfo), '\0');

	if (init_pspace(PS_STDERR, &finfo.f_pname) < 0)
		return;
	if (init_pspace(PS_STDERR, &finfo.f_plname) < 0)
		return;
	if (listnew || listnewf) {
		if (init_pspace(PS_STDERR, &newinfo.f_pname) < 0)
			return;
		if (init_pspace(PS_STDERR, &newinfo.f_plname) < 0)
			return;
	}

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
		if (prblockno)
			(void) tblocks();		/* set curblockno */

		finfo.f_name = finfo.f_pname.ps_path;
		finfo.f_lname = finfo.f_plname.ps_path;
		if (tcb_to_info(ptb, &finfo) == EOF)
			break;
		if (xdebug > 0)
			dump_info(&finfo);

#ifdef	USE_FIND
		if (dofind && !findinfo(&finfo)) {
			void_file(&finfo);
			continue;
		}
#endif

		if (do_subst) {
			subst(&finfo);
		}

		if (listnew || listnewf) {
			/*
			 * nsec beachten wenn im Archiv!
			 */
			if (((finfo.f_mtime > newinfo.f_mtime) ||
			    ((finfo.f_xflags & XF_MTIME) &&
			    (newinfo.f_xflags & XF_MTIME) &&
			    (finfo.f_mtime == newinfo.f_mtime) &&
			    (finfo.f_mnsec > newinfo.f_mnsec))) &&
					(!listnewf || is_file(&finfo))) {
				movebytes(&finfo, &newinfo,
						offsetof(FINFO, f_pname));
				movetcb(&tb, &newtb);
				if (strcpy_pspace(PS_STDERR,
						&newinfo.f_pname,
						finfo.f_name) < 0) {
					newinfo.f_name = "";
				} else {
					newinfo.f_name =
						newinfo.f_pname.ps_path;
				}
				if (newinfo.f_lname[0] != '\0') {
					if (strcpy_pspace(PS_STDERR,
							&newinfo.f_plname,
							finfo.f_lname) < 0) {
						newinfo.f_lname = "";
					} else {
						newinfo.f_lname =
						    newinfo.f_plname.ps_path;
					}
				}
				newinfo.f_flags |= F_HAS_NAME;
			}
			void_file(&finfo);
			continue;
		}
		if (listfile && !hash_lookup(finfo.f_name)) {
			void_file(&finfo);
			continue;
		}
		if (hash_xlookup(finfo.f_name)) {
			void_file(&finfo);
			continue;
		}
		if (havepat && !match(finfo.f_name)) {
			void_file(&finfo);
			continue;
		}
		list_file(&finfo);
		void_file(&finfo);
	}
#ifdef	USE_FIND
	if (dofind) {
		walkhome(&walkstate);
		walkclose(&walkstate);
		free(walkstate.twprivate);
	}
#endif
	if ((listnew || listnewf) && newinfo.f_mtime != 0L) {
		/*
		 * XXX
		 * XXX Achtung!!! tcb_to_info zerstört t_name[NAMSIZ]
		 * XXX und t_linkname[NAMSIZ].
		 * XXX Ist dies noch richtig?
		 * XXX Es sieht so aus as ob nur noch t_name[NAMSIZ] auf ' '
		 * XXX gesetzt wird wenn dort ein null Byte steht.
		 */
		if ((props.pr_flags & PR_CPIO) == 0) {
			/*
			 * Needed to set up the uname/gname fields for the
			 * various TAR headers.
			 */
			tcb_to_info(&newtb, &newinfo);
		}
		list_file(&newinfo);
	}
}

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

	*str++ = '?';				/* Unknown file type */
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
			str[9] = 't';		/* Sticky & exec. by others  */
		} else {
			str[9] = 'T';		/* Sticky but !exec. by oth  */
		}
	}
	if (mode & TSGID) {
		if (mode & TGEXEC) {
			str[6] = 's';		/* Sgid & executable by grp  */
		} else {
			if (!is_file(info))
				str[6] = 'S';	/* Sgid directory, or other  */
			else
				str[6] = 'l';	/* Mandatory lock file	    */
		}
	}
	if (mode & TSUID) {
		if (mode & TUEXEC)
			str[3] = 's';		/* Suid & executable by own. */
		else
			str[3] = 'S';		/* Suid but not executable   */
	}
	i = 10;
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
		char	mstr[13]; /* 10 UNIX chars + ACL '+' XATTR '@' + nul */
		char	lstr[22]; /* ' ' + link count as string - 64 bits */
	static	char	nuid[21]; /* uid as 64 bit long */
	static	char	ngid[21]; /* gid as 64 bit long */
		char	*add = "";

	f = vpr;
	if (prblockno)
		fgtprintf(f, "block %9lld: ", curblockno);
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
/*
 * XXX Übergangsweise, bis die neue Filetypenomenklatur sauber eingebaut ist.
 */
if (xft == 0 || xft == XT_BAD) {
	xft = info->f_xftype = IFTOXT(info->f_type);
	errmsgno(EX_BAD, "XXXXX xftype == 0 (typeflag = '%c' 0x%02X)\n",
				info->f_typeflag, info->f_typeflag);
}
		if (xft == XT_LINK)
			xft = info->f_rxftype;
		{
			char	*p = XTTOSTR(xft);

			if (p)
				mstr[0] = *p;
		}
		if (!paxls) {
			fprintf(f,
				" %s%s %3.*s/%-3.*s %.12s %4.4s ",
				mstr,
				lstr,
				(int)info->f_umaxlen, info->f_uname,
				(int)info->f_gmaxlen, info->f_gname,
				&tstr[4], &tstr[20]);
		} else {
			fprintf(f,
				"%s%s %-8.*s %-8.*s ",
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
	/*
	 * In case of a hardlinked symlink, we currently do not have the symlink
	 * target path and thus cannot check the synlink target. So first check
	 * whether it is a hardlink.
	 */
	if (is_link(info)) {
		if (is_dir(info))
			fgtprintf(f, " directory");
		fprintf(f, " %s %s",
			paxls ? "==" : "link to",
			info->f_lname);
	} else if (is_symlink(info))
		fprintf(f, " -> %s", info->f_lname);
	if (is_volhdr(info))
		fgtprintf(f, " --Volume Header--");
	if (is_multivol(info)) {
		fgtprintf(f, " --Continued at byte %lld--",
						(Llong)info->f_contoffset);
	}
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

		if (prblockno)
			fgtprintf(f, "block %9lld: ", curblockno);
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
				fgtprintf(f, "%s%s%s directory %s %s\n",
					mode, info->f_name, add,
					paxls ? "==" : "link to",
					info->f_lname);
			} else {
				fgtprintf(f, "%s%s%s directory\n", mode,
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
			fgtprintf(f, "%s%s special\n", mode, info->f_name);
		} else {
			fgtprintf(f, "%s%s %lld bytes, %lld tape blocks\n",
				mode, info->f_name, (Llong)info->f_size,
				(Llong)tarblocks(info->f_rsize));
		}
	}
}
