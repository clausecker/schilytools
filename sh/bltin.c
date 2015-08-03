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
#pragma ident	"@(#)bltin.c	1.16	06/06/16 SMI"
#endif

#include "defs.h"

/*
 * Copyright 2008-2015 J. Schilling
 *
 * @(#)bltin.c	1.55 15/07/28 2008-2015 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)bltin.c	1.55 15/07/28 2008-2015 J. Schilling";
#endif

/*
 *
 * UNIX shell
 *
 */

#ifdef	INTERACTIVE
#include	<schily/shedit.h>
#endif

#include	<errno.h>
#include	"sym.h"
#include	"hash.h"
#ifdef	DO_SYSALIAS
#include	"abbrev.h"
#endif
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/times.h>

#include	<schily/resource.h>

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

	exitval = 0;
	exval_clear();

	switch (type) {

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
		if (a1) {
			int	f;

			if ((f = pathopen(getpath(a1), a1)) < 0)
				failed(a1, notfound);
			else
				execexp(0, (Intptr_t)f);
		}
		break;

	case SYSTIMES:
		{
			struct rusage	ru;

			getrusage(RUSAGE_SELF, &ru);
			prtv(&ru.ru_utime, TRUE);
			prc_buff(SPACE);
			prtv(&ru.ru_stime, TRUE);
			prc_buff(NL);
			getrusage(RUSAGE_CHILDREN, &ru);
			prtv(&ru.ru_utime, TRUE);
			prc_buff(SPACE);
			prtv(&ru.ru_stime, TRUE);
			prc_buff(NL);
		}
		break;

	case SYSEXIT:
		if (tried_to_exit++ || endjobs(JOB_STOPPED)) {
			flags |= forcexit;	/* force exit */
#ifdef	DO_SIGNED_EXIT
			exitsh(a1 ? stosi(a1) : retval);
#else
			exitsh(a1 ? stoi(a1) : retval);
#endif
		}
		break;

	case SYSNULL:
		break;

	case SYSCONT:
		if (loopcnt) {
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
		if (loopcnt) {
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
		oldsigs(TRUE);
		execa(argv, -1, FALSE);
		done(0);
#else

	case SYSNEWGRP:
		if (flags & rshflg)
			failed(argv[0], restricted);
		else if (!endjobs(JOB_STOPPED|JOB_RUNNING))
			break;
		else {
			flags |= forcexit; /* bad exec will terminate shell */
			oldsigs(TRUE);
			rmtemp(0);
			rmfunctmp();
#ifdef ACCT
			doacct();
#endif
			execa(argv, -1, FALSE);
			done(0);
			/* NOTREACHED */
		}

#endif

#ifdef	DO_SYSPUSHD
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
#endif	/* DO_SYSPUSHD */

		/* FALLTHROUGH */
	case SYSCD:
#ifdef	DO_SYSPUSHD
		/*
		 * Enable cd - & cd -- ... only with DO_SYSPUSHD
		 */
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
#endif
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
			    cf(a1, UC ".") == 0 ||
			    cf(a1, UC "..") == 0 ||
			    (*a1 == '.' && (*(a1+1) == '/' ||
			    (*(a1+1) == '.' && *(a1+2) == '/'))))
				cdpath = UC nullstr;

			do {
				dir = cdpath;
				cdpath = catpath(cdpath, a1);
			} while ((f = chdir((const char *) curstak())) < 0 &&
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
			} else {
				unsigned char	*wd;

				cwd(curstak());		/* Canonic from stak */
				wd = cwdget();		/* Get reliable cwd  */
#ifdef	DO_SYSPUSHD
				if (type != SYSPUSHD)
					free(pop_dir(0));
				push_dir(wd);		/* Update dir stack  */
				if (pr_dirs(1))		/* If already printed */
					wd = NULL;	/* don't do it again */
#endif
				if (cf(UC nullstr, dir) &&
				    *dir != ':' &&
				    any('/', curstak()) &&
				    flags & prompt) {
					if (wd) {	/* Not yet printed */
						prs_buff(wd);
						prc_buff(NL);
					}
				}
#ifdef	DO_SYSALIAS
				if (flags & localaliasflg) {
					ab_use(LOCAL_AB, (char *)localname);
				}
#endif
			}
			zapcd();
		} else {
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

			if ((dolc -= places) < 0) {
				dolc = 0;
				error(badshift);
			} else {
				dolv += places;
			}
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
		if (a1) {
			int	cnt;

			cnt = options(argc, argv);
			if (cnt > 1)
				setargs(argv + argc - cnt);
		} else if (comptr(t)->comset == 0) {
			/*
			 * scan name chain and print
			 */
			namscan(printnam);
		}
		break;

	case SYSRDONLY:
		if (a1) {
			while (*++argv)
				attrib(lookup(*argv), N_RDONLY);
		} else {
			namscan(printro);
		}
		break;

	case SYSXPORT:
		{
			struct namnod	*n;

			if (a1) {
				while (*++argv) {
					n = lookup(*argv);
					if (n->namflg & N_FUNCTN)
						error(badexport);
					else
						attrib(n, N_EXPORT);
				}
			} else {
				namscan(printexp);
			}
		}
		break;

	case SYSEVAL:
		if (a1)
			execexp(a1, (Intptr_t)&argv[2]);
		break;

#ifdef	DO_SYSDOSH
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
#endif

#ifdef	DO_SYSREPEAT
	case SYSREPEAT:
		if (a1) {
			extern int opterr, optind;
			int	savopterr;
			int	savoptind;
			int	savsp;
			char	*savoptarg;
			int	roptind;
			int	c;
			int	delay = 0;
			int	count = -1;
			int	err = 0;

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
					count = stoi(UC optarg);
					break;
				case 'd':
					delay = stoi(UC optarg);
					break;
				case '?':
					gfailure(UC usage, repuse);
					err = 1;
				}
			}
			roptind = optind;
			optind = savoptind;
			opterr = savopterr;
			_sp = savsp;
			optarg = savoptarg;
			if (err)
				break;

			while (count != 0) {
				unsigned char		*sav = savstak();
				struct ionod		*iosav = iotemp;

				execexp(argv[roptind],
					(Intptr_t)&argv[roptind+1]);
				tdystak(sav, iosav);

				if (delay > 0)
					sh_sleep(delay);
				if (count > 0)
					count--;
				sigchk();
			}
		} else {
			gfailure(UC usage, repuse);
		}
		break;
#endif

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

#ifdef	DO_SYSATEXPR
	case SYSEXPR:
		expr(argc, argv);
		break;
#endif

	case SYSECHO:
		exitval = echo(argc, argv);
		break;

	case SYSHASH:
		if (a1) {
			if (a1[0] == '-') {
				if (a1[1] == 'r')
					zaphash();
				else
					error(badopt);
			} else {
				while (*++argv) {
					if (hashtype(hash_cmd(*argv)) ==
							NOTFOUND) {
						failed(*argv, notfound);
					}
				}
			}
		} else {
			hashpr();
		}
		break;

#ifdef	DO_SYSPUSHD
	case SYSDIRS:
		pr_dirs(0);
		break;
#endif

	case SYSPWD:
		cwdprint();
		break;

	case SYSRETURN:
		if (funcnt == 0)
			error(badreturn);

		execbrk = 1;
		exitval = (a1 ? stoi(a1) : retval);
		break;

	case SYSTYPE:
		if (a1) {
			/* return success only if all names are found */
			while (*++argv) {
#ifdef	DO_SYSALIAS
				char *val;

				if ((val = ab_value(LOCAL_AB,
							(char *)*argv, NULL,
							AB_BEGIN)) != NULL) {
					prs_buff(*argv);
					prs_buff(_gettext(
						" is a local alias for '"));
					prs_buff(UC val);
					prs_buff(UC "'\n");
					continue;
				} else if ((val = ab_value(GLOBAL_AB,
							(char *)*argv, NULL,
							AB_BEGIN)) != NULL) {
					prs_buff(*argv);
					prs_buff(_gettext(
						" is a global alias for '"));
					prs_buff(UC val);
					prs_buff(UC "'\n");
					continue;
				}
#endif
				exitval |= what_is_path(*argv);
			}
		}
		break;

	case SYSUNS:
		if (a1) {
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
		n = lookup(UC "OPTIND");
		optind = stoi(n->namval);
		if (argc > 3) {
			argv[2] = dolv[0];
			getoptval = getopt(argc-2,
					(char **)&argv[2], (char *)argv[1]);
		} else {
			getoptval = getopt(dolc+1,
					(char **)dolv, (char *)argv[1]);
		}
		if (getoptval == -1) {
			itos(optind);
			assign(n, numbuf);
			n = lookup(varnam);
			assign(n, UC nullstr);
			exitval = 1;
			break;
		}
		argv[2] = varnam;
		itos(optind);
		assign(n, numbuf);
		c[0] = (char)getoptval;
		c[1] = '\0';
		n = lookup(varnam);
		assign(n, c);
		n = lookup(UC "OPTARG");
		assign(n, UC optarg);
		}
		break;

#ifdef	INTERACTIVE
	case SYSHISTORY:
		shedit_bhist();
		break;

	case SYSSAVEHIST:
		shedit_bshist(&intrptr);
		if (intrptr)		/* Was set by shedit_bshist()?	*/
			intrptr = 0;	/* Disable intrptr for now	*/
		break;

	case SYSMAP:
		{
			int	f = 1;

			if (argc == 1) {
				shedit_list_map(&f);
			} else if (argc == 2 && eq(argv[1], "-r")) {
				shedit_remap();
			} else if (argc == 3 && eq(argv[1], "-u")) {
				shedit_del_map((char *)argv[2]);
			} else if (argc == 3 || argc == 4) {
				if (!shedit_add_map((char *)argv[1],
						(char *)argv[2],
						(char *)argv[3])) {
					prs(argv[1]);
					prs(UC ": ");
					prs(UC "already defined\n");
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

#ifdef	DO_SYSALIAS
	case SYSALIAS:
		sysalias(argc, argv);
		break;
	case SYSUNALIAS:
		sysunalias(argc, argv);
		break;
#endif

#ifdef	DO_SYSTRUE
	case SYSTRUE:
		break;
	case SYSFALSE:
		exitval = 1;
		if (flags & errflg)
			done(0);
		break;
#endif

#ifdef	DO_SYSBUILTIN
	case SYSBUILTIN:
		sysbuiltin(argc, argv);
		break;
#endif

#ifdef	DO_SYSFIND
	case SYSFIND:
		sysfind(argc, argv);
		break;
#endif

#ifdef	DO_SYSSYNC
	case SYSSYNC:
#ifdef	HAVE_SYNC
		sync();
#else
		failure(*argv, unimplemented);
#endif
		break;
#endif

#ifdef	DO_SYSPGRP
	case SYSPGRP:
		syspgrp(argc, (char **)argv);
		break;
#endif

#ifdef	DO_SYSERRSTR
	case SYSERRSTR: {
			int	err;
			char	*msg;
#ifndef	HAVE_STRERROR
#define	strerror(a)	errmsgstr(a)
#endif

			if (a1 && (err = stoi(a1)) > 0) {
				errno = 0;
				msg = strerror(err);
				if (errno == 0 && msg) {
					prs_buff(UC msg);
					prc_buff(NL);
				}
			}
		}
		break;
#endif

	default:
		prs_buff(_gettext("unknown builtin\n"));
	}

	flushb();		/* Flush print buffer */
	restore(fdindex);	/* Restore file descriptors */
	exval_set(exitval);	/* Prepare ${.sh.*} parameters */
	chktrap();		/* Run installed traps */
}
