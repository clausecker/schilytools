/* @(#)xheader.c	1.97 18/10/24 Copyright 2001-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)xheader.c	1.97 18/10/24 Copyright 2001-2018 J. Schilling";
#endif
/*
 *	Handling routines to read/write, parse/create
 *	POSIX.1-2001 extended archive headers
 *
 *	Copyright (c) 2001-2018 J. Schilling
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
#include <schily/unistd.h>	/* getpagesize() */
#include <schily/libport.h>	/* getpagesize() */
#include "star.h"
#include "props.h"
#include "table.h"
#include <schily/dirent.h>
#include <schily/standard.h>
#include <schily/string.h>
#define	__XDEV__	/* Needed to activate _dev_major()/_dev_minor() */
#include <schily/device.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#include <schily/idcache.h>
#include "starsubs.h"
#include "movearch.h"
#include "xtab.h"
#include "pathname.h"

extern	BOOL	no_xheader;
extern	BOOL	nowarn;
extern	BOOL	binflag;

extern	GINFO	*grip;				/* Global read info pointer	*/

#define	MAX_UNAME	64	/* The maximum length of a user/group name */

/*
 * Flags for gen_text()
 */
#define	T_ADDSLASH	1	/* Add slash to the argument	*/
#define	T_UTF8		2	/* Convert arg to UTF-8 coding	*/

typedef struct _unknown unkn_t;

struct _unknown {
	unkn_t	*u_next;
	char	u_name[1];
};

LOCAL	void	_xbinit		__PR((void));
EXPORT	void	xbinit		__PR((void));
LOCAL	void	xbgrow		__PR((int newsize));
EXPORT	void	xbbackup	__PR((void));
EXPORT	void	xbrestore	__PR((void));
EXPORT	int	xhsize		__PR((void));
LOCAL	void	write_xhdr	__PR((int type));
EXPORT	void	info_to_xhdr	__PR((FINFO * info, TCB * ptb));
LOCAL	void	check_xtime	__PR((char *keyword, FINFO *info));
EXPORT	void	gen_xtime	__PR((char *keyword, time_t sec, Ulong nsec));
EXPORT	void	gen_unumber	__PR((char *keyword, Ullong arg));
EXPORT	void	gen_number	__PR((char *keyword, Llong arg));
LOCAL	void	gen_iarray	__PR((char *keyword, ino_t *arg, int ents, int len));
EXPORT	void	gen_text	__PR((char *keyword, char *arg, int alen,
								Uint flags));
LOCAL	int	len_len		__PR((int len));
LOCAL	xtab_t	*lookup		__PR((char *cmd, int clen, xtab_t *cp));
EXPORT	int	tcb_to_xhdr	__PR((TCB * ptb, FINFO * info));
EXPORT	BOOL	xhparse		__PR((FINFO *info, char	*p, char *ep));
LOCAL	void	print_unknown	__PR((char *keyword));
EXPORT	void	xh_rangeerr	__PR((char *keyword, char *arg, int len));
LOCAL	void	print_toolong	__PR((char *keyword, char *arg, int len));
LOCAL	void	get_xvolhdr	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	info_xcopy	__PR((FINFO *ninfo, FINFO *oinfo));
EXPORT	BOOL	get_xtime	__PR((char *keyword, char *arg, int len,
						time_t *secp, long *nsecp));
LOCAL	void	get_atime	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_ctime	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_mtime	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
#ifdef	__needed_
EXPORT	BOOL	get_number	__PR((char *keyword, char *arg, Llong *llp));
#endif
LOCAL	BOOL	get_xnumber	__PR((char *keyword, char *arg, Ullong *llp, char *type));
EXPORT	BOOL	get_unumber	__PR((char *keyword, char *arg, Ullong *ullp, Ullong maxval));
EXPORT	BOOL	get_snumber	__PR((char *keyword, char *arg, Ullong *ullp, BOOL *negp, Ullong minval, Ullong maxval));
LOCAL	void	get_uid		__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_gid		__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_uname	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_gname	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_path	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_lpath	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_size	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_realsize	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_offset	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_major	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_minor	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_minorbits	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_dev		__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_ino		__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_nlink	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_filetype	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
#ifdef	USE_ACL
LOCAL	void	get_acl_type	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_acl_access	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_acl_default	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_acl_ace	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
#endif
#ifdef  USE_XATTR
LOCAL	void	get_attr	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
#endif
#ifdef	USE_FFLAGS
LOCAL	void	get_xfflags	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
#endif
LOCAL	void	get_dir		__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_iarray	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_release	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_archtype	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_hdrcharset	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	get_dummy	__PR((FINFO *info, char *keyword, int klen, char *arg, int len));
LOCAL	void	unsup_arg	__PR((char *keyword, char *arg));
LOCAL	void	bad_utf8	__PR((char *keyword, char *arg));

/*
 * Convert unsigned integer into a decimal string.
 * The parameter 'val' is of the needed size (basic type), so it is as fast
 * as possible.
 *
 * The string is written starting from the end backwards for best speed.
 */
LOCAL	Uchar	dtab[] = "0123456789";

#define	ui2decimal(val, np)	do {					      \
					*--(np) = dtab[(val) % (unsigned)10]; \
					(val) = (val) / (unsigned)10;	      \
				} while ((val) > 0)

#define	scopy(to, from)		while ((*(to)++ = *(from)++) != '\0')	\
					;


LOCAL	char	*xbuf;	/* Space used to prepare I/O from/to extended headers */
LOCAL	int	xblen;	/* the length of the buffer for the extended headers */
LOCAL	int	xbidx;	/* The index where we start to prepare next entry    */

EXPORT	BOOL	ghdr;	/* We need wo write a 'g'lobal header		*/
LOCAL	FINFO	ginfo;	/* The 'g'lobal FINFO data			*/
LOCAL	char	*guname;
LOCAL	char	*ggname;

LOCAL	unkn_t	*unkn;	/* A list of unknown keywords to print warnings once */
LOCAL	Ulong	badf;	/* A list of bad flags				*/

/*
 * Important for the correctness of gen_number(): As long as we stay <= 55
 * chars for the keyword, a 128 bit number entry will fit into 100 chars.
 *
 *			x_name,		x_namelen, x_func,	x_flag
 */
LOCAL xtab_t xtab[] = {
			{ "atime",		5, get_atime,	0	},
			{ "ctime",		5, get_ctime,	0	},
			{ "mtime",		5, get_mtime,	0	},

			{ "uid",		3, get_uid,	0	},
			{ "uname",		5, get_uname,	0	},
			{ "gid",		3, get_gid,	0	},
			{ "gname",		5, get_gname,	0	},

			{ "path",		4, get_path,	0	},
			{ "linkpath",		8, get_lpath,	0	},

			{ "size",		4, get_size,	0	},

			{ "charset",		7, get_dummy,	0	},
			{ "comment",		7, get_dummy,	0	},
			{ "hdrcharset",		10, get_hdrcharset,	0	},
/* "BINARY" */

			{ "SCHILY.devmajor",	15, get_major,	0	},
			{ "SCHILY.devminor",	15, get_minor,	0	},

#ifdef	USE_ACL
			{ "SCHILY.acl.access",	17, get_acl_access, 0	},
			{ "SCHILY.acl.default",	18, get_acl_default, 0	},
			{ "SCHILY.acl.ace",	14, get_acl_ace, 0	},
			{ "SCHILY.acl.type",	15, get_acl_type, 0	},
#else
/*
 * We don't want star to complain about unknown extended headers when it
 * has been compiled without ACL support.
 */
			{ "SCHILY.acl.access",	17, get_dummy,	0	},
			{ "SCHILY.acl.default",	18, get_dummy,	0	},
			{ "SCHILY.acl.ace",	14, get_dummy,	0	},
			{ "SCHILY.acl.type",	15, get_dummy,	0	},
#endif
#ifdef  USE_XATTR
			{ "SCHILY.xattr.*",	14, get_attr,	0	},
#else
			{ "SCHILY.xattr.*",	14, get_dummy,	0	},
#endif

#ifdef	USE_FFLAGS
			{ "SCHILY.fflags",	13, get_xfflags, 0	},
#else
/*
 * We don't want star to complain about unknown extended headers when it
 * has been compiled without extended file flag support.
 */
			{ "SCHILY.fflags",	13, get_dummy,	0	},
#endif
			{ "SCHILY.dev",		10, get_dev,	0	},
			{ "SCHILY.devminorbits", 19, get_minorbits,	0	},
			{ "SCHILY.ino",		10, get_ino,	0	},
			{ "SCHILY.nlink",	12, get_nlink,	0	},
			{ "SCHILY.filetype",	15, get_filetype, 0	},
			{ "SCHILY.tarfiletype",	18, get_filetype, 0	},
			{ "SCHILY.realsize",	15, get_realsize, 0	},
			{ "SCHILY.offset",	13, get_offset, 0	},

			{ "SCHILY.dir",		10, get_dir,	0	},
			{ "SCHILY.ddev",	11, get_dummy,	0	},
			{ "SCHILY.dino",	11, get_iarray,	0	},

			{ "SCHILY.release",	14, get_release, 0	},
			{ "SCHILY.archtype",	15, get_archtype, 0	},
			{ "SCHILY.volhdr.*",	15, get_xvolhdr, 0	},

			{ "SUN.devmajor",	12, get_major,	0	},
			{ "SUN.devminor",	12, get_minor,	0	},

			{ NULL,			0, NULL,	0	}};

/*
 * Initialize the growable buffer used for reading the extended headers
 */
LOCAL void
_xbinit()
{
	xbuf = ___malloc(1, "growable xheader");
	xblen = 1;
	xbidx = 0;
}

EXPORT void
xbinit()
{
	_xbinit();

	init_pspace(PS_EXIT, &ginfo.f_pname);
	ginfo.f_name = ginfo.f_pname.ps_path;
	ginfo.f_name[0] = '\0';

	init_pspace(PS_EXIT, &ginfo.f_plname);
	ginfo.f_lname = ginfo.f_plname.ps_path;
	ginfo.f_lname[0] = '\0';

	ginfo.f_uname = ___malloc(MAX_UNAME+1, "global user name");
	ginfo.f_gname = ___malloc(MAX_UNAME+1, "global group name");
	ginfo.f_uname[0] = '\0';
	ginfo.f_gname[0] = '\0';
	ginfo.f_devminorbits = 0;
	ggname = ginfo.f_gname;
	ginfo.f_xflags = 0;
	guname = ginfo.f_uname;
	ggname = ginfo.f_gname;
}

/*
 * Grow the growable buffer used for reading the extended headers
 */
LOCAL void
xbgrow(newsize)
	int	newsize;
{
	char	*newbuf;
	int	i;
	int	ps = getpagesize();

	/*
	 * grow in pagesize units
	 */
	for (i = 0; i < newsize; i += ps)
		/* LINTED */
		;
	newsize = i + xblen;
	newbuf = ___realloc(xbuf, newsize, "growable xheader");
	xbuf = newbuf;
	xblen = newsize;
}

/*
 * Variables used to allow us to create an extended header while we write one
 */
LOCAL	char	*oxbuf;	/* Space used to prepare I/O from/to extended headers */
LOCAL	int	oxblen;	/* the length of the buffer for the extended headers */
LOCAL	int	oxbidx;	/* The index where we start to prepare next entry    */

EXPORT void
xbbackup()
{
	oxbuf  = xbuf;
	oxblen = xblen;
	oxbidx = xbidx;

	_xbinit();
}

EXPORT void
xbrestore()
{
	free(xbuf);

	xbuf  = oxbuf;
	xblen = oxblen;
	xbidx = oxbidx;
}

EXPORT int
xhsize()
{
	return (xbidx);
}

LOCAL void
write_xhdr(type)
	int	type;
{
	FINFO	finfo;
	TCB	tb;
	TCB	*xptb;
	move_t	move;

	fillbytes((char *)&finfo, sizeof (finfo), '\0');

	if ((xptb = (TCB *)get_block(TBLOCK)) == NULL)
		xptb = &tb;
	else
		finfo.f_flags |= F_TCB_BUF;
	filltcb(xptb);
	strcpy(xptb->dbuf.t_name, "././@PaxHeader");
	finfo.f_name = xptb->dbuf.t_name;
	finfo.f_mode = TUREAD|TUWRITE;
	finfo.f_rsize = finfo.f_size = xbidx;
	finfo.f_xftype = XT_FILE;
	info_to_tcb(&finfo, xptb);
	xptb->dbuf.t_linkflag = (char)type;
	write_tcb(xptb, &finfo);

	move.m_data = xbuf;
	move.m_size = finfo.f_size;
	move.m_flags = 0;
	cr_file(&finfo, vp_move_to_arch, &move, 0, "moving extended header");

	xbidx = 0;	/* Reset xbuffer index to start of buffer. */
}

/*
 * Prepare and write out the extended header
 */
/* ARGSUSED */
EXPORT void
info_to_xhdr(info, ptb)
	register FINFO	*info;
	register TCB	*ptb;
{
		char	name[MAX_UNAME+1];
	register Ulong	xflags;
extern	long	hdrtype;
extern	BOOL	dodump;

	if (no_xheader)
		return;

	if (ghdr) {
		/*
		 * Delayed writing of a 'g' header is required.
		 */
		write_xhdr('g');
		ghdr = FALSE;
	}

	xflags = info->f_xflags & (props.pr_xhmask | XF_NOTIME);
	/*
	 * Unless we really don't want extended sub-second resolution
	 * timestamps or a specific selection of timestams has been set up,
	 * include all times (atime/ctime/mtime) if we need to include extended
	 * headers at all.
	 */
	if ((xflags & (XF_ATIME|XF_CTIME|XF_MTIME|XF_NOTIME)) == 0)
		xflags |= (XF_ATIME|XF_CTIME|XF_MTIME);

#ifdef	DEBUG_XHDR
	xflags = 0xffffffff;
#endif
	if ((xflags & ~XF_NOTIME) == 0)
		return;

	if (xflags & XF_ATIME) {
		check_xtime("atime", info);
		gen_xtime("atime", info->f_atime, info->f_ansec);
	}
	if (xflags & XF_CTIME) {
		check_xtime("ctime", info);
		gen_xtime("ctime", info->f_ctime, info->f_cnsec);
	}
	if (xflags & XF_MTIME) {
		check_xtime("mtime", info);
		gen_xtime("mtime", info->f_mtime, info->f_mnsec);
	}

	if (xflags & XF_UID) {
		/* LINTED */
		if (info->f_uid >= 0)
			gen_unumber("uid", (Ullong)info->f_uid);
		else
			gen_number("uid", (Llong)info->f_uid);
	}
	if (xflags & XF_GID) {
		/* LINTED */
		if (info->f_gid >= 0)
			gen_unumber("gid", (Ullong)info->f_gid);
		else
			gen_number("gid", (Llong)info->f_gid);
	}

	if (xflags & XF_UNAME) {
		ic_nameuid(name, sizeof (name)-1, info->f_uid);
		gen_text("uname", name, -1, T_UTF8);
	}
	if (xflags & XF_GNAME) {
		ic_namegid(name, sizeof (name)-1, info->f_gid);
		gen_text("gname", name, -1, T_UTF8);
	}

	if (xflags & XF_PATH) {
		gen_text("path", info->f_name, -1,
			(info->f_flags & F_ADDSLASH) != 0 ?
				(T_ADDSLASH|T_UTF8) : T_UTF8);
	}

	if (xflags & XF_LINKPATH && info->f_lnamelen)
		gen_text("linkpath", info->f_lname, -1, T_UTF8);

	if (xflags & XF_SIZE)
		gen_unumber("size", (Ullong)info->f_rsize);

	/*
	 * If "SCHILY.realsize" is needed, it must be past any "size" keyword
	 * in case a "size" keyword is also present.
	 */
	if (xflags & XF_REALSIZE)
		gen_unumber("SCHILY.realsize", (Ullong)info->f_size);
	if (xflags & XF_OFFSET)
		gen_unumber("SCHILY.offset", (Ullong)info->f_contoffset);

	if (H_TYPE(hdrtype) == H_SUNTAR) {
		if (xflags & XF_DEVMAJOR) {
			/* LINTED */
			if (info->f_rdevmaj >= 0)
				gen_unumber("SUN.devmajor", (Ullong)info->f_rdevmaj);
			else
				gen_number("SUN.devmajor", (Llong)info->f_rdevmaj);
		}
		if (xflags & XF_DEVMINOR) {
			/* LINTED */
			if (info->f_rdevmin >= 0)
				gen_unumber("SUN.devminor", (Ullong)info->f_rdevmin);
			else
				gen_number("SUN.devminor", (Llong)info->f_rdevmin);
		}
	} else {
		if (xflags & XF_DEVMAJOR) {
			/* LINTED */
			if (info->f_rdevmaj >= 0)
				gen_unumber("SCHILY.devmajor", (Ullong)info->f_rdevmaj);
			else
				gen_number("SCHILY.devmajor", (Llong)info->f_rdevmaj);
		}
		if (xflags & XF_DEVMINOR) {
			/* LINTED */
			if (info->f_rdevmin >= 0)
				gen_unumber("SCHILY.devminor", (Ullong)info->f_rdevmin);
			else
				gen_number("SCHILY.devminor", (Llong)info->f_rdevmin);
		}
	}

#ifdef	USE_ACL
	/*
	 * POSIX draft Access Control Lists, currently supported e.g. by Linux.
	 * Solaris ACLs have been converted to POSIX draft ACLs before.
	 */
#ifdef	__later__
	if (xflags & (XF_ACL_ACCESS|XF_ACL_DEFAULT)) {
		gen_text("SCHILY.acl.type", "POSIX draft", 11, 0);
	}
	if (xflags & XF_ACL_ACE) {
		gen_text("SCHILY.acl.type", "NFSv4", 5, 0);
	}
#endif
	if (xflags & XF_ACL_ACE) {
		gen_text("SCHILY.acl.ace", info->f_acl_ace, -1, T_UTF8);
	}

	if (xflags & XF_ACL_ACCESS) {
		gen_text("SCHILY.acl.access", info->f_acl_access, -1, T_UTF8);
	}

	if (xflags & XF_ACL_DEFAULT) {
		gen_text("SCHILY.acl.default", info->f_acl_default, -1, T_UTF8);
	}
#endif  /* USE_ACL */

#ifdef USE_XATTR
	if ((xflags & XF_XATTR) && info->f_xattr) {
		star_xattr_t	*x;

		for (x = info->f_xattr; x->name; x++) {
			char aname[PATH_MAX];
			js_snprintf(aname, PATH_MAX, "SCHILY.xattr.%s", x->name);
			gen_text(aname, x->value, x->value_len, 0);
		}
	}
#endif  /* USE_XATTR */

#ifdef	USE_FFLAGS
	if (xflags & XF_FFLAGS) {
extern char	*textfromflags	__PR((FINFO *, char *));

		char	fbuf[512];
		gen_text("SCHILY.fflags", textfromflags(info, fbuf), -1, 0);
	}
#endif

	if (dodump) {
		/* LINTED */
		if (info->f_dev >= 0)
			gen_unumber("SCHILY.dev", (Ullong)info->f_dev);
		else
			gen_number("SCHILY.dev", (Llong)info->f_dev);
		gen_unumber("SCHILY.ino", (Ullong)info->f_ino);
		gen_unumber("SCHILY.nlink", (Ullong)info->f_nlink);
		gen_text("SCHILY.filetype", XTTONAME(info->f_rxftype), -1, 0);
#ifdef	__needed__
		if (info->f_rxftype != info->f_xftype)
			gen_text("SCHILY.tarfiletype", XTTONAME(info->f_xftype), -1, 0);
#endif
		if (is_dir(info)) {
			int	oidx = xbidx;

			if (info->f_dir)
				gen_text("SCHILY.dir",
					info->f_dir, info->f_dirlen, T_UTF8);
			/*
			 * Estimate same length for inode array and dir content
			 * Add 1 to (xbidx - oidx) as "SCHILY.dino" is one
			 * longer than "SCHILY.dir".
			 */
			if (info->f_dirinos)
				gen_iarray("SCHILY.dino",
					info->f_dirinos, info->f_dirents,
					xbidx - oidx + 1);
		}
	} else if (is_multivol(info)) {
		gen_text("SCHILY.filetype", XTTONAME(info->f_rxftype), -1, 0);
	}

	write_xhdr(props.pr_xc);
}

LOCAL void
check_xtime(keyword, info)
	register char	*keyword;
	register FINFO	*info;
{
	long	l = 0;	/* Make GCC happy */

	if (*keyword == 'a')
		l = info->f_ansec;
	else if (*keyword == 'c')
		l = info->f_cnsec;
	else if (*keyword == 'm')
		l = info->f_mnsec;

	if (l >= 0 && l < 1000000000)
		return;

	/*
	 * check_xtime() is used in write mode so info->f_name is valid.
	 */
	errmsgno(EX_BAD, "Bad '%s' nsec value %ld for '%s' at %lld.\n",
		keyword, l, info->f_name, tblocks());
	l = 0;

	if (*keyword == 'a')
		info->f_ansec = l;
	else if (*keyword == 'c')
		info->f_cnsec = l;
	else if (*keyword == 'm')
		info->f_mnsec = l;
}

/*
 * Create a time string with sub-second granularity.
 *
 * <length> <time-name-spec>=<seconds>.<nano-seconds>\n
 *
 *	00 atime=<seconds>.123456789\n
 *	00 ctime=<seconds>.123456789\n
 *	00 mtime=<seconds>.123456789\n
 *
 *	00 mtime=.123456789\n
 *	.123456789.123456789
 *
 * As we always emmit 9 digits for the sub-second part, the
 * length of this entry is always more than 20 and less than 100.
 * The size of an entry is 20 + the size of the "seconds" part of the entry.
 * With 64 bit unsigned numbers, the "seconds" part will be 20 char at max.
 * We may safely fill in the two digit <length> later, when we know the value.
 *
 * The code is highly optimised because in PAX (extended TAR) mode we are
 * spending a significant amount of user CPU time in it.
 */
EXPORT void
gen_xtime(keyword, sec, nsec)
		char	*keyword;
	register time_t	sec;
	register Ulong	nsec;
{
		char	nb[32];
	register char	*p;
	register char	*np;
	register int	len;

	if (nsec >= 1000000000)	/* We would create an unusable string */
		nsec = 0;

	if ((xbidx + 100) > xblen)
		xbgrow(100);

	/*
	 * The following code is equivalent to:
	 *
	 * sprintf(p, "%d %s=%lld.%9.9ld\n", length, keyword, (Llong)sec, nsec)
	 *
	 * But is twice as fast.
	 */
	len = 20;		/* Base length, second field must be added  */
	p = &xbuf[xbidx+2];	/* point past length field		    */
	/*
	 * Fill in " ?time="
	 */
	*p++ = ' ';
	if (keyword[0] == 'a' || keyword[0] == 'c' || keyword[0] == 'm') {
		*p++ = keyword[0];
		*p++ = 't'; *p++ = 'i'; *p++ = 'm'; *p++ = 'e';
	} else {
		len = 15;	/* 2digitlen + ' ' + '=' + '.' + nsec + '\n' */
		np = keyword;
		while ((*p++ = *np++) != '\0')
			len++;
		--p;
	}
	*p++ = '=';
	if (sec < 0) {		/* Time is before 01.01.1970 ...	    */
		*p++ = '-';
		sec = -sec;
		len += 1;
	}

	np = &nb[sizeof (nb)-1]; /* Point to end of number string buffer    */
	*np = '\0';		/* Null terminate			    */
	ui2decimal(sec, np);	/* Convert to unsigned decimal string	    */
	len += &nb[sizeof (nb)-1] - np;

	scopy(p, np);		/* Copy number string buffer to xheader	    */
	--p,			/* Overshoot compensation		    */
	*p++ = '.';		/* Decimal point between secs and nsecs	    */

	np = &p[10];		/* Point to end of <nsec> string part	    */
	*np = '\0';		/* Null terminate			    */
	*--np = '\n';		/* Newline at end of line		    */
	ui2decimal(nsec, np);	/* Convert to unsigned decimal string	    */
	while (np > p)
		*--np = '0';	/* Left fill with '0' to 9 digits	    */

	np = &xbuf[xbidx+2];	/* Point to end of length string part	    */
	xbidx += len;		/* 'len' is trashed in ui2decimal()	    */
	ui2decimal(len, np);	/* Convert to unsigned decimal string	    */
}

/*
 * Create a generic unsigned number string.
 *
 * <length> <name-spec>=<value>\n
 *
 * The length of this entry is always less than 100 chars if the length of the
 * 'name-spec' is less than 75 chars (the maximum length of a 64 bit number in
 * decimal is 20 digits). Even in case of a 128 bit number length will be less
 * than 100 chars if the length of 'name-spec' is less than 55 chars.
 *
 * The code is highly optimised because in dump mode when using extended TAR
 * headers with additional fields, we are spending a significant amount of
 * user CPU time in it.
 */
EXPORT void
gen_unumber(keyword, arg)
	register char	*keyword;
	register Ullong	arg;
{
		char	nb[64];	/* 41 is enough for unsigned 128 bit ints    */
	register char	*p;
	register char	*np;
	register int	len;
	register int	i;

	if ((xbidx + 100) > xblen)
		xbgrow(100);

	/*
	 * The following code is equivalent to:
	 *
	 * sprintf(p, "%d %s=%llu\n", length, keyword, arg);
	 *
	 * But is twice as fast.
	 */
	np = &nb[sizeof (nb)-1]; /* Point to end of number string buffer    */
	*np = '\0';		/* Null terminate			    */
	ui2decimal(arg, np);	/* Convert to unsigned decimal string	    */

	len = strlen(keyword);	/* Compute the length, start with strlen(kw) */

	len += &nb[sizeof (nb)-1] - np;	/* Add strlen(number)		    */
	len += 2 + 3;		/* Add 2 for strlen(len) and 3 for " =\n"   */

	if (len < 10) {		/* If < 10, the len field is 1 digit only   */
		len -= 1;	/* This happens when strlen(keyword) is < 4 */
		i = 1;		/* and number is one digit only.	    */
	} else {
		i = 2;
	}
	p = &xbuf[xbidx+i];	/* Point past length field		    */
	*p++ = ' ';		/* Fill in space after length field	    */
	scopy(p, keyword);	/* Copy keyword into to xheader		    */
	--p,			/* Overshoot compensation		    */
	*p++ = '=';		/* Fill in '=' after keyword field	    */

	scopy(p, np);		/* Copy number string buffer to xheader	    */
	*--p = '\n';		/* Newline at end of line		    */

	np = &xbuf[xbidx+i];	/* Point to end of length string part	    */
	xbidx += len;		/* 'len' is trashed in ui2decimal()	    */
	ui2decimal(len, np);	/* Convert to unsigned decimal string	    */
}

/*
 * Create a generic signed number string.
 *
 * <length> <name-spec>=<value>\n
 *
 * The length of this entry is always less than 100 chars if the length of the
 * 'name-spec' is less than 75 chars (the maximum length of a 64 bit number in
 * decimal is 20 digits). Even in case of a 128 bit number length will be less
 * than 100 chars if the length of 'name-spec' is less than 55 chars.
 *
 * The code is highly optimised because in dump mode when using extended TAR
 * headers with additional fields, we are spending a significant amount of
 * user CPU time in it.
 */
EXPORT void
gen_number(keyword, arg)
	register char	*keyword;
	register Llong	arg;
{
		char	nb[64];	/* 41 is enough for unsigned 128 bit ints    */
	register char	*p;
	register char	*np;
	register int	len;
	register int	i;
		BOOL	neg = FALSE;

	if ((xbidx + 100) > xblen)
		xbgrow(100);

	/*
	 * The following code is equivalent to:
	 *
	 * sprintf(p, "%d %s=%lld\n", length, keyword, arg);
	 *
	 * But is twice as fast.
	 */
	np = &nb[sizeof (nb)-1]; /* Point to end of number string buffer    */
	*np = '\0';		/* Null terminate			    */
	if (arg < 0) {
		arg = -arg;
		neg = TRUE;
	}
	ui2decimal(arg, np);	/* Convert to unsigned decimal string	    */
	if (neg)
		*--np = '-';

	len = strlen(keyword);	/* Compute the length, start with strlen(kw) */

	len += &nb[sizeof (nb)-1] - np;	/* Add strlen(number)		    */
	len += 2 + 3;		/* Add 2 for strlen(len) and 3 for " =\n"   */

	if (len < 10) {		/* If < 10, the len field is 1 digit only   */
		len -= 1;	/* This happens when strlen(keyword) is < 4 */
		i = 1;		/* and number is one digit only.	    */
	} else {
		i = 2;
	}
	p = &xbuf[xbidx+i];	/* Point past length field		    */
	*p++ = ' ';		/* Fill in space after length field	    */
	scopy(p, keyword);	/* Copy keyword into to xheader		    */
	--p,			/* Overshoot compensation		    */
	*p++ = '=';		/* Fill in '=' after keyword field	    */

	scopy(p, np);		/* Copy number string buffer to xheader	    */
	*--p = '\n';		/* Newline at end of line		    */

	np = &xbuf[xbidx+i];	/* Point to end of length string part	    */
	xbidx += len;		/* 'len' is trashed in ui2decimal()	    */
	ui2decimal(len, np);	/* Convert to unsigned decimal string	    */
}

/*
 * Create a string from an array of unsigned inode numbers.
 *
 * <length> <keyword>=<values>\n
 *
 * <values> is a space separated list of unsigned integers in decimal ascii.
 *
 * The len parameter is the estimated length of the whole string. It is used
 * to pre-allocate a buffer that hopefully has the right size already in order
 * to avoid copying the content.
 *
 * The code is highly optimised because in dump mode when using extended TAR
 * headers with additional fields, we are spending a significant amount of
 * user CPU time in it.
 */
LOCAL void
gen_iarray(keyword, arg, ents, len)
	register char	*keyword;
		ino_t	*arg;
		int	ents;
	register int	len;	/* Estimated length */
{
		char	nb[64];	/* 41 is enough for unsigned 128 bit ints    */
	register Ullong	ll;
	register char	*p;
	register char	*np;
	register int	i;
	register int	llen;
	register int	olen;

	/*
	 * The following code is equivalent to:
	 *
	 * sprintf(p, "%d %s=%llu %llu ...\n", length, keyword, arg[...]...);
	 *
	 * But avoids copying if possible.
	 */
	i = len;
	if (len <= 0)			/* No estimated length already	    */
		len = strlen(keyword) + 3; /* + ' ' + '=' + '\n'	    */
	olen = len;
	len += llen = len_len(len);	/* add length of length string	    */

	if (i <= 0)
		i = len + ents * 2;	/* Make a minimal guess on the len  */
	else
		i = len;		/* Use guessed value from parameter */

	if ((xbidx + i) > xblen)
		xbgrow(i);

	p = &xbuf[xbidx+llen];	/* Point past length field		    */
	*p++ = ' ';		/* Fill in space after length field	    */
	scopy(p, keyword);	/* Copy keyword into to xheader		    */
	--p,			/* Overshoot compensation		    */
	*p++ = '=';		/* Fill in '=' after keyword field	    */

	len = p - &xbuf[xbidx];	/* strlen(keyword) + ' ' + '='		    */
	for (i = 0; i < ents; i++) {
		if (((p - xbuf) + 100) > xblen) {
			register int	xb_idx;
			/*
			 * The address for xbuf may change in case that
			 * realloc() cannot grow the current memory chunk,
			 * recalculate 'p' in this case.
			 */
			xb_idx = p - xbuf;
			xbgrow(100);
			p = xbuf + xb_idx;
		}
		ll = (Llong)arg[i];
		np = &nb[sizeof (nb)-1]; /* Point to end of number str. buf */
		*np = '\0';		/* Null terminate		    */
		ui2decimal(ll, np);	/* Convert to unsigned decimal str. */
		len += &nb[sizeof (nb)-1] - np;	/* strlen(number)	    */
		len += 1;		/* Space (' ') or Newline ('\n')    */

		scopy(p, np);		/* Copy number str. buf to xheader  */
		p[-1] = ' ';		/* Space at end of number	    */
	}
	*--p = '\n';			/* Overwrite last space by newline  */
	i = p - &xbuf[xbidx] + 1; /* Total string length		    */
	i -= llen;		/* Subtract old length length value	    */

	if (olen != i) {	/* Length without length string changed	    */
		olen = llen;	/* Save old length string length	    */
		llen = len_len(i);	/* New length of length string	    */
		if (olen != llen) {	/* We need to move the whole text   */
			p = &xbuf[xbidx+olen];
			olen = llen - olen;
			movebytes(p, p+olen, i);
		}
		len = i + llen;
	}

	np = &xbuf[xbidx+llen];	/* Point to end of length string part	    */
	*np = ' ';		/* May have been overwritten by movebytes() */
	xbidx += len;		/* 'len' is trashed in ui2decimal()	    */
	ui2decimal(len, np);	/* Convert to unsigned decimal string	    */
}

/*
 * Create a generic text string in UTF-8 coding.
 *
 * <length> <name-spec>=<value>\n
 *
 * This function will have to carefully check for the resultant length
 * and thus compute the total length in advance. If the rare case that the
 * UTF-8 conversion changes the length so much that the length of the length
 * string will be different from the estimated value, we need to move whole
 * text by one.
 *
 * The code is highly optimised because in dump mode when using extended TAR
 * headers with additional fields, we may need to copy directory listings
 * that are longer then 100000 bytes.
 */
EXPORT void
gen_text(keyword, arg, alen, flags)
	register char	*keyword;
		char	*arg;
		int	alen;
		Uint	flags;
{
	register char	*p;
	register char	*np;
	register int	len;
	register int	i;
	register int	llen;
	register int	olen;

	/*
	 * The following code is equivalent to:
	 *
	 * sprintf(p, "%d %s=%s\n", length, keyword, arg);
	 *
	 * But avoids copying if possible.
	 */
	if ((len = alen) == -1)
		len = strlen(arg);
	alen = len;
	if (flags & T_ADDSLASH)		/* only used if 'path' is a dir	    */
		len++;

	len += strlen(keyword) + 3;	/* + ' ' + '=' + '\n'		    */
	olen = len;
	len += llen = len_len(len);	/* add length of length string	    */

	i = len;
	if (flags & T_UTF8)
		i *= 6;			/* UTF-8 Factor may be up to 6!	    */
	if ((xbidx + i) > xblen)
		xbgrow(i);


	p = &xbuf[xbidx+llen];	/* Point past length field		    */
	*p++ = ' ';		/* Fill in space after length field	    */
	scopy(p, keyword);	/* Copy keyword into to xheader		    */
	--p,			/* Overshoot compensation		    */
	*p++ = '=';		/* Fill in '=' after keyword field	    */

	if (flags & T_UTF8 && !binflag) {
		i = to_utf8((Uchar *)p, i, (Uchar *)arg, alen);
		p += i + 1;
	} else {
		p = movebytes(arg, p, alen);
		p++;
	}

	if (flags & T_ADDSLASH) { /* only used if 'path' is a dir	    */
		i++;		/* String length increases by '/'	    */
		*--p = '/';	/* Slash at end of string		    */
		*++p = '\n';	/* Newline at end of line		    */
	} else {
		*--p = '\n';	/* Newline at end of line		    */
	}
	i = p - &xbuf[xbidx] + 1; /* Total string length		    */
	i -= llen;		/* Subtract old length length value	    */

	if (olen != i) {	/* Length without length string changed	    */
		olen = llen;	/* Save old length string length	    */
		llen = len_len(i);	/* New length of length string	    */
		if (olen != llen) {	/* We need to move the whole text   */
			p = &xbuf[xbidx+olen];
			olen = llen - olen;
			movebytes(p, p+olen, i);
		}
		len = i + llen;
	}

	np = &xbuf[xbidx+llen];	/* Point to end of length string part	    */
	*np = ' ';		/* May have been overwritten by movebytes() */
	xbidx += len;		/* 'len' is trashed in ui2decimal()	    */
	ui2decimal(len, np);	/* Convert to unsigned decimal string	    */
}

/*
 * It bad to see new global variables while we are working on a star library.
 */
LOCAL	star_xattr_t	*static_xattr;

/*
 * The xattr list grows dynamically. Reset it before reading
 * the next Extended Header.
 */
EXPORT void
tcb_to_xhdr_reset()
{
	/*
	 * XXX Dies soll laut A Grünbacher in tcb_to_info() aufgerufen werden
	 */
	free_xattr(&static_xattr);
}

LOCAL int
len_len(len)
	register int	len;
{
	if (len <= 8)
		return (1);
	if (len <= 97)
		return (2);
	if (len <= 996)
		return (3);
	if (len <= 9995)
		return (4);
	if (len <= 99994)
		return (5);
	if (len <= 999993)
		return (6);
	if (len <= 9999992)
		return (7);
	if (len <= 99999991)
		return (8);
	if (len <= 999999990)
		return (9);

	return (10);
}

/*
 * Lookup command in command tab
 */
LOCAL xtab_t *
lookup(cmd, clen, cp)
	register char	*cmd;
	register int	clen;
	register xtab_t	*cp;
{
	for (; cp->x_name; cp++) {
		register int	len = cp->x_namelen-1;

		if (cp->x_name[len] == '*') {
			if (strncmp(cmd, cp->x_name, len) == 0)
				return (cp);
		}
		if (clen != cp->x_namelen)
			continue;

		if ((*cmd == *cp->x_name) &&
		    strcmp(cmd, cp->x_name) == 0) {
			return (cp);
		}
	}
	return ((xtab_t *)NULL);
}

/*
 * Read extended POSIX.1-2001 header and parse the content.
 */
EXPORT int
tcb_to_xhdr(ptb, info)
	register TCB	*ptb;
	register FINFO	*info;
{
	register char	*p;
	register char 	*ep;
		FINFO	*oinfo = info;
		move_t	move;
		Ullong	ull;

#ifdef	XH_DEBUG
error("Block: %lld\n", tblocks());
#endif
	if (ptb->dbuf.t_linkflag == LF_GHDR) {
		grinit();
		info = &ginfo;
	}
	/*
	 * File size is strlen of extended header
	 */
	stolli(ptb->dbuf.t_size, &ull);
	info->f_size = ull;
	info->f_rsize = (off_t)info->f_size;
	/*
	 * Reset xbidx to make xbgrow() work correctly for our case.
	 */
	xbidx = 0;
	if ((info->f_size+1) > xblen)
		xbgrow(info->f_size+1);

	/*
	 * move_from_arch() always adds null byte to make decoding easier.
	 */
	move.m_data = xbuf;
	move.m_flags = 0;
	if (xt_file(info, vp_move_from_arch, &move, 0,
						"moving extended header") < 0) {
		die(EX_BAD);
	}

#ifdef	XH_DEBUG
error("Block: %lld\n", tblocks());
error("xbuf: '%s'\n", xbuf);
#endif

	p = xbuf;
	ep = p + ull;
	if (!no_xheader)
		xhparse(info, p, ep);

	if (ptb->dbuf.t_linkflag == LF_GHDR) {
		griprint(grip);
		gipsetup(grip);
		info_xcopy(oinfo, info);
	}
	return (get_tcb(ptb));
}

EXPORT BOOL
xhparse(info, p, ep)
	register FINFO	*info;
	register char	*p;
	register char	*ep;
{
	register xtab_t	*cp;
	register char	*keyword;
	register char 	*arg;
		long	length;

	while (p < ep) {
		register int klen;

		if (*p < '0' || *p > '9') {
			errmsgno(EX_BAD,
			"Syntax error in extended header: non digit in len at %lld.\n",
			tblocks());
			return (FALSE);
		}
		keyword = astolb(p, &length, 10);
		if (*keyword != ' ') {
			errmsgno(EX_BAD,
			"Syntax error in extended header: missing ' ' at %lld.\n",
			tblocks());
			return (FALSE);
		}
		keyword++;
		arg = strchr(keyword, '=');
		klen = arg - keyword;
		if ((arg == NULL) || (klen > length)) {
			errmsgno(EX_BAD,
			"Syntax error in extended header: missing '=' at %lld.\n",
			tblocks());
			return (FALSE);
		}
		*arg++ = '\0';			/* Kill equal sign */

		if (*(p + length -1) != '\n') {
			arg[-1] = '=';
			errmsgno(EX_BAD,
			"Syntax error in extended header: missing '\\n' at %lld.\n",
			tblocks());
			return (FALSE);
		}
		*(p + length -1) = '\0';	/* Kill new-line character */

		if ((cp = lookup(keyword, klen, xtab)) != NULL) {
			(*cp->x_func)(info, keyword, klen,
						arg, length - (arg-p) - 1);
		} else {
			print_unknown(keyword);
		}
		arg[-1] = '=';
		p += length;
		p[-1] = '\n';
	}
	return (TRUE);
}

LOCAL void
print_unknown(keyword)
	register char	*keyword;
{
	register unkn_t	*up = unkn;

	while (up) {
		if (streql(keyword, up->u_name))
			break;
		up = up->u_next;
	}
	if (up == NULL) {
		errmsgno(EX_BAD,
			"Unknown extended header keyword '%s' ignored at %lld.\n",
				keyword, tblocks());

		up = ___malloc(sizeof (*up) + strlen(keyword), "unknown list");
		strcpy(up->u_name, keyword);
		up->u_next = unkn;
		unkn = up;
	}
}

EXPORT void
xh_rangeerr(keyword, arg, len)
	char	*keyword;
	char	*arg;
	int	len;
{
	if (nowarn)
		return;
	errmsgno(EX_BAD,
		"WARNING: %s '%.*s' in extended header at %lld exceeds local range.\n",
		keyword, len, arg, tblocks());
}

LOCAL void
print_toolong(keyword, arg, len)
	char	*keyword;
	char	*arg;
	int	len;
{
	if (nowarn)
		return;
	errmsgno(EX_BAD,
		"WARNING: %s '%.*s' in extended header at %lld too long, ignoring.\n",
		keyword, len, arg, tblocks());
}

LOCAL void
get_xvolhdr(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	register xtab_t	*cp;
extern		xtab_t	volhtab[];

	if ((cp = lookup(keyword, klen, volhtab)) != NULL) {
		(*cp->x_func)(info, keyword, klen,
					arg, len);
	} else {
		print_unknown(keyword);
	}
}

LOCAL void
info_xcopy(ninfo, oinfo)
	register FINFO	*ninfo;
	register FINFO	*oinfo;
{
	if (oinfo->f_xflags & XF_ATIME) {
		ninfo->f_atime = oinfo->f_atime;
		ninfo->f_ansec = oinfo->f_ansec;
		ninfo->f_xflags |= XF_ATIME;
	}
	if (oinfo->f_xflags & XF_CTIME) {
		ninfo->f_ctime = oinfo->f_ctime;
		ninfo->f_cnsec = oinfo->f_cnsec;
		ninfo->f_xflags |= XF_CTIME;
	}
	if (oinfo->f_xflags & XF_MTIME) {
		ninfo->f_mtime = oinfo->f_mtime;
		ninfo->f_mnsec = oinfo->f_mnsec;
		ninfo->f_xflags |= XF_MTIME;
	}
	/* We ignore XF_COMMENT */
	if (oinfo->f_xflags & XF_UID) {
		ninfo->f_uid = oinfo->f_uid;
		ninfo->f_xflags |= XF_UID;
	}
	if (oinfo->f_xflags & XF_UNAME) {
		strcpy(guname, oinfo->f_uname);
		ninfo->f_uname = oinfo->f_uname;
		ninfo->f_umaxlen = ninfo->f_umaxlen;
		oinfo->f_uname = guname;
		ninfo->f_xflags |= XF_UNAME;
	}
	if (oinfo->f_xflags & XF_GID) {
		ninfo->f_gid = oinfo->f_gid;
		ninfo->f_xflags |= XF_GID;
	}
	if (oinfo->f_xflags & XF_GNAME) {
		strcpy(ggname, oinfo->f_gname);
		ninfo->f_gname = oinfo->f_gname;
		ninfo->f_gmaxlen = ninfo->f_gmaxlen;
		oinfo->f_gname = ggname;
		ninfo->f_xflags |= XF_GNAME;
	}
	if (oinfo->f_xflags & XF_PATH) {
		strcpy(ninfo->f_name, oinfo->f_name);
		ninfo->f_xflags |= XF_PATH;
	}
	if (oinfo->f_xflags & XF_LINKPATH) {
		strcpy(ninfo->f_lname, oinfo->f_lname);
		ninfo->f_xflags |= XF_LINKPATH;
	}

	if (oinfo->f_xflags & XF_SIZE) {
		ninfo->f_rsize = oinfo->f_rsize;
		ninfo->f_xflags |= XF_SIZE;
	}
	/* XF_CHARSET currently is a dummy */

	if (oinfo->f_xflags & XF_DEVMAJOR) {
		ninfo->f_rdevmaj = oinfo->f_rdevmaj;
		ninfo->f_xflags |= XF_DEVMAJOR;
	}
	if (oinfo->f_xflags & XF_DEVMINOR) {
		ninfo->f_rdevmin = oinfo->f_rdevmin;
		ninfo->f_xflags |= XF_DEVMINOR;
	}
	if (oinfo->f_xflags & (XF_ACL_ACCESS|XF_ACL_DEFAULT)) {
		errmsgno(EX_BAD, "WARNING: Found global ACL data at %lld.\n",
			tblocks());
	}
	if (oinfo->f_xflags & XF_FFLAGS) {
		ninfo->f_fflags = oinfo->f_fflags;
		ninfo->f_xflags |= XF_FFLAGS;
	}
	if (oinfo->f_xflags & XF_REALSIZE) {
		ninfo->f_size = oinfo->f_size;
		ninfo->f_xflags |= XF_REALSIZE;
	}
	if (oinfo->f_xflags & XF_OFFSET) {
		ninfo->f_contoffset = oinfo->f_contoffset;
		ninfo->f_xflags |= XF_OFFSET;
	}
	if (oinfo->f_xflags & XF_XATTR) {
		errmsgno(EX_BAD, "WARNING: Found global XATTR data at %lld.\n",
			tblocks());
	}
}

/*
 * generic function to read args that hold times
 *
 * The time may either be in second resolution or in sub-second resolution.
 * In the latter case the second fraction and the sub second fraction
 * is separated by a dot ('.').
 */
EXPORT BOOL
get_xtime(keyword, arg, len, secp, nsecp)
	char	*keyword;
	char	*arg;
	int	len;
	time_t	*secp;
	long	*nsecp;
{
#ifdef	__use_default_time__
extern struct	timespec	ddate;
#endif
	Llong	ll;
	long	l;
	int	flen;
	char	*p;

	p = astollb(arg, &ll, 10);
	if (*p == '\0' || *p == '.') {
		time_t	secs = ll;
		if (secs != ll) {
			xh_rangeerr(keyword, arg, len);
#ifdef	__use_default_time__
			*secp = ddate.tv_sec;
#endif
			goto bad;
		}
		*secp = ll;		/* XXX Check for NULL ptr? */
	}
	if (*p == '\0') {		/* time has second resolution only */
		if (nsecp == NULL)
			return (TRUE);
		*nsecp = 0;
		return (TRUE);
	} else if (*p == '.') {		/* time has sub-second resolution */
		flen = strlen(++p);	/* remember resolution		    */
		if (flen > 9)		/* if resolution is better than	    */
			p[9] = '\0';	/* nanoseconds kill rest of line as */
		p = astolb(p, &l, 10);	/* we don't understand more.	    */
		if (*p == '\0') {	/* number read correctly	    */
			if (l >= 0) {
				while (flen < 9) {	/* convert to nsecs */
					l *= 10;
					flen++;
				}
				if (nsecp == NULL)
					return (TRUE);
				*nsecp = l;
				return (TRUE);
			}
		}
	}
bad:
	errmsgno(EX_BAD, "Bad timespec '%s' for '%s' in extended header at %lld.\n",
		arg, keyword, tblocks());
	return (FALSE);
}

/*
 * get read access time from extended header
 */
/* ARGSUSED */
LOCAL void
get_atime(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		info->f_xflags &= ~XF_ATIME;
		return;
	}
	if (get_xtime(keyword, arg, len, &info->f_atime, &info->f_ansec))
		info->f_xflags |= XF_ATIME;
}

/*
 * get inode status change time from extended header
 */
/* ARGSUSED */
LOCAL void
get_ctime(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		info->f_xflags &= ~XF_CTIME;
		return;
	}
	if (get_xtime(keyword, arg, len, &info->f_ctime, &info->f_cnsec))
		info->f_xflags |= XF_CTIME;
}

/*
 * get modification time from extended header
 */
/* ARGSUSED */
LOCAL void
get_mtime(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		info->f_xflags &= ~XF_MTIME;
		return;
	}
	if (get_xtime(keyword, arg, len, &info->f_mtime, &info->f_mnsec))
		info->f_xflags |= XF_MTIME;
}

/*
 * generic function to read args that hold decimal numbers
 */
#ifdef	__needed_
EXPORT BOOL
get_number(keyword, arg, llp)
	char	*keyword;
	char	*arg;
	Llong	*llp;
{
	Llong	ll;
	char	*p;

	p = astollb(arg, &ll, 10);
	if (*p == '\0') {		/* number read correctly */
		*llp = ll;		/* XXX Check for NULL ptr? */
		return (TRUE);
	}
	errmsgno(EX_BAD, "Bad number '%s' for '%s' in extended header at %lld.\n",
		arg, keyword, tblocks());
	return (FALSE);
}
#endif

/*
 * generic function to read args that hold unsigned decimal numbers
 */
LOCAL BOOL
get_xnumber(keyword, arg, ullp, type)
	char	*keyword;
	char	*arg;
	Ullong	*ullp;
	char	*type;
{
	Ullong	ull;
	char	*p;

	seterrno(0);
	p = astoullb(arg, &ull, 10);
	if (*p == '\0') {		/* number read correctly */
		if (geterrno() != 0) {
			errmsgno(EX_BAD,
			"Number overflow with '%s' for '%s' in extended header at %lld.\n",
			arg, keyword, tblocks());
			return (FALSE);
		}
		*ullp = ull;		/* XXX Check for NULL ptr? */
		return (TRUE);
	}
	errmsgno(EX_BAD, "Bad %s number '%s' for '%s' in extended header at %lld.\n",
		type,
		arg, keyword, tblocks());
	return (FALSE);
}

EXPORT BOOL
get_unumber(keyword, arg, ullp, maxval)
	char	*keyword;
	char	*arg;
	Ullong	*ullp;
	Ullong	maxval;
{
	if (!get_xnumber(keyword, arg, ullp, "unsigned"))
		return (FALSE);

	if (*ullp > maxval) {
		errmsgno(EX_BAD,
		"Value '%s' is out of range 0..%llu for '%s' in extended header at %lld.\n",
		arg, maxval, keyword, tblocks());
		return (FALSE);
	}
	return (TRUE);
}


/*
 * generic function to read args that hold signed decimal numbers
 */
EXPORT BOOL
get_snumber(keyword, arg, ullp, negp, minval, maxval)
	char	*keyword;
	char	*arg;
	Ullong	*ullp;
	BOOL	*negp;
	Ullong	minval;
	Ullong	maxval;
{
	char	*p = arg;
#define	is_space(c)	 ((c) == ' ' || (c) == '\t')

	while (is_space(*p))
		p++;

	*negp = FALSE;
	if (*p == '-') {
		p++;
		*negp = TRUE;
	}
	return (get_xnumber(keyword, p, ullp, "signed"));
}

/*
 * get user id (if > 2097151)
 * POSIX requires uid_t to be a signed int but the values for uid_t to be
 * non-negative.
 * We allow signed ints and carefully check the values.
 */
/* ARGSUSED */
LOCAL void
get_uid(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;
	BOOL	neg = FALSE;

	if (len == 0) {
		info->f_xflags &= ~XF_UID;
		return;
	}
	if (get_snumber(keyword, arg, &ull, &neg,
					-(Ullong)UID_T_MIN, UID_T_MAX)) {
		info->f_xflags |= XF_UID;
		if (neg)
			info->f_uid = -ull;
		else
			info->f_uid = ull;

		if ((neg && -info->f_uid != ull) ||
			(!neg && info->f_uid != ull)) {

			xh_rangeerr(keyword, arg, len);
			info->f_flags |= F_BAD_UID;
			info->f_uid = ic_uid_nobody();
		}
	}
}

/*
 * get group id (if > 2097151)
 * POSIX requires gid_t to be a signed int but the values for gid_t to be
 * non-negative.
 * We allow signed ints and carefully check the values.
 */
/* ARGSUSED */
LOCAL void
get_gid(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;
	BOOL	neg = FALSE;

	if (len == 0) {
		info->f_xflags &= ~XF_GID;
		return;
	}
	if (get_snumber(keyword, arg, &ull, &neg,
					-(Ullong)GID_T_MIN, GID_T_MAX)) {
		info->f_xflags |= XF_UID;
		if (neg)
			info->f_gid = -ull;
		else
			info->f_gid = ull;

		if ((neg && -info->f_gid != ull) ||
			(!neg && info->f_gid != ull)) {

			xh_rangeerr(keyword, arg, len);
			info->f_flags |= F_BAD_GID;
			info->f_gid = ic_gid_nobody();
		}
	}
}

/*
 * Space for returning user/group names.
 * XXX If we ever change to use allocated space, we need to change info_xcopy()
 */
LOCAL	Uchar	_uname[MAX_UNAME+2];
LOCAL	Uchar	_gname[MAX_UNAME+2];

/*
 * get user name (if name length is > 32 chars or if contains non ASCII chars)
 */
/* ARGSUSED */
LOCAL void
get_uname(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		info->f_xflags &= ~XF_UNAME;
		return;
	}
	if (len > MAX_UNAME) {
		print_toolong(keyword, arg, len);
		return;
	}
	if (ginfo.f_xflags & XF_BINARY) {
		strcpy((char *)_uname, arg);
		info->f_xflags |= XF_UNAME;
		info->f_uname = (char *)_uname;
		info->f_umaxlen = len;
	} else if (from_utf8(_uname, sizeof (_uname), (Uchar *)arg, &len)) {
		info->f_xflags |= XF_UNAME;
		info->f_uname = (char *)_uname;
		info->f_umaxlen = len;
	} else {
		bad_utf8(keyword, arg);
	}
}

/*
 * get group name (if name length is > 32 chars or if contains non ASCII chars)
 */
/* ARGSUSED */
LOCAL void
get_gname(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		info->f_xflags &= ~XF_GNAME;
		return;
	}
	if (len > MAX_UNAME) {
		print_toolong(keyword, arg, len);
		return;
	}
	if (ginfo.f_xflags & XF_BINARY) {
		strcpy((char *)_gname, arg);
		info->f_xflags |= XF_GNAME;
		info->f_gname = (char *)_gname;
		info->f_gmaxlen = len;
	} else if (from_utf8(_gname, sizeof (_gname), (Uchar *)arg, &len)) {
		info->f_xflags |= XF_GNAME;
		info->f_gname = (char *)_gname;
		info->f_gmaxlen = len;
	} else {
		bad_utf8(keyword, arg);
	}
}

/*
 * get path (if name length is > 100-255 chars or if contains non ASCII chars)
 */
/* ARGSUSED */
LOCAL void
get_path(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		info->f_xflags &= ~XF_PATH;
		return;
	}

	/*
	 * Check whether we are called via get_xhtype() -> xhparse()
	 */
	if (info->f_name == NULL)
		return;

	if (ginfo.f_xflags & XF_BINARY) {
		if (strlcpy_pspace(PS_STDERR, &info->f_pname, arg, len) < 0) {
			print_toolong(keyword, arg, len);
			info->f_xflags |= F_BAD_META;
			return;
		}
		info->f_name = info->f_pname.ps_path;
		info->f_namelen = len;
		info->f_xflags |= XF_PATH;
	} else {
		int	ilen = len;
		BOOL	ret;

		do {
			len = ilen;
			ret = from_utf8((Uchar *)info->f_name,
					info->f_pname.ps_size,
					(Uchar *)arg, &len);
			if (len >= info->f_pname.ps_size) {
				/*
				 * An increment of 1 is OK, since it is unlikely
				 * that the path grows by more than 256 per dir.
				 */
				if (incr_pspace(PS_STDERR,
						&info->f_pname, 1) < 0) {
					print_toolong(keyword, arg, len);
					info->f_xflags |= F_BAD_META;
					return;
				}
				info->f_name = info->f_pname.ps_path;
			} else
				break;
		} while (1);

		if (ret) {
			info->f_namelen = len;
			info->f_xflags |= XF_PATH;
		} else {
			bad_utf8(keyword, arg);
		}
	}
}

/*
 * get link path (if name length is > 100 chars or if contains non ASCII chars)
 */
/* ARGSUSED */
LOCAL void
get_lpath(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		info->f_xflags &= ~XF_LINKPATH;
		return;
	}

	/*
	 * Check whether we are called via get_xhtype() -> xhparse()
	 */
	if (info->f_lname == NULL)
		return;

	if (ginfo.f_xflags & XF_BINARY) {
		if (strlcpy_pspace(PS_STDERR, &info->f_plname, arg, len) < 0) {
			print_toolong(keyword, arg, len);
			info->f_xflags |= F_BAD_META;
			return;
		}
		info->f_lname = info->f_plname.ps_path;
		info->f_lnamelen = len;
		info->f_xflags |= XF_LINKPATH;
	} else {
		int	ilen = len;
		BOOL	ret;

		do {
			len = ilen;
			ret = from_utf8((Uchar *)info->f_lname,
					info->f_plname.ps_size,
					(Uchar *)arg, &len);
			if (len >= info->f_plname.ps_size) {
				/*
				 * An increment of 1 is OK, since it is unlikely
				 * that the path grows by more than 256 per dir.
				 */
				if (incr_pspace(PS_STDERR,
						&info->f_plname, 1) < 0) {
					print_toolong(keyword, arg, len);
					info->f_xflags |= F_BAD_META;
					return;
				}
				info->f_lname = info->f_plname.ps_path;
			} else
				break;
		} while (1);

		if (ret) {
			info->f_lnamelen = len;
			info->f_xflags |= XF_LINKPATH;
		} else {
			bad_utf8(keyword, arg);
		}
	}
}

/*
 * get size, either real size or size on tape (usually when size is > 8 GB)
 * The file size is doubtlessly an ungined integer
 */
/* ARGSUSED */
LOCAL void
get_size(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;

	if (len == 0) {
		info->f_xflags &= ~XF_SIZE;
		return;
	}
	if (get_unumber(keyword, arg, &ull, OFF_T_MAX)) {
		info->f_xflags |= XF_SIZE;
		info->f_llsize = ull;
		info->f_rsize = (off_t)ull;
		if (info->f_rsize != ull) {
			xh_rangeerr(keyword, arg, len);
			ull = 0;
			info->f_flags |= (F_BAD_META | F_BAD_SIZE);
			info->f_rsize = (off_t)ull;
		}
		/*
		 * If real size is not yet known, copy over the tape size to
		 * avoid problems. If real size is found later, it will
		 * overwrite unconditional.
		 */
		if ((info->f_xflags & XF_REALSIZE) == 0) {
			info->f_xflags |= XF_REALSIZE;
			info->f_size = (off_t)ull;
		}
	}
}

/*
 * get real file size (usually when size is > 8 GB)
 * The real file size is doubtlessly an ungined integer
 */
/* ARGSUSED */
LOCAL void
get_realsize(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;

	if (len == 0) {
		info->f_xflags &= ~XF_REALSIZE;
		return;
	}
	if (get_unumber(keyword, arg, &ull, OFF_T_MAX)) {
		info->f_xflags |= XF_REALSIZE;
		info->f_size = (off_t)ull;
		if (info->f_size != ull) {
			xh_rangeerr(keyword, arg, len);
			info->f_size = (off_t)0;
		}
	}
}

/*
 * get multivolume file offset (usually when size is > 8 GB)
 * The multivolume file offset is doubtlessly an ungined integer
 */
/* ARGSUSED */
LOCAL void
get_offset(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;

	if (len == 0) {
		info->f_xflags &= ~XF_OFFSET;
		return;
	}
	if (get_unumber(keyword, arg, &ull, OFF_T_MAX)) {
		info->f_xflags |= XF_OFFSET;
		info->f_contoffset = (off_t)ull;
		if (info->f_contoffset != ull) {
			xh_rangeerr(keyword, arg, len);
			info->f_contoffset = (off_t)0;
		}
	}
}

/*
 * get major device number (always vendor unique)
 * The major device number should be unsigned but POSIX does not say anything
 * about the content and defined dev_t to be a signed int.
 */
/* ARGSUSED */
LOCAL void
get_major(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;
	BOOL	neg = FALSE;
	dev_t	d;

	if (len == 0) {
		info->f_xflags &= ~XF_DEVMAJOR;
		return;
	}
	if (get_snumber(keyword, arg, &ull, &neg,
					-(Ullong)MAJOR_T_MIN, MAJOR_T_MAX)) {
		info->f_xflags |= XF_DEVMAJOR;
		if (neg)
			info->f_rdevmaj = -ull;
		else
			info->f_rdevmaj = ull;
		d = makedev(info->f_rdevmaj, 0);
		d = major(d);
		if ((neg && -d != ull) || (!neg && d != ull)) {
			xh_rangeerr(keyword, arg, len);
			info->f_flags |= F_BAD_META;
		}
	}
}

/*
 * get minor device number (always vendor unique)
 * The minor device number should be unsigned but POSIX does not say anything
 * about the content and defined dev_t to be a signed int.
 */
/* ARGSUSED */
LOCAL void
get_minor(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;
	BOOL	neg = FALSE;
	dev_t	d;

	if (len == 0) {
		info->f_xflags &= ~XF_DEVMINOR;
		return;
	}
	if (get_snumber(keyword, arg, &ull, &neg,
					-(Ullong)MINOR_T_MIN, MINOR_T_MAX)) {
		info->f_xflags |= XF_DEVMINOR;
		if (neg)
			info->f_rdevmin = -ull;
		else
			info->f_rdevmin = ull;
		d = makedev(0, info->f_rdevmin);
		d = minor(d);
		if ((neg && -d != ull) || (!neg && d != ull)) {
			xh_rangeerr(keyword, arg, len);
			info->f_flags |= F_BAD_META;
		}
	}
}

/*
 * get number of minor bits for this file (vendor unique)
 * This is naturally unsigned.
 */
/* ARGSUSED */
LOCAL void
get_minorbits(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;

	if (len == 0) {
		info->f_devminorbits = 0;
		return;
	}
	if (get_unumber(keyword, arg, &ull, INO_T_MAX)) {
		info->f_devminorbits = ull;
		if (info->f_devminorbits != ull) {
			xh_rangeerr(keyword, arg, len);
			info->f_devminorbits = 0;
		}
	}
}

/*
 * get device number of device containing FS (vendor unique)
 * The device number should be unsigned but POSIX does not say anything
 * about the content and defined dev_t to be a signed int.
 */
/* ARGSUSED */
LOCAL void
get_dev(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;
	BOOL	neg = FALSE;

	info->f_devmaj = 0;
	info->f_devmin = 0;
	if (len == 0) {
		info->f_dev = 0;
		return;
	}
	if (get_snumber(keyword, arg, &ull, &neg,
					-(Ullong)DEV_T_MIN, DEV_T_MAX)) {
		int	mbits;
		Llong	ll;

		if (neg)
			info->f_dev = ll = -ull;
		else
			info->f_dev = ll = ull;

		mbits = info->f_devminorbits;
		if (mbits == 0)
			mbits = ginfo.f_devminorbits;
		if (mbits) {
			int	oerr = geterrno();

			seterrno(0);
			info->f_devmaj	= _dev_major(mbits, ll);
			info->f_devmin	= _dev_minor(mbits, ll);
			info->f_dev = makedev(info->f_devmaj, info->f_devmin);
			ull = dev_make(info->f_devmaj, info->f_devmin);
			/*
			 * Check whether the device from the archive
			 * can be represented on the local system.
			 */
			if (geterrno() ||
			    info->f_devmaj != major(info->f_dev) ||
			    info->f_devmin != minor(info->f_dev) ||
			    (Ullong)info->f_dev != ull) {
				xh_rangeerr(keyword, arg, len);
				info->f_dev = 0;
				info->f_devmaj = 0;
				info->f_devmin = 0;
			}
			seterrno(oerr);
			return;
		}
		/*
		 * Let us hope that both, the archiving and the
		 * extracting system use the same major()/minor()
		 * mapping.
		 */
		info->f_devmaj	= major(ll);
		info->f_devmin	= minor(ll);

		if ((neg && -info->f_dev != ull) ||
			(!neg && info->f_dev != ull)) {
			xh_rangeerr(keyword, arg, len);
			info->f_dev = 0;
		}
	}
}

/*
 * get inode number for this file (vendor unique)
 * POSIX defines ino_t to be unsigned.
 */
/* ARGSUSED */
LOCAL void
get_ino(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;

	if (len == 0) {
		info->f_ino = 0;
		return;
	}
	if (get_unumber(keyword, arg, &ull, INO_T_MAX)) {
		info->f_ino = ull;
		if (info->f_ino != ull) {
			xh_rangeerr(keyword, arg, len);
			info->f_ino = 0;
		}
	}
}

/*
 * get link count for this file (vendor unique)
 * POSIX defines nlink_t to ne signed but all real link counts in archives
 * need to be positive numbers.
 */
/* ARGSUSED */
LOCAL void
get_nlink(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	Ullong	ull;

	if (len == 0) {
		info->f_nlink = 0;
		return;
	}
	if (get_unumber(keyword, arg, &ull, NLINK_T_MAX)) {
		info->f_nlink = ull;
		if (info->f_nlink != ull) {
			xh_rangeerr(keyword, arg, len);
			info->f_nlink = 0;
		}
	}
}

/*
 * get tar file type or real file type for this file (vendor unique)
 */
/* ARGSUSED */
LOCAL void
get_filetype(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	int	i;

	if (len == 0) {
		if (keyword[7] == 'f')		/* "SCHILY.filetype" */
			info->f_rxftype = XT_BAD;
		else
			info->f_xftype = XT_BAD;
		return;
	}

	for (i = 0; i <= XT_BAD; i++) {
		if (xtnamelen_tab[i] != len)
			continue;
		if (streql(xttoname_tab[i], arg))
			break;
	}
	if (i >= XT_BAD)			/* Use type from tarhdr */
		return;

	if (keyword[7] == 'f') {		/* "SCHILY.filetype" */
		info->f_rxftype = i;
		info->f_filetype = XTTOST(info->f_rxftype);
		info->f_type = XTTOIF(info->f_rxftype);
	} else {				/* "SCHILY.tarfiletype" */
		info->f_xftype = i;
	}
}

#ifdef	USE_ACL

/*
 * XXX acl_access_text/acl_default_text are a bad idea. (see acl_unix.c)
 */
LOCAL pathstore_t	acl_access_text;
LOCAL pathstore_t	acl_default_text;

/* ARGSUSED */
LOCAL void
get_acl_type(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 11 && streql(arg, "POSIX draft"))
		return;
	if (len == 5 && streql(arg, "NFSv4"))
		return;

	info->f_flags |= F_BAD_ACL;

	if (badf & F_BAD_ACL)
		return;
	errmsgno(EX_BAD,
		"Unknown ACL type '%s' ignored at %lld.\n",
				arg, tblocks());
	badf |= F_BAD_ACL;
}

/* ARGSUSED */
LOCAL void
get_acl_access(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0 || (info->f_flags & F_BAD_ACL)) {
		info->f_xflags &= ~XF_ACL_ACCESS;
		info->f_acl_access = NULL;
		return;
	}
	if ((len + 2) > acl_access_text.ps_size)
		grow_pspace(PS_EXIT, &acl_access_text, (len + 2));
	if (acl_access_text.ps_path == NULL)
		return;
	if (ginfo.f_xflags & XF_BINARY) {
		strcpy(acl_access_text.ps_path, arg);
		info->f_xflags |= XF_ACL_ACCESS;
		info->f_acl_access = acl_access_text.ps_path;
	} else if (from_utf8((Uchar *)acl_access_text.ps_path,
		    acl_access_text.ps_size,
		    (Uchar *)arg, &len)) {
		info->f_xflags |= XF_ACL_ACCESS;
		info->f_acl_access = acl_access_text.ps_path;
	} else {
		bad_utf8(keyword, arg);
	}
}

/* ARGSUSED */
LOCAL void
get_acl_default(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0 || (info->f_flags & F_BAD_ACL)) {
		info->f_xflags &= ~XF_ACL_DEFAULT;
		info->f_acl_default = NULL;
		return;
	}
	if ((len + 2) > acl_default_text.ps_size)
		grow_pspace(PS_EXIT, &acl_default_text, (len + 2));
	if (acl_default_text.ps_path == NULL)
		return;
	if (ginfo.f_xflags & XF_BINARY) {
		strcpy(acl_default_text.ps_path, arg);
		info->f_xflags |= XF_ACL_DEFAULT;
		info->f_acl_default = acl_default_text.ps_path;
	} else if (from_utf8((Uchar *)acl_default_text.ps_path,
		    acl_default_text.ps_size,
		    (Uchar *)arg, &len)) {
		info->f_xflags |= XF_ACL_DEFAULT;
		info->f_acl_default = acl_default_text.ps_path;
	} else {
		bad_utf8(keyword, arg);
	}
}

LOCAL pathstore_t	acl_ace_text;

/* ARGSUSED */
LOCAL void
get_acl_ace(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0 || (info->f_flags & F_BAD_ACL)) {
		info->f_xflags &= ~XF_ACL_ACE;
		info->f_acl_ace = NULL;
		return;
	}
	if ((len + 2) > acl_ace_text.ps_size)
		grow_pspace(PS_EXIT, &acl_ace_text, (len + 2));
	if (acl_ace_text.ps_path == NULL)
		return;
	if (ginfo.f_xflags & XF_BINARY) {
		strcpy(acl_ace_text.ps_path, arg);
		info->f_xflags |= XF_ACL_ACE;
		info->f_acl_ace = acl_ace_text.ps_path;
	} else if (from_utf8((Uchar *)acl_ace_text.ps_path,
		    acl_ace_text.ps_size,
		    (Uchar *)arg, &len)) {
		info->f_xflags |= XF_ACL_ACE;
		info->f_acl_ace = acl_ace_text.ps_path;
	} else {
		bad_utf8(keyword, arg);
	}
}

#endif  /* USE_ACL */

#ifdef USE_XATTR

/* ARGSUSED */
LOCAL void
get_attr(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	register star_xattr_t	*x;
	register int		num = 0;

	if (len == 0) {
		/*
		 * This is not the right way as we clear all xattr info.
		 */
		if (static_xattr) {
			free(static_xattr);
			static_xattr = NULL;
		}
		info->f_fflags = 0;
		info->f_xflags &= ~XF_FFLAGS;
		return;
	}
	if (static_xattr) {
		for (x = static_xattr; x->name; x++)
			num++;
		x = ___realloc(static_xattr,
			(num+2) * sizeof (star_xattr_t), "extended attribute");
	} else {
		x = ___malloc(
			(num+2) * sizeof (star_xattr_t), "extended attribute");
	}
	static_xattr = x;
	x += num;

	/*
	 * should always succeed ...
	 */
	if (strncmp(keyword, "SCHILY.xattr.", 13) == 0)
		keyword += 13;

	x->name = ___malloc(strlen(keyword)+1, "extended attribute");
	x->value = ___malloc(len+1, "extended attribute");
	strcpy(x->name, keyword);
	x->value_len = len;
	movebytes(arg, x->value, len);
	((char *)x->value)[len] = '\0';
	fillbytes(x+1, sizeof (star_xattr_t), '\0');

	info->f_xattr = static_xattr;
	info->f_xflags |= XF_XATTR;
}

#endif  /* USE_XATTR */

#ifdef	USE_FFLAGS

/* ARGSUSED */
LOCAL void
get_xfflags(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		info->f_fflags = 0;
		info->f_xflags &= ~XF_FFLAGS;
		return;
	}
	texttoflags(info, arg);
	info->f_xflags |= XF_FFLAGS;
}

#endif	/* USE_FFLAGS */

/* ARGSUSED */
LOCAL void
get_dir(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	info->f_dir = 0;
	info->f_dirlen = 0;

	if (len == 0)
		return;
	/*
	 * Old archives had only one nul at the end.
	 * Note that we propagate the space from the xheader extract buffer
	 * to the extraction process (e.g. diff.c or incremental restore.c)
	 */
	if (arg[len-2] != '\0')
		arg[len++] = '\0';	/* Kill '\n' */

	if (ginfo.f_xflags & XF_BINARY) {
		;
	} else {
		/*
		 * The non UTF-8 string is shorter so we convert in place.
		 */
		from_utf8((Uchar *)arg, len, (Uchar *)arg, &len);
	}
	info->f_dir = arg;
	info->f_dirlen = len;
}

/* ARGSUSED */
LOCAL void
get_iarray(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	register int	dirents = 0;
	register int	i;
	register char	*p = arg;
	register ino_t	*ino;
		Ullong	ull;

	if (info->f_dirinos)
		free(info->f_dirinos);
	info->f_dirinos = NULL;
	info->f_dirents = 0;

	if (len == 0)
		return;

	while (p) {
		if (*p == ' ')
			p++;
		p = strchr(p, ' ');
		dirents++;
	}
	ino = ___malloc(dirents * sizeof (ino_t), "inos");

	for (p = arg, i = 0; i < dirents; i++) {
		if (*p == ' ')
			p++;
		seterrno(0);
		p = astoullb(p, &ull, 10);
		if (*p != ' ' && *p != '\0') {
			errmsgno(EX_BAD,
			"Bad number '%s' for '%s' in extended header al %lld.\n",
							arg, keyword, tblocks());
			/* XXX ino -> 0 and continue instead? */
			free(ino);
			return;
		} else {
			if (geterrno() != 0) {
				errmsgno(EX_BAD,
				"Number overflow with '%s' for '%s' in extended header at %lld.\n",
					arg, keyword, tblocks());
				/* XXX ino -> 0 and continue instead? */
				free(ino);
				return;
			}
		}
		if (ull > INO_T_MAX) {
			xh_rangeerr(keyword, arg, len);
			ino[i] = 0;
		} else {
			ino[i] = ull;
		}
	}
	info->f_dirinos = ino;
	info->f_dirents = dirents;
}

/* ARGSUSED */
LOCAL void
get_release(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		if (grip->release) {
			if ((grip->gflags & GF_NOALLOC) == 0)
				free(grip->release);
			grip->release = NULL;
		}
		grip->gflags &= ~GF_RELEASE;
		return;
	}
	if (ginfo.f_xflags & XF_BINARY) {
		;
	} else {
		/*
		 * The non UTF-8 string is shorter so we convert in place.
		 */
		from_utf8((Uchar *)arg, len, (Uchar *)arg, &len);
	}
	grip->gflags |= GF_RELEASE;
	grip->release = ___savestr(arg);
}

/* ARGSUSED */
LOCAL void
get_archtype(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
	if (len == 0) {
		grip->gflags &= ~GF_ARCHTYPE;
		grip->archtype = H_UNDEF;
		return;
	}
	grip->gflags |= GF_ARCHTYPE;
	grip->archtype = hdr_type(arg);
}

/*
 * Get the charset used for path, linkpath, uname and gname.
 */
/* ARGSUSED */
LOCAL void
get_hdrcharset(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
#ifdef	__needed__
	BOOL	is_global = info == &ginfo;
#endif

	if (len == 23 && streql("ISO-IR 10646 2000 UTF-8", arg)) {
		info->f_xflags &= ~XF_BINARY;
	} else if (len == 6 && streql("BINARY", arg)) {
		info->f_xflags |= XF_BINARY;
	} else {
		unsup_arg(keyword, arg);
	}
}

/*
 * Dummy 'get' function used for all fields that we don't yet understand or
 * fields that we indent to ignore.
 */
/* ARGSUSED */
LOCAL void
get_dummy(info, keyword, klen, arg, len)
	FINFO	*info;
	char	*keyword;
	int	klen;
	char	*arg;
	int	len;
{
}

LOCAL void
unsup_arg(keyword, arg)
	char	*keyword;
	char	*arg;
{
	errmsgno(EX_BAD,
		"Unsupported arg '%s' for '%s' in extended header at %lld.\n",
		arg, keyword, tblocks());
}

LOCAL void
bad_utf8(keyword, arg)
	char	*keyword;
	char	*arg;
{
	errmsgno(EX_BAD, "Bad UTF-8 arg '%s' for '%s' in extended header at %lld.\n",
		arg, keyword, tblocks());
}
