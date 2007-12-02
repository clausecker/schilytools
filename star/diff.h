/* @(#)diff.h	1.13 06/02/14 Copyright 1993-2006 J. Schilling */
/*
 *	Definitions for the taylorable diff command
 *
 *	Copyright (c) 1993-2006 J. Schilling
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

#define	D_PERM		0x000001
#define	D_TYPE		0x000002
#define	D_NLINK		0x000004
#define	D_UID		0x000010
#define	D_GID		0x000020
#define	D_UNAME		0x000040
#define	D_GNAME		0x000080
#define	D_ID	(D_UID|D_GID|D_UNAME|D_GNAME)
#define	D_SIZE		0x000100
#define	D_DATA		0x000200
#define	D_RDEV		0x000400
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

/*
 * Atime frequently changes, it makes no sense to check it by default.
 * Mtime on symlinks cannot be copies, so do not check it too.
 */
#define	D_DEFLT	(~(D_ATIME|D_LMTIME))
#define	D_ALL	(~0L);

extern	long	diffopts;
