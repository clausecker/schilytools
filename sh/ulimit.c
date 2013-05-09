/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
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
 * This file contains modifications Copyright 2008-2013 J. Schilling
 *
 * @(#)ulimit.c	1.12 13/04/26 2008-2013 J. Schilling
 */
#ifdef	SCHILY_BUILD
#include <schily/mconfig.h>
#endif
#ifndef lint
static	UConst char sccsid[] =
	"@(#)ulimit.c	1.12 13/04/26 2008-2013 J. Schilling";
#endif

/*
 * ulimit builtin
 */

#ifdef	SCHILY_BUILD
#include <schily/time.h>
#include <schily/resource.h>
#define	rlim_t	Intmax_t		/* XXX may go away with <schily/resource.h> */
#else
#include <sys/resource.h>
#include <stdlib.h>
#endif

#include "defs.h"

static struct rlimtab {
	int	value;
	char	*name;
	char	*scale;
	rlim_t	divisor;
} rlimtab[] = {
#ifdef	RLIMIT_CPU
{	RLIMIT_CPU,	"time",		"seconds",	1,	},
#endif
#ifdef	RLIMIT_FSIZE
{	RLIMIT_FSIZE,	"file",		"blocks",	512,	},
#endif
#ifdef	RLIMIT_DATA
{	RLIMIT_DATA,	"data",		"kbytes",	1024,	},
#endif
#ifdef	RLIMIT_STACK
{	RLIMIT_STACK,	"stack",	"kbytes",	1024,	},
#endif
#ifdef	RLIMIT_CORE
{	RLIMIT_CORE,	"coredump",	"blocks",	512,	},
#endif
#ifdef	RLIMIT_MEMLOCK
{	RLIMIT_MEMLOCK,	"memlock",	"kbytes",	1024	},
#endif
#ifdef	RLIMIT_RSS
{	RLIMIT_RSS,	"memoryuse",	"kbytes",	1024,	},
#endif
#ifdef	RLIMIT_NOFILE
{	RLIMIT_NOFILE,	"nofiles",	"descriptors",	1,	},
#endif
#ifdef	RLIMIT_NPROC
{	RLIMIT_NPROC,	"processes",	"count",	1	},
#endif
#ifdef	RLIMIT_VMEM
{	RLIMIT_VMEM,	"memory",	"kbytes",	1024,	},
#endif
{	0,		NULL,		NULL,		0,	},
};

	void	sysulimit	__PR((int argc, char **argv));

void
sysulimit(argc, argv)
	int	argc;
	char	**argv;
{
	extern int opterr, optind;
	int savopterr, savoptind, savsp;
	char *savoptarg;
	char *args;
	char errargs[PATH_MAX];
	int hard, soft, cnt, c, res;
	rlim_t limit, new_limit;
	struct rlimit rlimit;
	char resources[RLIM_NLIMITS];
	struct rlimtab *rlp;

	for (res = 0;  res < RLIM_NLIMITS; res++) {
		resources[res] = 0;
	}

	savoptind = optind;
	savopterr = opterr;
	savsp = _sp;
	savoptarg = optarg;
	optind = 1;
	_sp = 1;
	opterr = 0;
	hard = 0;
	soft = 0;
	cnt = 0;

	while ((c = getopt(argc, argv, "HSacdflmnstuv")) != -1) {
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
		case 'c':
#ifdef	RLIMIT_CORE
			res = RLIMIT_CORE;
#endif
			break;
		case 'd':
#ifdef	RLIMIT_DATA
			res = RLIMIT_DATA;
#endif
			break;
		case 'f':
#ifdef	RLIMIT_FSIZE
			res = RLIMIT_FSIZE;
#endif
			break;
		case 'l':
#ifdef	RLIMIT_MEMLOCK
			res = RLIMIT_MEMLOCK;
#endif
			break;
		case 'm':
#ifdef	RLIMIT_RSS
			res = RLIMIT_RSS;
#endif
			break;
		case 'n':
#ifdef	RLIMIT_NOFILE
			res = RLIMIT_NOFILE;
#endif
			break;
		case 's':
#ifdef	RLIMIT_STACK
			res = RLIMIT_STACK;
#endif
			break;
		case 't':
#ifdef	RLIMIT_CPU
			res = RLIMIT_CPU;
#endif
			break;
		case 'u':
#ifdef	RLIMIT_NPROC
			res = RLIMIT_NPROC;
#endif
			break;
		case 'v':
#ifdef	RLIMIT_VMEM
			res = RLIMIT_VMEM;
#endif
			break;
		case '?':
			gfailure((unsigned char *)usage, ulimuse);
			goto err;
		}
		resources[res]++;
		cnt++;
	}

#ifdef	RLIMIT_FSIZE
	if (cnt == 0) {
		resources[res = RLIMIT_FSIZE]++;
		cnt++;
	}
#endif

	/*
	 * if out of arguments, then print the specified resources
	 */

	if (optind == argc) {
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
			if (cnt > 1) {
				prs_buff(_gettext(rlp->name));
				prc_buff('(');
				prs_buff(_gettext(rlp->scale));
				prc_buff(')');
				prc_buff(' ');
			}
			if (soft) {
				if (rlimit.rlim_cur == RLIM_INFINITY) {
					prs_buff(_gettext("unlimited"));
				} else  {
					prull_buff(rlimit.rlim_cur /
					    rlp->divisor);
				}
			}
			if (hard && soft) {
				prc_buff(':');
			}
			if (hard) {
				if (rlimit.rlim_max == RLIM_INFINITY) {
					prs_buff(_gettext("unlimited"));
				} else  {
					prull_buff(rlimit.rlim_max /
					    rlp->divisor);
				}
			}
			prc_buff('\n');
		}
		goto err;
	}

	if (cnt > 1 || optind + 1 != argc) {
		gfailure((unsigned char *)usage, ulimuse);
		goto err;
	}

	if (eq(argv[optind], "unlimited")) {
		limit = RLIM_INFINITY;
	} else {
		args = argv[optind];

		new_limit = limit = 0;
		do {
			if (*args < '0' || *args > '9') {
				snprintf(errargs, PATH_MAX-1,
				"%s: %s", argv[0], args);
				failure((unsigned char *)errargs, badnum);
				goto err;
			}
			/* Check for overflow! */
			new_limit = (limit * 10) + (*args - '0');
			if (new_limit >= limit) {
				limit = new_limit;
			} else {
				snprintf(errargs, PATH_MAX-1,
				"%s: %s", argv[0], args);
				failure((unsigned char *)errargs, badnum);
				goto err;
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
			goto err;
		}
	}

	if (getrlimit(res, &rlimit) < 0) {
		failure((unsigned char *)argv[0], badnum);
		goto err;
	}

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
		"%s: %s", argv[0], argv[optind]);
		failure((unsigned char *)errargs, badulimit);
	}

err:
	optind = savoptind;
	opterr = savopterr;
	_sp = savsp;
	optarg = savoptarg;
}
