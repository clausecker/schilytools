/* @(#)longnames.c	1.49 08/12/22 Copyright 1993, 1995, 2001-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)longnames.c	1.49 08/12/22 Copyright 1993, 1995, 2001-2008 J. Schilling";
#endif
/*
 *	Handle filenames that cannot fit into a single
 *	string of 100 charecters
 *
 *	Copyright (c) 1993, 1995, 2001-2008 J. Schilling
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
#include "star.h"
#include "props.h"
#include "table.h"
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/schily.h>
#include "starsubs.h"
#include "movearch.h"
#include "checkerr.h"

extern	BOOL	no_dirslash;

LOCAL	void	enametoolong	__PR((char *name, int len, int maxlen));
LOCAL	char	*split_posix_name __PR((char *name, int namlen, int add));
EXPORT	BOOL	name_to_tcb	__PR((FINFO *info, TCB *ptb));
EXPORT	void	tcb_to_name	__PR((TCB *ptb, FINFO *info));
EXPORT	void	tcb_undo_split	__PR((TCB *ptb, FINFO *info));
EXPORT	int	tcb_to_longname	__PR((TCB *ptb, FINFO *info));
EXPORT	void	write_longnames	__PR((FINFO *info));
LOCAL	void	put_longname	__PR((FINFO *info,
					char *name, int namelen, char *tname,
							Ulong  xftype));

LOCAL void
enametoolong(name, len, maxlen)
	char	*name;
	int	len;
	int	maxlen;
{
	if (!errhidden(E_NAMETOOLONG, name)) {
		if (!errwarnonly(E_NAMETOOLONG, name))
			xstats.s_toolong++;
		errmsgno(EX_BAD, "%s: Name too long (%d > %d chars)\n",
							name, len, maxlen);
		(void) errabort(E_NAMETOOLONG, name, TRUE);
	}
}


LOCAL char *
split_posix_name(name, namlen, add)
	char	*name;
	int	namlen;
	int	add;
{
	register char	*low;	/* Lower margin to start searching for '/' */
	register char	*high;	/* Upper margin to start searching for '/' */

	if (namlen+add > props.pr_maxprefix+1+props.pr_maxsname) {
		/*
		 * Name too long to be split (does not fit in pfx + '/' + name)
		 */
		if ((props.pr_nflags & PR_LONG_NAMES) == 0) /* No longnames */
			enametoolong(name, namlen+add,
				props.pr_maxprefix+1+props.pr_maxsname);
		return (NULL);
	}
	/*
	 * 'low' is set to (namelen - 100) because we cannot split more than
	 * 100 chars before the end of the name.
	 * 'high' is set to 155 because we cannot put more than 155 chars into
	 * the prefix.
	 * Then start to look backwards from the rightmost position if there is
	 * slash where we may split.
	 */
	low = &name[namlen+add - props.pr_maxsname];
	if (--low < name)
		low = name;
	high = &name[props.pr_maxprefix > namlen ? namlen:props.pr_maxprefix];

#ifdef	DEBUG
error("low: %d:%s high: %d:'%c',%s\n",
			strlen(low), low, strlen(high), *high, high);
#endif
	high++;
	while (--high >= low)
		if (*high == '/')
			break;
	if (high < low) {
		if ((props.pr_nflags & PR_LONG_NAMES) == 0) {
			if (!errhidden(E_NAMETOOLONG, name)) {
				if (!errwarnonly(E_NAMETOOLONG, name))
					xstats.s_toolong++;
				errmsgno(EX_BAD,
				"%s: Name too long (cannot split)\n",
									name);
				(void) errabort(E_NAMETOOLONG, name, TRUE);
			}
		}
		return (NULL);
	}
#ifdef	DEBUG
error("solved: add: %d prefix: %d suffix: %d\n",
			add, high-name, strlen(high+1)+add);
#endif
	return (high);
}

/*
 * Es ist sichergestelt, daß namelen >= 1 ist.
 */
EXPORT BOOL
name_to_tcb(info, ptb)
	FINFO	*info;
	TCB	*ptb;
{
	char	*name = info->f_name;
	int	namelen = info->f_namelen;
	int	add = 0;
	char	*np = NULL;

	if (namelen == 0)
		raisecond("name_to_tcb: namelen", 0L);

	/*
	 * We need a test without 'add' because we currently never add a slash
	 * at the end of a directiry name when in CPIO mode.
	 */
	if (namelen > props.pr_maxnamelen) {
		enametoolong(name, namelen, props.pr_maxnamelen);
		return (FALSE);
	}
	if ((props.pr_flags & PR_CPIO) != 0)
		return (TRUE);

	if (is_dir(info) && !no_dirslash && name[namelen-1] != '/')
		add++;

	if ((namelen+add) <= props.pr_maxsname) {	/* Fits in shortname */
		if (add)
			strcatl(ptb->dbuf.t_name, name, "/", (char *)NULL);
		else
			strcpy(ptb->dbuf.t_name, name);
		return (TRUE);
	}

	if (namelen+add > props.pr_maxnamelen) {	/* Now we know 'add' */
		enametoolong(name, namelen+add, props.pr_maxnamelen);
		return (FALSE);
	}
	if (props.pr_nflags & PR_POSIX_SPLIT)
		np = split_posix_name(name, namelen, add);
	if (np == NULL) {
		/*
		 * Cannot split
		 * (namelen+add <= props.pr_maxnamelen) has been checked before
		 */
		if (props.pr_nflags & PR_LONG_NAMES) {
			if (props.pr_flags & PR_XHDR)
				info->f_xflags |= XF_PATH;
			else
				info->f_flags |= F_LONGNAME;
			if (add)
				info->f_flags |= F_ADDSLASH;
			strncpy(ptb->dbuf.t_name, name, props.pr_maxsname);
			return (TRUE);
		} else {
			/*
			 * In case of PR_POSIX_SPLIT we did already print the
			 * error message.
			 */
			if ((props.pr_nflags & PR_POSIX_SPLIT) == 0) {
				enametoolong(name, namelen+add,
							props.pr_maxnamelen);
			}
			return (FALSE);
		}
	}

	/*
	 * Do actual splitting based on split name pointer 'np'.
	 */
	if (add)
		strcatl(ptb->dbuf.t_name, &np[1], "/", (char *)NULL);
	else
		strcpy(ptb->dbuf.t_name, &np[1]);
	strncpy(ptb->dbuf.t_prefix, name, np - name);
	info->f_flags |= F_SPLIT_NAME;
	return (TRUE);
}

/*
 * This function is only called by tcb_to_info().
 * If we ever decide to call it from somewhere else check if the linkname
 * kludge for 100 char linknames does not make problems.
 */
EXPORT void
tcb_to_name(ptb, info)
	TCB	*ptb;
	FINFO	*info;
{
	/*
	 * Name has already been set up from somwhere else.
	 * We have nothing to do.
	 */
	if (info->f_flags & F_HAS_NAME)
		return;

	if ((info->f_flags & F_LONGLINK) == 0 &&	/* name from 'K' head*/
	    (info->f_xflags & XF_LINKPATH) == 0) {	/* name from 'x' head*/

		/*
		 * Our caller has set ptb->dbuf.t_linkname[NAMSIZ] to '\0'
		 * if the link name len is exactly 100 chars.
		 */
		strcpy(info->f_lname, ptb->dbuf.t_linkname);
	}

	/*
	 * Name has already been set up because it is a very long name.
	 */
	if (info->f_flags & F_LONGNAME)
		return;
	/*
	 * Name has already been set up from a POSIX.1-2001 extended header.
	 */
	if (info->f_xflags & XF_PATH)
		return;

	if (props.pr_nflags & PR_POSIX_SPLIT) {
		strcatl(info->f_name, ptb->dbuf.t_prefix,
					*ptb->dbuf.t_prefix?"/":"",
					ptb->dbuf.t_name, (char *)NULL);
	} else {
		strcpy(info->f_name, ptb->dbuf.t_name);
	}
}

EXPORT void
tcb_undo_split(ptb, info)
	TCB	*ptb;
	FINFO	*info;
{
	fillbytes(ptb->dbuf.t_name, NAMSIZ, '\0');
	fillbytes(ptb->dbuf.t_prefix, props.pr_maxprefix, '\0');

	info->f_flags &= ~F_SPLIT_NAME;

	if (props.pr_flags & PR_XHDR)
		info->f_xflags |= XF_PATH;
	else
		info->f_flags |= F_LONGNAME;

	strncpy(ptb->dbuf.t_name, info->f_name, props.pr_maxsname);
}

/*
 * A bad idea to do this here!
 * We have to set up a more generalized pool of namebuffers wich are allocated
 * on an actual MAX_PATH base or even better allocated on demand.
 *
 * XXX If we change the code to allocate the data, we need to make sure that
 * XXX the allocated data holds one byte more than needed as movearch.c
 * XXX adds a second null byte to the buffer to enforce null-termination.
 */
#ifdef	__needed__
char	longlinkname[PATH_MAX+1];
#endif

EXPORT int
tcb_to_longname(ptb, info)
	register TCB	*ptb;
	register FINFO	*info;
{
	move_t	move;
	Ullong	ull;

	/*
	 * File size is strlen of name + 1
	 */
	stolli(ptb->dbuf.t_size, &ull);
	info->f_size = ull;
	info->f_rsize = info->f_size;
	if (info->f_size > PATH_MAX) {
		/*
		 * We do not know the name here,
		 * we only have the short ptb->dbuf.t_name
		 */
/*		if (!errhidden(E_NAMETOOLONG, name)) {*/
/*			if (!errwarnonly(E_NAMETOOLONG, name))*/
				xstats.s_toolong++;
			errmsgno(EX_BAD,
			"Long name too long (%lld) ignored.\n",
							(Llong)info->f_size);
/*			(void) errabort(E_NAMETOOLONG, name, TRUE);*/
/*		}*/
		void_file(info);
		return (get_tcb(ptb));
	}
	if (ptb->dbuf.t_linkflag == LF_LONGNAME) {
		if ((info->f_xflags & XF_PATH) != 0) {
			/*
			 * Ignore old star/gnutar extended headers for very
			 * long filenames if we already found a POSIX.1-2001
			 * compliant long PATH name.
			 */
			void_file(info);
			return (get_tcb(ptb));
		}
		info->f_namelen = info->f_size -1;
		info->f_flags |= F_LONGNAME;
		move.m_data = info->f_name;
	} else {
		if ((info->f_xflags & XF_LINKPATH) != 0) {
			/*
			 * Ignore old star/gnutar extended headers for very
			 * long linknames if we already found a POSIX.1-2001
			 * compliant long LINKPATH name.
			 */
			void_file(info);
			return (get_tcb(ptb));
		}
		info->f_lnamelen = info->f_size -1;
		info->f_flags |= F_LONGLINK;
		move.m_data = info->f_lname;
	}
	move.m_flags = 0;
	if (xt_file(info, vp_move_from_arch, &move, 0, "moving long name") < 0)
		die(EX_BAD);

	return (get_tcb(ptb));
}

EXPORT void
write_longnames(info)
	register FINFO	*info;
{
	/*
	 * XXX Should test for F_LONGNAME & F_FLONGLINK
	 */
	if ((info->f_flags & F_LONGNAME) ||
	    (info->f_namelen > props.pr_maxsname)) {
		put_longname(info, info->f_name, info->f_namelen+1,
						"././@LongName", XT_LONGNAME);
	}
	if ((info->f_flags & F_LONGLINK) ||
	    (info->f_lnamelen > props.pr_maxslname)) {
		put_longname(info, info->f_lname, info->f_lnamelen+1,
						"././@LongLink", XT_LONGLINK);
	}
}

LOCAL void
put_longname(info, name, namelen, tname, xftype)
	FINFO	*info;
	char	*name;
	int	namelen;
	char	*tname;
	Ulong	xftype;
{
	FINFO	finfo;
	TCB	tb;
	TCB	*ptb;
	move_t	move;

	fillbytes((char *)&finfo, sizeof (finfo), '\0');

	if ((ptb = (TCB *)get_block(TBLOCK)) == NULL)
		ptb = &tb;
	else
		finfo.f_flags |= F_TCB_BUF;
	filltcb(ptb);

	strcpy(ptb->dbuf.t_name, tname);

	move.m_flags = 0;
	if ((info->f_flags & F_ADDSLASH) != 0 && xftype == XT_LONGNAME) {
		/*
		 * A slash is only added to the filename and not to the
		 * linkname.
		 */
		move.m_flags |= MF_ADDSLASH;
		namelen++;
	}
	finfo.f_mode = TUREAD|TUWRITE;
	finfo.f_rsize = finfo.f_size = namelen;
	finfo.f_xftype = xftype;
	info_to_tcb(&finfo, ptb);
	write_tcb(ptb, &finfo);

	move.m_data = name;
	move.m_size = finfo.f_size;
	cr_file(&finfo, vp_move_to_arch, &move, 0, "moving long name");
}
