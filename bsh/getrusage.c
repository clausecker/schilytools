/* @(#)getrusage.c	1.24 06/09/26 Copyright 1987-2005 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)getrusage.c	1.24 06/09/26 Copyright 1987-2005 J. Schilling";
#endif
/*
 *	getrusage() emulation for SVr4
 *
 *	Copyright (c) 1987-2005 J. Schilling
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

/*
 * XXX Solaris kann kein 64 bit proc file (LF32)
 */
#undef	USE_LARGEFILES	/* XXX Temporärer Hack für Solaris */

#include <schily/mconfig.h>
#include <stdio.h>
#include "bsh.h"
#include <schily/unistd.h>
#include <schily/fcntl.h>
#include <schily/time.h>
#include "resource.h"	/* Die lokale Version vom bsh Port */

#ifdef	HAVE_SYS_PROCFS_H
#	include <sys/procfs.h>
#endif
#include "limit.h"


#undef	HAVE_GETRUSAGE

#ifndef	HAVE_GETRUSAGE
EXPORT	int	getrusage	__PR((int who, struct rusage *rusage));
#endif

#ifndef	HAVE_GETRUSAGE
/*
 * XXX who wird nicht untersteutzt!
 */
EXPORT int
getrusage(who,  rusage)
	int		who;
	struct rusage	*rusage;
{
#ifdef	PIOCUSAGE
	int		f;
	char		cproc[32];
	prusage_t	prusage;
#endif

	if (rusage)
		fillbytes((void *)rusage, sizeof (struct rusage), 0);

#ifdef	PIOCUSAGE
	if (who == RUSAGE_CHILDREN)
		return (-1);
	sprintf(cproc, "/proc/%ld", (long)getpid());
	if ((f = open(cproc, 0)) < 0)
		return (-1);
	if (ioctl(f, PIOCUSAGE, &prusage) < 0) {
		close(f);
		return (-1);
	}
	close(f);
	if (rusage) {
		rusage->ru_utime.tv_sec =  prusage.pr_utime.tv_sec;
		rusage->ru_utime.tv_usec = prusage.pr_utime.tv_nsec/1000;
		rusage->ru_stime.tv_sec =  prusage.pr_stime.tv_sec;
		rusage->ru_stime.tv_usec = prusage.pr_stime.tv_nsec/1000;

/* Missing fields:			*/
/*		rusage->ru_maxrss = XXX;*/
/*		rusage->ru_ixrss = XXX;*/
/*		rusage->ru_idrss = XXX;*/
/*		rusage->ru_isrss = XXX;*/

		rusage->ru_minflt = prusage.pr_minf;
		rusage->ru_majflt = prusage.pr_majf;
		rusage->ru_nswap  = prusage.pr_nswap;
		rusage->ru_inblock = prusage.pr_inblk;
		rusage->ru_oublock = prusage.pr_oublk;
		rusage->ru_msgsnd = prusage.pr_msnd;
		rusage->ru_msgrcv = prusage.pr_mrcv;
		rusage->ru_nsignals = prusage.pr_sigs;
		rusage->ru_nvcsw = prusage.pr_vctx;
		rusage->ru_nivcsw = prusage.pr_ictx;
	}
#endif
	return (0);
}
#endif	/* HAVE_GETRUSAGE */
