/* @(#)send.c	1.5 18/05/17 Copyright 2011-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)send.c	1.5 18/05/17 Copyright 2011-2018 J. Schilling";
#endif
/*
 *	Send data for a StreamArchive to the output file
 *
 *	Copyright (c) 2011-2018 J. Schilling
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

#include <schily/stdio.h>	/* fprintf */
#include <schily/unistd.h>
#include <schily/unistd.h>
#include <schily/utypes.h>
#include <schily/fcntl.h>
#include <schily/stat.h>
#include <schily/device.h>
#include <schily/schily.h>
#include <schily/idcache.h>
#include <schily/strar.h>
#include "header.h"
#include "table.h"

LOCAL	int	sendnulls	__PR((strar *s,
					off_t len, char *buf, size_t bufsize));

#define	S_ALLPERM	(S_IRWXU|S_IRWXG|S_IRWXO)
#define	S_ALLFLAGS	(S_ISUID|S_ISGID|S_ISVTX)
#define	S_ALLMODES	(S_ALLFLAGS | S_ALLPERM)

void
strar_archtype(s)
	strar		*s;
{
	strar_xbreset();
	strar_gen_text("archtype", "StreamArchive", -1, 0);
	filewrite(s->f_fp, strar_gxbuf(), strar_gxbsize());
}

void
strar_eof(s)
	strar		*s;
{
	strar_xbreset();
	strar_gen_text("status", "EOF", -1, 0);
	filewrite(s->f_fp, strar_gxbuf(), strar_gxbsize());
}

int
strar_send(s, name)
	strar		*s;
	const char	*name;
{
	struct stat	sb;

	if (lstat(name, &sb) < 0)
		return (-1);

	s->f_name = (char *)name;
	return (strar_st_send(s, &sb));
}

int
strar_st_send(s, sp)
	strar		*s;
	struct stat	*sp;
{
	struct stat sb;
	int	f = -1;		/* Make sily GCC quiet */
	int	len = 0;
	int	cnt = 0;
	off_t	amt = 0;
	int	ret = 0;
	int	type;
	int	xflags = s->f_xflags;
	int	utf8 = (s->f_xflags & XF_BINARY) ? 0 : T_UTF8;
	char	buf[32*1024];

#define	MAX_UNAME	64	/* The maximum length of a user/group name */
	char	name[MAX_UNAME+1];

	strar_xbreset();

	s->f_xftype = s->f_rxftype = type = IFTOXT(sp->st_mode);

	if (type >= XT_FILE && type <= XT_CONT) {
		if (sp->st_size > 0) {
			if ((f = open(s->f_name, 0)) < 0) {
				return (-1);
			}
		}
	} else if ((xflags & XF_FILETYPE) == 0) {
		return (-1);
	}

	if (!utf8)
		strar_gen_text("hdrcharset", "BINARY", -1, 0);

	strar_gen_text("path", s->f_name, -1, utf8);

	if (type == XT_SLINK) {
		int	llen;

		llen = readlink(s->f_name, buf, sizeof (buf));
		if (llen >= 0 && llen < sizeof (buf)) {
			/*
			 * string from readlink is not null terminated
			 */
			buf[llen] = '\0';
			strar_gen_text("linkpath", buf, -1, utf8);
		}
	}
	if (s->f_cmdflags & CMD_VERBOSE) {
		s->f_size = sp->st_size;
		s->f_mode = sp->st_mode;
		s->f_mtime = sp->st_mtime;
		s->f_atime = sp->st_atime;
		s->f_ctime = sp->st_ctime;
		s->f_lname = buf;
		strar_vprint(s);
	}

	permtostr(sp->st_mode, buf);

	if (xflags & XF_FILETYPE)
		strar_gen_text("filetype", XTTONAME(IFTOXT(sp->st_mode)), -1, 0);
	if (xflags & XF_MODE)
		strar_gen_text("mode", buf, -1, 0);

	if (xflags & XF_DEV)
		strar_gen_number("dev", (long long)sp->st_dev);

	if (xflags & XF_INO)
		strar_gen_number("ino", (long long)sp->st_ino);

	if (xflags & XF_NLINK)
		strar_gen_number("nlink", (long long)sp->st_nlink);

	if (xflags & XF_UID)
		strar_gen_number("uid", (long long)sp->st_uid);
	if (xflags & XF_GID)
		strar_gen_number("gid", (long long)sp->st_gid);

	if (xflags & XF_UNAME) {
		ic_nameuid(name, sizeof (name)-1, sp->st_uid);
		if (name[0])
			strar_gen_text("uname", name, -1, utf8);
	}
	if (xflags & XF_GNAME) {
		ic_namegid(name, sizeof (name)-1, sp->st_gid);
		if (name[0])
			strar_gen_text("gname", name, -1, utf8);
	}

	if (type > XT_DIR) {
		strar_gen_number("devmajor", (long long)major(sp->st_rdev));
		strar_gen_number("devminor", (long long)minor(sp->st_rdev));
	}
	if (xflags & XF_ATIME)
		strar_gen_xtime("atime", sp->st_atime, stat_ansecs(sp));
	if (xflags & XF_MTIME)
		strar_gen_xtime("mtime", sp->st_mtime, stat_mnsecs(sp));
	if (xflags & XF_CTIME)
		strar_gen_xtime("ctime", sp->st_ctime, stat_cnsecs(sp));

	if (type < XT_FILE || type > XT_CONT)
		sp->st_size = 0;

	if (sp->st_size == 0) {
		strar_gen_number("size", (long long)0);
		strar_gen_number("status", (long long)0);
		filewrite(s->f_fp, strar_gxbuf(), strar_gxbsize());
		return (0);
	}

	strar_gen_number("size", (long long)sp->st_size);
	filewrite(s->f_fp, strar_gxbuf(), strar_gxbsize());
	strar_xbreset();

	seterrno(0);
	do {
		cnt = sizeof (buf);
		if ((amt + cnt) > sp->st_size)
			cnt = sp->st_size - amt;
		if ((len = read(f, buf, cnt)) > 0) {
			ssize_t n = filewrite(s->f_fp, buf, len);
			if (n < 0) {
				sendnulls(s,
					sp->st_size - amt, buf, sizeof (buf));
				break;
			}
			amt += n;
		}
	} while (len > 0 && amt < sp->st_size);
	if (amt < sp->st_size) {
		if (geterrno() == 0)
			errmsgno(EX_BAD, "File '%s' shrunk.\n",
				s->f_name);
		else
			errmsg("I/O error while extracting '%s'.\n",
				s->f_name);
		sendnulls(s, sp->st_size - amt, buf, sizeof (buf));
		len = -1;
	}
	if (fstat(f, &sb) < 0) {
		/*
		 * Should not happen.
		 */
		len = -1;
	} else {
		if (sp->st_size != sb.st_size) {
			len = -1;
			errmsgno(EX_BAD, "File '%s' changed size.\n",
				s->f_name);
		} else if (sp->st_mtime != sb.st_mtime) {
			len = -1;
			errmsgno(EX_BAD, "File '%s' changed content.\n",
				s->f_name);
		}
	}
	if (len < 0 || cnt < 0) {
		if (geterrno() == 0)
			seterrno(-1);
		strar_gen_number("status", (long long)geterrno());
		ret = -1;
	} else {
		strar_gen_number("status", (long long)0);
	}
	filewrite(s->f_fp, strar_gxbuf(), strar_gxbsize());
	close(f);

	return (ret);
}

LOCAL int
sendnulls(s, len, buf, bufsize)
	strar	*s;
	off_t	len;
	char	*buf;
	size_t	bufsize;
{
	off_t	amt = 0;
	int	cnt = 0;

	fillbytes(buf, bufsize, '\0');

	do {
		cnt = bufsize;
		if ((amt + cnt) > len)
			cnt = len - amt;
		cnt = filewrite(s->f_fp, buf, cnt);
		if (cnt < 0)
			return (-1);
		amt += cnt;
	} while (amt < len);

	return (0);
}
