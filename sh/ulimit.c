/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#if defined(sun)
#pragma ident	"@(#)ulimit.c	1.14	06/06/16 SMI"
#endif

/*
 * Copyright 2008-2017 J. Schilling
 *
 * @(#)ulimit.c	1.27 17/12/20 2008-2017 J. Schilling
 */
#ifdef	SCHILY_INCLUDES
#include <schily/mconfig.h>
#endif
#ifndef lint
static	UConst char sccsid[] =
	"@(#)ulimit.c	1.27 17/12/20 2008-2017 J. Schilling";
#endif

/*
 * ulimit builtin
 */

#ifdef	SCHILY_INCLUDES
#include <schily/time.h>
#include <schily/resource.h>
#else
#include <sys/resource.h>
#include <stdlib.h>
#endif

#include "defs.h"

static char	unlimited[] = "unlimited";

static struct rlimtab {
	int		value;
	char		*name;
	char		*aname;
	char		*scale;
	unsigned char	option;
	rlim_t		divisor;
} rlimtab[] = {
#ifdef	RLIMIT_CPU
{	RLIMIT_CPU,	"time",		"cputime",	"seconds", 't', 1, },
#endif
#ifdef	RLIMIT_FSIZE
{	RLIMIT_FSIZE,	"file",		"filesize",	"blocks",  'f', 512, },
#endif
#ifdef	RLIMIT_DATA
{	RLIMIT_DATA,	"data",		"datasize",	"kbytes",  'd', 1024, },
#endif
#ifdef	RLIMIT_STACK
{	RLIMIT_STACK,	"stack",	"stacksize",	"kbytes",  's', 1024, },
#endif
#ifdef	RLIMIT_CORE
{	RLIMIT_CORE,	"coredump",	"coredumpsize",	"blocks",  'c', 512, },
#endif
#ifdef	RLIMIT_MEMLOCK
{	RLIMIT_MEMLOCK,	"memlock",	"memorylocked",	"kbytes",  'l', 1024 },
#endif
#ifdef	RLIMIT_RSS
{	RLIMIT_RSS,	"memoryuse",	NULL,		"kbytes",  'm', 1024, },
#endif
#if	defined(RLIMIT_UMEM) && !!defined(RLIMIT_RSS)
{	RLIMIT_UMEM,	"memoryuse",	NULL,		"kbytes",  'm', 1024, },
#endif
#ifdef	RLIMIT_NOFILE
{	RLIMIT_NOFILE,	"nofiles",	"descriptors",	"descriptors",
								    'n', 1, },
#endif
#if	defined(RLIMIT_OFILE) && !defined(RLIMIT_NOFILE)
{	RLIMIT_OFILE,	"nofiles",	"descriptors",	"descriptors",
								    'n', 1, },
#endif
#ifdef	RLIMIT_NPROC
{	RLIMIT_NPROC,	"processes",	"nproc",	"count",   'u', 1 },
#endif
#ifdef	RLIMIT_VMEM
{	RLIMIT_VMEM,	"memory",	"vmemsize",	"kbytes",  'v', 1024, },
#endif
#ifdef	RLIMIT_AS
{	RLIMIT_AS,	"addressspace",	NULL,		"kbytes",  'M', 1024, },
#endif
#ifdef	RLIMIT_HEAP	/* BS2000/OSD */
{	RLIMIT_HEAP,	"heapsize",	NULL,		"kBytes", '\0', 1024, },
#endif
#ifdef	RLIMIT_CONCUR	/* CONVEX max. # of processors per process */
{	RLIMIT_CONCUR,	"concurrency",	NULL,		"thread(s)", '\0', 1 },
#endif
#ifdef	RLIMIT_NICE
{	RLIMIT_NICE,	"schedpriority", "maxnice",	"nice",	   'e', 1, },
#endif
#ifdef	RLIMIT_SIGPENDING
{	RLIMIT_SIGPENDING, "sigpending", NULL,		"count",   'i', 1, },
#endif
#ifdef	RLIMIT_NPTS	/* FreeBSD maximum # of pty's */
{	RLIMIT_NPTS,	"npty",		 NULL,		"count",   'P', 1, },
#endif
#ifdef	RLIMIT_MSGQUEUE
{	RLIMIT_MSGQUEUE, "messagequeues", "msgqueues",	"count",   'q', 1, },
#endif
#ifdef	RLIMIT_RTPRIO
{	RLIMIT_RTPRIO,	"rtpriority",	"maxrtprio",	"nice",	   'r', 1, },
#endif
#ifdef	RLIMIT_SWAP
{	RLIMIT_SWAP,	"swap",		"swapsize",	"kbytes",  'w', 1024, },
#endif
#ifdef	RLIMIT_RTTIME
{	RLIMIT_RTTIME,	"rttime",	"maxrttime",	"usec",	   'R', 1, },
#endif
#ifdef	RLIMIT_LOCKS
{	RLIMIT_LOCKS,	"locks",	"filelocks",	"count",   'L', 1, },
/* bash compat: -x */
{	RLIMIT_LOCKS,	"locks",	NULL,		"count",   'x', 1, },
#endif
#ifdef	RLIMIT_SBSIZE	/* FreeBSD maximum size of all socket buffers */
{	RLIMIT_SBSIZE,	"sbsize",	NULL,		"bytes",   'b', 1, },
#endif
#ifdef	RLIMIT_KQUEUES	/* FreeBSD kqueues allocated */
{	RLIMIT_KQUEUES,	"kqueues",	NULL,		"count",   'k', 1, },
#endif
#ifdef	RLIMIT_UMTXP	/* FreeBSD process-shared umtx */
{	RLIMIT_UMTXP,	"umtx shared locks",	NULL,	"count",   'o', 1, },
#endif

{	0,		NULL,		NULL,	NULL,		0, 0,	},
};

	void	sysulimit	__PR((int argc, unsigned char **argv));

void
sysulimit(argc, argv)
	int		argc;
	unsigned char	**argv;
{
	struct optv optv;
	unsigned char *args;
	char errargs[PATH_MAX];
	int hard, soft, cnt, c, res;
	rlim_t limit, new_limit;
	struct rlimit rlimit;
	char resources[RLIM_NLIMITS];
	struct rlimtab *rlp;
#ifdef	DO_SYSLIMIT
	int	bsdmode = argv[0][0] == 'l';
#else
#define	bsdmode	0
#endif

	for (res = 0;  res < RLIM_NLIMITS; res++) {
		resources[res] = 0;
	}

	optinit(&optv);
	hard = 0;
	soft = 0;
	cnt = 0;

	while ((c = optget(argc, argv, &optv,
			    "HSacdefilmnqrstuvxLMPR")) != -1) {
		switch (c) {
		case 'S':
			soft++;
			continue;
		case 'H':
			hard++;
			continue;
		case 'a':
			for (res = 0;  res < RLIM_NLIMITS; res++) {
				resources[res]++;
			}
#ifdef	RLIM_NLIMITS
			cnt = RLIM_NLIMITS;
#endif
			continue;

		default:
			for (rlp = rlimtab; rlp->name; rlp++) {
				if (rlp->option == c) {
					res = rlp->value;
					break;
				}
			}
			break;

		case '?':
#ifdef	DO_SYSLIMIT
			gfailure(UC usage, bsdmode ? limuse : ulimuse);
#else
			gfailure(UC usage, ulimuse);
#endif
			return;
		}
		resources[res]++;
		cnt++;
	}
#ifdef	DO_SYSLIMIT
	if (cnt == 0 && optv.optind < argc) {
		args = argv[optv.optind];
		res = -1;

		for (rlp = rlimtab; rlp->name; rlp++) {
			if (strstr(rlp->name, C args) == rlp->name ||
			    (rlp->aname &&
			    strstr(rlp->aname, C args) == rlp->aname)) {
				if (res >= 0) {
					failure(args, ambiguous);
					return;
				}
				res = rlp->value;
			}
		}
		if (res < 0) {
			failure(args, enoent);
			return;
		}
		resources[res]++;
		cnt++;
		optv.optind++;
		bsdmode++;
	}
#endif

#ifdef	RLIMIT_FSIZE
	if (cnt == 0) {
#ifdef	DO_SYSLIMIT
		if (bsdmode) {
			for (res = 0;  res < RLIM_NLIMITS; res++) {
				resources[res] = 1;
			}
			cnt++;
		}
#endif
		resources[res = RLIMIT_FSIZE]++;
		cnt++;
	}
#endif

	/*
	 * if out of arguments, then print the specified resources
	 */
	if (optv.optind == argc) {
#ifdef	DO_SYSLIMIT
		if (bsdmode && !hard && !soft) {
			hard++;
			soft++;
		}
#endif
		/*
		 * No extra args, so we are in list mode.
		 */
		if (!hard && !soft) {
			soft++;
		}
		for (res = 0; res < RLIM_NLIMITS; res++) {
			if (resources[res] == 0) {
				continue;
			}
			for (rlp = rlimtab; rlp->name; rlp++) {
				if (rlp->value == res)
					break;
			}
			if (rlp->name == NULL)
				continue;
			if (getrlimit(res, &rlimit) < 0) {
				continue;
			}
			if (cnt > 1 || bsdmode) {
#ifdef	DO_ULIMIT_OPTS
				if (!bsdmode) {
					prc_buff('-');
					prc_buff(rlp->option);
					prc_buff(':');
					prc_buff(' ');
				}
#endif
#ifdef	DO_SYSLIMIT
				if (bsdmode && rlp->aname)
					prs_buff(_gettext(rlp->aname));
				else
#endif
					prs_buff(_gettext(rlp->name));
				prc_buff('(');
				prs_buff(_gettext(rlp->scale));
				prc_buff(')');
				prc_buff(' ');
			}
			if (soft) {
				if (rlimit.rlim_cur == RLIM_INFINITY) {
					prs_buff(_gettext(unlimited));
				} else  {
					prull_buff((UIntmax_t)rlimit.rlim_cur /
					    rlp->divisor);
				}
			}
			if (hard && soft) {
#ifdef	DO_SYSLIMIT
				if (bsdmode)
					prc_buff('\t');
				else
#endif
					prc_buff(':');
			}
			if (hard) {
				if (rlimit.rlim_max == RLIM_INFINITY) {
					prs_buff(_gettext(unlimited));
				} else  {
					prull_buff((UIntmax_t)rlimit.rlim_max /
					    rlp->divisor);
				}
			}
			prc_buff('\n');
		}
		return;
	}

	if (cnt > 1 || optv.optind + 1 != argc) {
#ifdef	DO_SYSLIMIT
		gfailure(UC usage, bsdmode ? limuse : ulimuse);
#else
		gfailure(UC usage, ulimuse);
#endif
		return;
	}

#ifdef	DO_SYSLIMIT
	if (strstr(unlimited, C argv[optv.optind]) == unlimited) {
#else
	if (eq(argv[optv.optind], unlimited)) {
#endif
		limit = RLIM_INFINITY;
	} else {
		args = argv[optv.optind];

		new_limit = limit = 0;
		do {
			if (*args < '0' || *args > '9') {
				snprintf(errargs, PATH_MAX-1,
				"%s: %s", argv[0], args);
				failure((unsigned char *)errargs, badnum);
				return;
			}
			/* Check for overflow! */
			new_limit = (limit * 10) + (*args - '0');
			if (new_limit >= limit) {
				limit = new_limit;
			} else {
				snprintf(errargs, PATH_MAX-1,
				"%s: %s", argv[0], args);
				failure((unsigned char *)errargs, badnum);
				return;
			}
		} while (*++args);

		for (rlp = rlimtab; rlp->name; rlp++) {
			if (rlp->value == res)
				break;
		}
		if (rlp->name == NULL)
			goto fail;

		/* Check for overflow! */
		new_limit = limit * rlp->divisor;
		if (new_limit >= limit) {
			limit = new_limit;
		} else {
			snprintf(errargs, PATH_MAX-1,
			"%s: %s", argv[0], args);
			failure((unsigned char *)errargs, badnum);
			return;
		}
	}

	if (getrlimit(res, &rlimit) < 0) {
		failure((unsigned char *)argv[0], badnum);
		return;
	}

#ifdef	DO_SYSLIMIT
	if (bsdmode && !hard && !soft) {
		soft++;
	}
#endif
	if (!hard && !soft) {
		hard++;
		soft++;
	}
	if (hard) {
		rlimit.rlim_max = limit;
	}
	if (soft) {
		rlimit.rlim_cur = limit;
	}

	if (setrlimit(res, &rlimit) < 0) {
	fail:
		snprintf(errargs, PATH_MAX-1,
		"%s: %s", argv[0], argv[optv.optind]);
		failure((unsigned char *)errargs, badulimit);
	}
}
