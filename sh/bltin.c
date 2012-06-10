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
#pragma ident	"@(#)bltin.c	1.16	06/06/16 SMI"
#endif

#include "defs.h"

/*
 * This file contains modifications Copyright 2008-2012 J. Schilling
 *
 * @(#)bltin.c	1.27 12/06/10 2008-2012 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)bltin.c	1.27 12/06/10 2008-2012 J. Schilling";
#endif

/*
 *
 * UNIX shell
 *
 */

#include	<errno.h>
#include	"sym.h"
#include	"hash.h"
#include	"abbrev.h"
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/times.h>

	void	builtin	__PR((int type, int argc, unsigned char **argv,
							struct trenod *t));

void
builtin(type, argc, argv, t)
int type;
int argc;
unsigned char **argv;
struct trenod *t;
{
	short fdindex = initio(t->treio, (type != SYSEXEC));
	unsigned char *a1 = argv[1];
	struct argnod *np = NULL;

	switch (type)
	{

	case SYSSUSP:
		syssusp(argc, (char **)argv);
		break;

	case SYSSTOP:
		sysstop(argc, (char **)argv);
		break;

	case SYSKILL:
		syskill(argc, (char **)argv);
		break;

	case SYSFGBG:
		sysfgbg(argc, (char **)argv);
		break;

	case SYSJOBS:
		sysjobs(argc, (char **)argv);
		break;

	case SYSDOT:
		if (a1)
		{
			int	f;

			if ((f = pathopen(getpath(a1), a1)) < 0)
				failed(a1, notfound);
			else
				execexp(0, (Intptr_t)f);
		}
		break;

	case SYSTIMES:
		{
			struct tms tms;

			times(&tms);
			prt(tms.tms_utime);
			prc_buff(SPACE);
			prt(tms.tms_stime);
			prc_buff(NL);
			prt(tms.tms_cutime);
			prc_buff(SPACE);
			prt(tms.tms_cstime);
			prc_buff(NL);
		}
		break;

	case SYSEXIT:
		if (tried_to_exit++ || endjobs(JOB_STOPPED)) {
			flags |= forcexit;	/* force exit */
			exitsh(a1 ? stoi(a1) : retval);
		}
		break;

	case SYSNULL:
		break;

	case SYSCONT:
		if (loopcnt)
		{
			execbrk = breakcnt = 1;
			if (a1)
				breakcnt = stoi(a1);
			if (breakcnt > loopcnt)
				breakcnt = loopcnt;
			else
				breakcnt = -breakcnt;
		}
		break;

	case SYSBREAK:
		if (loopcnt)
		{
			execbrk = breakcnt = 1;
			if (a1)
				breakcnt = stoi(a1);
			if (breakcnt > loopcnt)
				breakcnt = loopcnt;
		}
		break;

	case SYSTRAP:
		systrap(argc, (char **)argv);
		break;

	case SYSEXEC:
		argv++;
		ioset = 0;
		if (a1 == 0) {
			setmode(0);
			break;
		}
		/* FALLTHROUGH */

#ifdef RES	/* Research includes login as part of the shell */

	case SYSLOGIN:
		if (!endjobs(JOB_STOPPED|JOB_RUNNING))
			break;
		oldsigs();
		execa(argv, -1);
		done(0);
#else

	case SYSNEWGRP:
		if (flags & rshflg)
			failed(argv[0], restricted);
		else if (!endjobs(JOB_STOPPED|JOB_RUNNING))
			break;
		else
		{
			flags |= forcexit; /* bad exec will terminate shell */
			oldsigs();
			rmtemp(0);
			rmfunctmp();
#ifdef ACCT
			doacct();
#endif
			execa(argv, -1);
			done(0);
			/* NOTREACHED */
		}

#endif

	case SYSPOPD:
		/* FALLTHROUGH */
	case SYSPUSHD:
		init_dirs();
		if (a1 && a1[0] == '-') {
			if (a1[1] == '-' && a1[2] == '\0') {	/* "--" */
				a1 = argv[2];
			} else if (type == SYSPUSHD && a1[1] == '\0') {
				extern struct namnod opwdnod;

				a1 = opwdnod.namval;
				if (a1 == NULL || *a1 == '\0') {
					free(np);
					failed(a1, baddir);
					break;
				}
			} else {
				int	off = stoi(&a1[1]);

				if (!(np = pop_dir(off))) {
					failed(a1, badoff);
					break;
				}
				a1 = np->argval;
			}
		} else if (type == SYSPOPD) {
			struct argnod *dnp = pop_dir(0);

			/*
			 * init_dirs() grants pop_dir(0) != NULL
			 */
			if (dnp->argnxt == NULL) {
				error(emptystack);
				break;
			}
			a1 = dnp->argnxt->argval;
			free(dnp);
		}
		/* FALLTHROUGH */
	case SYSCD:
		if (type == SYSCD && a1 && a1[0] == '-') {
			if (a1[1] == '-' && a1[2] == '\0') {	/* "--" */
				a1 = argv[2];
			} else if (a1[1] == '\0') {
				extern struct namnod opwdnod;

				a1 = opwdnod.namval;
				if (a1 == NULL || *a1 == '\0') {
					free(np);
					failed(a1, baddir);
					break;
				}
			}
		}
		/*
		 * A restricted Shell does not allow "cd" at all.
		 */
		if (flags & rshflg) {
			free(np);
			failed(argv[0], restricted);
		} else if ((a1 && *a1) || (a1 == 0 && (a1 = homenod.namval))) {
			unsigned char *cdpath;
			unsigned char *dir;
			int f;

			/*
			 * Make sure that cwdname[] is set to be able to track
			 * the previous working directory in OLDPWD.
			 */
			cwdset();

			if ((cdpath = cdpnod.namval) == 0 ||
			    *a1 == '/' ||
			    cf(a1, (unsigned char *)".") == 0 ||
			    cf(a1, (unsigned char *)"..") == 0 ||
			    (*a1 == '.' && (*(a1+1) == '/' || (*(a1+1) == '.' && *(a1+2) == '/'))))
				cdpath = (unsigned char *)nullstr;

			do
			{
				dir = cdpath;
				cdpath = catpath(cdpath, a1);
			}
			while ((f = chdir((const char *) curstak())) < 0 &&
			    cdpath);

			free(np);
			if (f < 0) {
				switch (errno) {
#ifdef	EMULTIHOP
				case EMULTIHOP:
					failed(a1, emultihop);
					break;
#endif
				case ENOTDIR:
					failed(a1, enotdir);
					break;
				case ENOENT:
					failed(a1, enoent);
					break;
				case EACCES:
					failed(a1, eacces);
					break;
#ifdef	ENOLINK
				case ENOLINK:
					failed(a1, enolink);
					break;
#endif
				default:
					failed(a1, baddir);
					break;
				}
			}
			else
			{
				unsigned char	*wd;

				cwd(curstak());		/* Canonic from stak */
				wd = cwdget();		/* Get reliable cwd  */
				if (type != SYSPUSHD)
					free(pop_dir(0));
				push_dir(wd);		/* Update dir stack  */
				if (pr_dirs(1))		/* If already printed */
					wd = NULL;	/* don't do it again */
				if (cf((unsigned char *)nullstr, dir) &&
				    *dir != ':' &&
				    any('/', curstak()) &&
				    flags & prompt) {
					if (wd) {	/* Not yet printed */
						prs_buff(wd);
						prc_buff(NL);
					}
				}
				if (flags & localaliasflg) {
					ab_use(LOCAL_AB, (char *)localname);
				}
			}
			zapcd();
		}
		else
		{
			free(np);
			/*
			 * cd "" is not permitted,
			 * cd	 without parameter is cd $HOME
			 * but $HOME was not set.
			 */
			if (a1)
				error(nulldir);
			else
				error(nohome);
		}

		break;

	case SYSSHFT:
		{
			int places;

			places = a1 ? stoi(a1) : 1;

			if ((dolc -= places) < 0)
			{
				dolc = 0;
				error(badshift);
			}
			else
				dolv += places;
		}

		break;

	case SYSWAIT:
		syswait(argc, (char **)argv);
		break;

	case SYSREAD:
		if (argc < 2)
			failed(argv[0], mssgargn);
		rwait = 1;
		exitval = readvar(&argv[1]);
		rwait = 0;
		break;

	case SYSSET:
		if (a1)
		{
			int	cnt;

			cnt = options(argc, argv);
			if (cnt > 1)
				setargs(argv + argc - cnt);
		} else if (comptr(t)->comset == 0)
		{
			/*
			 * scan name chain and print
			 */
			namscan(printnam);
		}
		break;

	case SYSRDONLY:
		exitval = 0;
		if (a1)
		{
			while (*++argv)
				attrib(lookup(*argv), N_RDONLY);
		}
		else
			namscan(printro);

		break;

	case SYSXPORT:
		{
			struct namnod	*n;

			exitval = 0;
			if (a1)
			{
				while (*++argv)
				{
					n = lookup(*argv);
					if (n->namflg & N_FUNCTN)
						error(badexport);
					else
						attrib(n, N_EXPORT);
				}
			}
			else
				namscan(printexp);
		}
		break;

	case SYSEVAL:
		if (a1)
			execexp(a1, (Intptr_t)&argv[2]);
		break;

	case SYSDOSH:
		if (a1 == NULL) {
			break;
		} else {
			struct dolnod	*olddolh;
			unsigned char	**olddolv = dolv;
			int		olddolc = dolc;
			struct ionod	*io = t->treio;
			short		idx;

			/*
			 * save current positional parameters
			 */
			olddolh = (struct dolnod *)savargs(funcnt);
			funcnt++;
			setargs(&argv[2]);
			idx = initio(io, 1);
			execexp(a1, (Intptr_t)0);
			restore(idx);
			(void) restorargs(olddolh, funcnt);
			dolv = olddolv;
			dolc = olddolc;
			funcnt--;
		}
		break;

	case SYSREPEAT:
		if (a1) {
			extern int opterr, optind;
			int	savopterr;
			int	savoptind;
			int	savsp;
			char	*savoptarg;
			int	c;
			int	delay = 0;
			int	count = -1;

			savoptind = optind;
			savopterr = opterr;
			savsp = _sp;
			savoptarg = optarg;
			optind = 1;
			_sp = 1;
			opterr = 0;

			while ((c = getopt(argc, (char **)argv,
						"c:(count)d:(delay)")) != -1) {
				switch (c) {
				case 'c':
					count = stoi((unsigned char *)optarg);
					break;
				case 'd':
					delay = stoi((unsigned char *)optarg);
					break;
				case '?':
					gfailure((unsigned char *)usage, repuse);
					goto err;
				}
			}
			while (count != 0) {
				unsigned char		*sav = savstak();
				struct ionod		*iosav = iotemp;

				execexp(argv[optind], (Intptr_t)&argv[optind+1]);
				tdystak(sav, iosav);

				if (delay > 0)
					sh_sleep(delay);
				if (count > 0)
					count--;
			}
err:
			optind = savoptind;
			opterr = savopterr;
			_sp = savsp;
			optarg = savoptarg;
		} else {
			gfailure((unsigned char *)usage, repuse);
		}
		break;

#ifndef RES
	case SYSULIMIT:
		sysulimit(argc, (char **)argv);
		break;

	case SYSUMASK:
		sysumask(argc, (char **)argv);
		break;
#endif

	case SYSTST:
		exitval = test(argc, argv);
		break;

	case SYSECHO:
		exitval = echo(argc, argv);
		break;

	case SYSHASH:
		exitval = 0;

		if (a1)
		{
			if (a1[0] == '-')
			{
				if (a1[1] == 'r')
					zaphash();
				else
					error(badopt);
			}
			else
			{
				while (*++argv)
				{
					if (hashtype(hash_cmd(*argv)) == NOTFOUND)
						failed(*argv, notfound);
				}
			}
		}
		else
			hashpr();

		break;

	case SYSDIRS:
		exitval = 0;
		pr_dirs(0);
		break;

	case SYSPWD:
		{
			exitval = 0;
			cwdprint();
		}
		break;

	case SYSRETURN:
		if (funcnt == 0)
			error(badreturn);

		execbrk = 1;
		exitval = (a1 ? stoi(a1) : retval);
		break;

	case SYSTYPE:
		exitval = 0;
		if (a1)
		{
			/* return success only if all names are found */
			while (*++argv)
				exitval |= what_is_path(*argv);
		}
		break;

	case SYSUNS:
		exitval = 0;
		if (a1)
		{
			while (*++argv)
				unset_name(*argv);
		}
		break;

	case SYSGETOPT: {
		int getoptval;
		struct namnod *n;
		extern unsigned char numbuf[];
		unsigned char *varnam = argv[2];
		unsigned char c[2];
		if (argc < 3) {
			failure(argv[0], mssgargn);
			break;
		}
		exitval = 0;
		n = lookup((unsigned char *)"OPTIND");
		optind = stoi(n->namval);
		if (argc > 3) {
			argv[2] = dolv[0];
			getoptval = getopt(argc-2, (char **)&argv[2], (char *)argv[1]);
		}
		else
			getoptval = getopt(dolc+1, (char **)dolv, (char *)argv[1]);
		if (getoptval == -1) {
			itos(optind);
			assign(n, numbuf);
			n = lookup(varnam);
			assign(n, (unsigned char *)nullstr);
			exitval = 1;
			break;
		}
		argv[2] = varnam;
		itos(optind);
		assign(n, numbuf);
		c[0] = (char) getoptval;
		c[1] = '\0';
		n = lookup(varnam);
		assign(n, c);
		n = lookup((unsigned char *)"OPTARG");
		assign(n, (unsigned char *)optarg);
		}
		break;

#ifdef	INTERACTIVE
	case SYSHISTORY:
		bhist();
		break;

	case SYSSAVEHIST:
		bshist(&intrptr);
		if (intrptr)
			*intrptr = 0;
		break;

	case SYSMAP:
		{
			int	f = 1;

			if (argc == 1) {
				list_map(&f);
			} else if (argc == 2 && eq(argv[1], "-r")) {
				remap();
			} else if (argc == 3 && eq(argv[1], "-u")) {
				del_map((char *)argv[2]);
			} else if (argc == 3 || argc == 4) {
				if (!add_map((char *)argv[1], (char *)argv[2], (char *)argv[3])) {
					prs(argv[1]);
					prs((unsigned char *)": ");
					prs((unsigned char *)"already defined\n");
					error("bad map");
				}
			} else if (argc > 4) {
				error(arglist);
			} else {
				error(mssgargn);
			}
		}
		break;
#endif

#ifdef	DO_SYSALLOC
	case SYSALLOC:
		chkmem();
		break;
#endif

	case SYSALIAS:
		sysalias(argc, argv);
		break;
	case SYSUNALIAS:
		sysunalias(argc, argv);
		break;

	default:
		prs_buff(_gettext("unknown builtin\n"));
	}


	flushb();
	restore(fdindex);
	chktrap();
}
