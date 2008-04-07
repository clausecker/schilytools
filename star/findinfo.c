/* @(#)findinfo.c	1.8 08/04/06 Copyright 2005-2007 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)findinfo.c	1.8 08/04/06 Copyright 2005-2007 J. Schilling";
#endif
/*
 *	Convert FINFO -> struct stat for find_expr()
 *
 *	Copyright (c) 2005-2007 J. Schilling
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
#include "star.h"
#include <schily/stat.h>
#include <schily/schily.h>
#include <schily/idcache.h>
#include "starsubs.h"

#ifdef	HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifndef	DEV_BSIZE
#define	DEV_BSIZE	512
#endif

#include <schily/walk.h>
#include <schily/find.h>

#ifdef	USE_FIND

extern	findn_t	*find_node;

EXPORT	BOOL	findinfo	__PR((FINFO *info));

/*
 * Called from extract.c, diff.c and list.c
 */
EXPORT BOOL
findinfo(info)
	FINFO	*info;
{
	struct stat sb;
	BOOL	ret;
extern	struct WALK walkstate;

	if (find_node == NULL)
		return (TRUE);

	sb.st_dev = info->f_dev;
	sb.st_ino = info->f_ino;
	sb.st_mode = info->f_mode|info->f_type;
	sb.st_nlink = info->f_nlink;
	sb.st_uid = info->f_uid;
	sb.st_gid = info->f_gid;
	sb.st_rdev = info->f_rdev;
	sb.st_size = info->f_rsize;
	sb.st_atime = info->f_atime;
	sb.st_mtime = info->f_mtime;
	sb.st_ctime = info->f_ctime;
	sb.st_blksize = 0;
#ifdef	HAVE_ST_BLOCKS
	sb.st_blocks = (info->f_size+1023) / DEV_BSIZE;
#endif
	walkstate.lname = info->f_lname;
	walkstate.pflags = PF_ACL|PF_XATTR;
	if (info->f_xflags & (XF_ACL_ACCESS|XF_ACL_DEFAULT))
		walkstate.pflags |= PF_HAS_ACL;
	if (info->f_xflags & XF_XATTR)
		walkstate.pflags |= PF_HAS_XATTR;
	ret = find_expr(info->f_name, (char *)filename(info->f_name),
						&sb, &walkstate, find_node);
	if (!ret)
		return (ret);
	info->f_mode = sb.st_mode & 07777;
	if (info->f_uid != sb.st_uid) {
		static  char    nuid[32+1];

		info->f_uid = sb.st_uid;
		if (ic_nameuid(nuid, sizeof (nuid), info->f_uid)) {
			info->f_uname = nuid;
			info->f_umaxlen = sizeof (nuid)-1;
		} else {
			snprintf(nuid, sizeof (nuid), "%llu", (Llong)info->f_uid);
			info->f_uname = nuid;
			info->f_umaxlen = sizeof (nuid)-1;
		}
	}
	if (info->f_gid != sb.st_gid) {
		static  char    ngid[32+1];

		info->f_gid = sb.st_gid;
		if (ic_namegid(ngid, sizeof (ngid), info->f_gid)) {
			info->f_gname = ngid;
			info->f_gmaxlen = sizeof (ngid)-1;
		} else {
			snprintf(ngid, sizeof (ngid), "%llu", (Llong)info->f_gid);
			info->f_gname = ngid;
			info->f_gmaxlen = sizeof (ngid)-1;
		}
	}
	return (ret);
}

#endif	/* USE_FIND */
