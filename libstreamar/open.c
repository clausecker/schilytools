/* @(#)open.c	1.5 18/05/17 Copyright 2017-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)open.c	1.5 18/05/17 Copyright 2017-2018 J. Schilling";
#endif
/*
 *	Open a StreamArchive
 *
 *	Copyright (c) 2017-2018 J. Schilling
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
#include <schily/utypes.h>
#include <schily/schily.h>
#include <schily/errno.h>
#include <schily/strar.h>
#include "header.h"

#define	my_uid		strar_my_uid
#define	mode_mask	strar_mode_mask
#define	old_umask	strar_old_umask

#define	ROOT_UID	0

EXPORT	uid_t	my_uid;

/*
 * This is used to allow extracting archives as non root when they
 * contain read only directories. It tries to stay as close as possible
 * to the user's umask when creating intermediate directories.
 * We do not modify the umask in a way that would even grant unepected
 * permissions to others for a short time.
 */
EXPORT	mode_t	mode_mask;
EXPORT	mode_t	old_umask;

#define	PERM_BITS	(S_IRWXU|S_IRWXG|S_IRWXO)	/* u/g/o basic perm */

LOCAL	void	init_umask	__PR((void));

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

int
strar_open(info, arname, arfd, mode, codeset)
	register FINFO	*info;
	const	char	*arname;
		int	arfd;
		int	mode;
	const	char	*codeset;
{
	strar_init(info);

	if (arname) {
		info->f_fpname = arname;
		if (mode & OM_ARFD) {
			if ((info->f_fp = fileluopen(arfd,
					(mode & OM_READ) ? "r":"wct")) == NULL)
				return (-1);
		} else if ((info->f_fp = fileopen(arname,
					(mode & OM_READ) ? "r":"wct")) == NULL)
			return (-1);
	} else {
		if (mode & OM_READ) {
			info->f_fpname = "stdin";
			info->f_fp = stdin;
		} else if (mode & OM_WRITE) {
			info->f_fpname = "stdout";
			info->f_fp = stdout;
		} else {
			seterrno(EINVAL);
			return (-1);
		}
	}
	my_uid = geteuid();
	init_umask();
	utf8_codeset(codeset);
	if (mode & OM_READ) {
		utf8_init(S_EXTRACT);
	} else if (mode & OM_WRITE) {
		utf8_init(S_CREATE);
	}

	return (0);
}
