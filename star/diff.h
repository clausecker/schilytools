/* @(#)diff.h	1.18 19/11/02 Copyright 1993-2019 J. Schilling */
/*
 *	Definitions for the taylorable diff command
 *
 *	Copyright (c) 1993-2019 J. Schilling
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

#ifndef	_DIFF_H
#define	_DIFF_H

#ifdef	__cplusplus
extern "C" {
#endif

#define	D_PERM		0x000001
#define	D_TYPE		0x000002
#define	D_NLINK		0x000004
#define	D_SYMPERM	0x000008
#define	D_UID		0x000010
#define	D_GID		0x000020
#define	D_UNAME		0x000040
#define	D_GNAME		0x000080
#define	D_ID	(D_UID|D_GID|D_UNAME|D_GNAME)
#define	D_SIZE		0x000100
#define	D_DATA		0x000200
#define	D_RDEV		0x000400

#define	D_UNUSED1	0x000800

#define	D_HLINK		0x001000
#define	D_SLINK		0x002000
#define	D_SLPATH	0x004000
#define	D_SPARS		0x008000
#define	D_ATIME		0x010000
#define	D_MTIME		0x020000
#define	D_CTIME		0x040000
#define	D_TIMES	(D_ATIME|D_MTIME|D_CTIME)
#define	D_LMTIME	0x080000
#define	D_XTIMES	(D_ATIME|D_MTIME|D_CTIME|D_LMTIME)
#define	D_DIR		0x100000
#define	D_ACL		0x200000
#define	D_XATTR		0x400000
#define	D_FFLAGS	0x800000
#define	D_ANTIME	0x1000000
#define	D_MNTIME	0x2000000
#define	D_CNTIME	0x4000000
#define	D_DNLINK	0x8000000
#define	D_MAX		0x80000000

/*
 * Atime frequently changes, it makes no sense to check it by default.
 * Mtime on symlinks cannot be copies, so do not check it too.
 */
#define	D_DEFLT	(~(D_SYMPERM|D_ATIME|D_LMTIME))
#define	D_ALL	(~0L);

extern	long	diffopts;

typedef struct diffs {
	int	d_perm;
	int	d_type;
	int	d_nlink;
	int	d_dnlink;
	int	d_symperm;
	int	d_uid;
	int	d_gid;
	int	d_uname;
	int	d_gname;
	int	d_size;
	int	d_data;
	int	d_rdev;
	int	d_hlink;
	int	d_slink;
	int	d_slpath;
	int	d_sparse;
	int	d_atime;
	int	d_mtime;
	int	d_ctime;
	int	d_antime;
	int	d_mntime;
	int	d_cntime;
	int	d_lmtime;
	int	d_dir;
	int	d_acl;
	int	d_xattr;
	int	d_fflags;
} diffs_t;

#ifdef	__cplusplus
}
#endif

#endif	/* _DIFF_H */
