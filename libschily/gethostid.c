/* @(#)gethostid.c	1.17 06/09/26 Copyright 1995-2003 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)gethostid.c	1.17 06/09/26 Copyright 1995-2003 J. Schilling";
#endif
/*
 *	Copyright (c) 1995-2003 J. Schilling
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
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/utypes.h>
#ifdef	HAVE_SYS_SYSTEMINFO_H
#include <sys/systeminfo.h>
#endif
#include <schily/libport.h>

#ifndef	HAVE_GETHOSTID
EXPORT	long	gethostid	__PR((void));
#endif


#if	!defined(HAVE_GETHOSTID)

#if	defined(SI_HW_SERIAL)

EXPORT long
gethostid()
{
	long	id;

	char	hbuf[257];
	sysinfo(SI_HW_SERIAL, hbuf, sizeof (hbuf));
	id = atoi(hbuf);
	return (id);
}
#else

#include <schily/errno.h>
EXPORT long
gethostid()
{
	long	id = -1L;

#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (id);
}
#endif

#endif
