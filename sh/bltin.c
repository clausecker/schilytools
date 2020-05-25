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
 * Copyright 2008-2020 J. Schilling
 *
 * @(#)bltin.c	1.147 20/05/17 2008-2020 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)bltin.c	1.147 20/05/17 2008-2020 J. Schilling";
#endif

/*
 *
 * UNIX shell
 *
 */

#if	defined(INTERACTIVE) || defined(DO_SYSFC)
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
#include	<schily/wait.h>		/* Needed for CLD_EXITED */

	void	builtin	__PR((int type, int argc, unsigned char **argv,
					struct trenod *t, int xflags));
#ifdef	DO_POSIX_CD
static	int	opt_LP	__PR((int argc, unsigned char **argv,
					int *opts, const char *use));
#endif
static	int	whatis	__PR((unsigned char *arg, int verbose));
#ifdef	DO_SYSCOMMAND
static	void	syscommand __PR((int argc, unsigned char **argv,
					struct trenod *t, int xflags));
#endif
#if	defined(INTERACTIVE) || defined(DO_SYSFC)
static	void	syshist	__PR((int argc, unsigned char **argv,
					struct trenod *t, int xflags));
#endif
#ifdef	DO_TILDE
static unsigned char *etilde	__PR((unsigned char *arg, unsigned char *s));
#endif

#define	no_pipe	(int *)0

void
builtin(type, argc, argv, t, xflags)
	int		type;
	int		argc;
	unsigned char	**argv;
	struct trenod	*t;
	int		xflags;
{
	short		fdindex;
	unsigned char	*a0 = NULL;
	unsigned char	*a1 = argv[1];
	struct argnod	*np = NULL;
	int		cdopt = 0;
#if defined(DO_POSIX_CD) || defined(DO_SYSPUSHD) || defined(DO_SYSDOSH) || \
	defined(DO_GETOPT_UTILS) || defined(DO_SYSERRSTR)
	int		ind = 1;
#endif
#ifdef	DO_POSIX_FAILURE
	unsigned long	oflags = flags;
#endif

	exitval = 0;
	exval_clear();
#ifdef	DO_POSIX_FAILURE
	if ((type & SPC_BUILTIN) == 0)
		flags |= noexit;
#endif
	type = hashdata(type);
	fdindex = initio(t->treio, (type != SYSEXEC));
#ifdef	DO_POSIX_FAILURE
	flags = oflags;
	if (exitval)
		goto out;
#endif

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
		sysjobs(argc, argv);
		break;

	case SYSDOT:			/* POSIX special builtin */
		if (a1) {
			int	f;

			if ((f = pathopen(getpath(a1), a1)) < 0)
				failed(a1, notfound);
			else {
#ifdef	DO_POSIX_RETURN
				jmps_t	dotjmp;
				jmps_t	*odotjmp = dotshell;
				struct fileblk	*ostandin = standin;

				if (setjmp(dotjmp.jb)) {
					/*
					 * pushed in execexp()
					 */
					while (ostandin != standin) {
						if (!pop())
							break;
					}
					dotshell = odotjmp;
					dotcnt--;
					break;
				}
				dotshell = &dotjmp;
				dotcnt++;
#endif

				execexp(0, (Intptr_t)f, xflags);

#ifdef	DO_POSIX_RETURN
				dotcnt--;
				dotshell = odotjmp;
#endif
			}
		}
		break;

	case SYSTIMES:			/* POSIX special builtin */
		{
			struct rusage	ru;

			getrusage(RUSAGE_SELF, &ru);
			prtv(&ru.ru_utime, 3, 'l');
			prc_buff(SPACE);
			prtv(&ru.ru_stime, 3, 'l');
			prc_buff(NL);
			getrusage(RUSAGE_CHILDREN, &ru);
			prtv(&ru.ru_utime, 3, 'l');
			prc_buff(SPACE);
			prtv(&ru.ru_stime, 3, 'l');
			prc_buff(NL);
		}
		break;

	case SYSEXIT:			/* POSIX special builtin */
		if (tried_to_exit++ || endjobs(JOB_STOPPED)) {
#ifdef	INTERACTIVE
			if (!(xflags & XEC_EXECED))
				shedit_treset();	/* Writes ~/.history */
#endif
			flags |= forcexit;	/* force exit */
#ifdef	DO_SIGNED_EXIT
			exitsh(a1 ? stosi(a1) : retval);
#else
			exitsh(a1 ? stoi(a1) : retval);
#endif
		}
		break;

	case SYSNULL:			/* POSIX special builtin */
		break;

	case SYSCONT:			/* POSIX special builtin */
		if (loopcnt) {
			execbrk = breakcnt = 1;
			if (a1) {
				breakcnt = stoi(a1);
#ifdef	DO_CONT_BRK_POSIX
				if (breakcnt == 0) {
					exitval = ERROR;
					break;
				}
#endif
			}
			if (breakcnt > loopcnt)
				breakcnt = loopcnt;
#ifndef	DO_CONT_BRK_FIX
			else
#endif
			breakcnt = -breakcnt;
		}
		break;

	case SYSBREAK:			/* POSIX special builtin */
		if (loopcnt) {
			execbrk = breakcnt = 1;
			if (a1) {
				breakcnt = stoi(a1);
#ifdef	DO_CONT_BRK_POSIX
				if (breakcnt == 0) {
					exitval = ERROR;
					break;
				}
#endif
			}
			if (breakcnt > loopcnt)
				breakcnt = loopcnt;
		}
		break;

	case SYSTRAP:			/* POSIX special builtin */
		systrap(argc, (char **)argv);
		break;

	case SYSEXEC:			/* POSIX special builtin */
		argv++;
		ioset = 0;
		if (a1 == 0) {
			setmode(0);
			break;
		}
#ifdef	DO_EXEC_AC
		if (eq(a1, "-a")) {
			argv++;
			if (*argv) {
				a0 = *argv++;
			}
		}
#endif
		/* FALLTHROUGH */

#ifdef RES	/* Research includes login as part of the shell */

	case SYSLOGIN:
		if (!endjobs(JOB_STOPPED|JOB_RUNNING))
			break;
		oldsigs(TRUE);
		execa(argv, -1, FALSE, a0);
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
			rmfunctmp(0);
#ifdef ACCT
			doacct();
#endif
			execa(argv, -1, FALSE, a0);
			done(0);
			/* NOTREACHED */
		}

#endif

#ifdef	DO_SYSPUSHD
	case SYSPOPD:
		/* FALLTHROUGH */
	case SYSPUSHD:
#ifdef	DO_POSIX_CD
		cdopt = -1;
		ind = opt_LP(argc, argv, &cdopt,
			type == SYSPOPD ?
				"popd [ -L | -P ] [-offset]":
				"pushd [ -L | -P ] [-offset | directory]");
		if (ind < 0)
			break;
		a1 = argv[ind];
#endif
		init_dirs();
		if (a1 && a1[0] == '-') {
			if (type == SYSPUSHD && a1[1] == '\0') {
				extern struct namnod opwdnod;

				a1 = opwdnod.namval;
				if (a1 == NULL || *a1 == '\0') {
					free(np);
					failure(a1, baddir);
					break;
				}
			} else {
				int	off = stoi(&a1[1]);

				if (!(np = pop_dir(off))) {
					failure(a1, badoff);
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
				/*
				 * "dnp" is the static "dirs",
				 * no need to free()
				 */
				gfailure(UC emptystack, NULL);
				break;
			}
			a1 = dnp->argnxt->argval;
			free(dnp);
		}
#endif	/* DO_SYSPUSHD */

		/* FALLTHROUGH */
	case SYSCD:
#ifdef	DO_POSIX_CD
		if (type == SYSCD) {
			ind = opt_LP(argc, argv, &cdopt,
						"cd [ -L | -P ] directory");
			if (ind < 0)
				break;
			a1 = argv[ind];
		}

		/*
		 * Enable cd - & cd -- ... only with DO_POSIX_CD
		 */
		if (type == SYSCD && a1 && a1[0] == '-') {
			if (a1[1] == '\0') {
				extern struct namnod opwdnod;

				a1 = opwdnod.namval;
				if (a1 == NULL || *a1 == '\0') {
					free(np);
					failure(a1, baddir);
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
#ifdef	DO_POSIX_CD
				if ((cdopt & CHDIR_L) && *curstak() != '/') {
					/*
					 * Concatenate $PWD and curstak() and
					 * normalize the resulting path.
					 */
					if (!cwdrel2abs()) {
						Failure(a1, baddir);
						goto out;
					}
				}
#endif
			} while ((f = lchdir((char *) curstak())) < 0 &&
			    cdpath);

			free(np);
			if (f < 0) {
				switch (errno) {
#ifdef	EMULTIHOP
				case EMULTIHOP:
					Failure(a1, emultihop);
					break;
#endif
				case ENOTDIR:
					Failure(a1, enotdir);
					break;
				case ENOENT:
					Failure(a1, enoent);
					break;
				case EACCES:
					Failure(a1, eacces);
					break;
#ifdef	ENOLINK
				case ENOLINK:
					Failure(a1, enolink);
					break;
#endif
				default:
					Failure(a1, baddir);
					break;
				}
				break;	/* No zapcd(), chdir() did not work */
			} else {
				unsigned char	*wd;

				ocwdnod();		/* Update OLDPWD=    */
				cwd(curstak(), NULL);	/* Canonic from stak */
				wd = cwdget(cdopt);	/* Get reliable cwd  */
#ifdef	DO_SYSPUSHD
				if (type != SYSPUSHD)
					free(pop_dir(0));
				push_dir(wd);		/* Update dir stack  */
				if (pr_dirs(type ==	/* Print if len > 0  */
				    SYSPOPD ?		/* or cmd was "popd" */
				    0:1, cdopt))	/* If already printed */
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
				if (flags2 & localaliasflg) {
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
				Error(nulldir);
			else
				Error(nohome);
		}
		break;

	case SYSSHFT:			/* POSIX special builtin */
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
#ifndef	DO_READ_R
		if (argc < 2) {
			Failure(argv[0], mssgargn);
			break;
		}
#endif
		rwait = 1;
		exitval = readvar(argc, argv);
		rwait = 0;
		break;

	case SYSSET:			/* POSIX special builtin */
		if (a1) {
			int	cnt;

			cnt = options(argc, argv);
#ifdef	DO_POSIX_SET
			if (cnt > 1 || dashdash)
#else
			if (cnt > 1)
#endif
				setargs(argv + argc - cnt);
		} else if (comptr(t)->comset == 0) {
			/*
			 * scan name chain and print
			 */
			namscan(printnam);
		}
		break;

#ifdef	DO_SYSLOCAL
	case SYSLOCAL:
		{
			ind = optskip(argc, argv, "local [name[=value] ...]");
			if (ind-- < 0)
				break;
			argv += ind;

			if (localp == NULL)
				error("local can only be used in a function");

			if (argv[1]) {
				while (*++argv) {
					unsigned char *p;

					p = UC strchr((char *)*argv, '=');
					if (p)
						*p = '\0';
					pushval(lookup(*argv), localp);
					if (p) {
						*p = '=';
#ifdef	DO_TILDE
						if (strchr(C p, '~') != NULL)
							p = etilde(*argv, p);
						else
#endif
							p = *argv;
						setname(p, 0);
					}
					localcnt++;
				}
			} else {
				namscan(printlocal);
			}
		}
		break;
#endif	/* DO_SYSLOCAL */

	case SYSRDONLY:			/* POSIX special builtin */
		{
#ifdef	DO_POSIX_EXPORT
			struct optv	optv;
			int		c;
			int		isp = 0;

			optinit(&optv);
			optv.optflag |= OPT_SPC;

			while ((c = optnext(argc, argv, &optv, "p",
				    "readonly [-p] [name[=value] ...]")) !=
									-1) {
				if (c == 0)	/* Was -help */
					goto out;
				else if (c == 'p')
					isp++;
			}
			argv += --optv.optind;
#endif
			if (argv[1]) {
				while (*++argv) {
#ifdef	DO_POSIX_EXPORT
					unsigned char	*p;

					p = UC strchr((char *)*argv, '=');
					if (p) {
#ifdef	DO_TILDE
						if (strchr(C p, '~') != NULL)
							p = etilde(*argv, p);
						else
#endif
							p = *argv;
						setname(p, N_RDONLY);
						continue;
					}
#endif
					attrib(lookup(*argv), N_RDONLY);
				}
			} else {
#ifdef	DO_POSIX_EXPORT
				namscan(isp?printpro:printro);
#else
				namscan(printro);
#endif
			}
		}
		break;

	case SYSXPORT:			/* POSIX special builtin */
		{
			struct namnod	*n;
#ifdef	DO_POSIX_EXPORT
			struct optv	optv;
			int		c;
			int		isp = 0;

			optinit(&optv);
			optv.optflag |= OPT_SPC;

			while ((c = optnext(argc, argv, &optv, "p",
				    "export [-p] [name[=value] ...]")) != -1) {
				if (c == 0)	/* Was -help */
					goto out;
				else if (c == 'p')
					isp++;
			}
			argv += --optv.optind;
#endif
			if (argv[1]) {
				while (*++argv) {
#ifdef	DO_POSIX_EXPORT
					unsigned char	*p;

					p = UC strchr((char *)*argv, '=');
					if (p) {
#ifdef	DO_TILDE
						if (strchr(C p, '~') != NULL)
							p = etilde(*argv, p);
						else
#endif
							p = *argv;
						setname(p, N_EXPORT);
						continue;
					}
#endif
					n = lookup(*argv);
#ifndef	DO_POSIX_UNSET
					if (n->namflg & N_FUNCTN)
						error(badexport);
					else
#endif
						attrib(n, N_EXPORT);
				}
			} else {
#ifdef	DO_POSIX_EXPORT
				namscan(isp?printpexp:printexp);
#else
				namscan(printexp);
#endif
			}
		}
		break;

	case SYSEVAL:			/* POSIX special builtin */
		if (a1) {
			flags &= ~noexit;
			execexp(a1, (Intptr_t)&argv[2], xflags);
		}
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

			ind = optskip(argc, argv,
					    "dosh command [commandname args]");
			if (ind < 0)
				break;
			if (ind >= argc)
				break;

			/*
			 * save current positional parameters
			 */
			olddolh = (struct dolnod *)savargs(funcnt);
			funcnt++;
			setargs(&argv[ind+1]);
			idx = initio(io, 1);
			execexp(argv[ind], (Intptr_t)0, xflags);
			restore(idx);
			(void) restorargs(olddolh, funcnt);
			dolv = olddolv;
			dolc = olddolc;
			funcnt--;
		}
		break;
#endif	/* DO_SYSDOSH */

#ifdef	DO_SYSREPEAT
	case SYSREPEAT:
		if (a1) {
			struct optv optv;
			int	c;
			int	delay = 0;
			int	count = -1;
			int	err = 0;

			optinit(&optv);

			while ((c = optget(argc, argv, &optv,
						"c:(count)d:(delay)")) != -1) {
				switch (c) {
				case 'c':
					count = stoi(UC optv.optarg);
					break;
				case 'd':
					delay = stoi(UC optv.optarg);
					break;
				case '?':
					gfailure(UC usage, repuse);
					err = 1;
					goto reperr;
				}
			}
		reperr:
			if (err)
				break;

			while (count != 0) {
				unsigned char		*sav = savstak();
				struct ionod		*iosav = iotemp;

				execexp(argv[optv.optind],
					(Intptr_t)&argv[optv.optind+1], xflags);
				tdystak(sav, iosav);

				if (delay > 0)
					sh_sleep(delay);
				if (count > 0)
					count--;
				/*
				 * Exit if this shell received a signal
				 */
				sigchk();
				/*
				 * Exit if child received a signal
				 */
				if ((ex.ex_code != 0 &&
				    ex.ex_code != CLD_EXITED) ||
				    ex.ex_status != 0) {
					exitsh(exitval ? exitval : SIGFAIL);
				}
			}
		} else {
			gfailure(UC usage, repuse);
		}
		break;
#endif	/* DO_SYSREPEAT */

#ifndef RES
	case SYSULIMIT:
		sysulimit(argc, argv);
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
		{
#ifdef	DO_GETOPT_UTILS		/* For all builtins that support -- */
			struct optv	optv;
			int		c;

			optinit(&optv);

			while ((c = optnext(argc, argv, &optv, "r",
				    "hash [-r] [name ...]")) != -1) {
				if (c == 0)	/* Was -help */
					goto out;
				else if (c == 'r') {
					zaphash();
					goto out;
				}
			}
			argv += --optv.optind;
#else
			if (a1 && a1[0] == '-') {
				if (a1[1] == 'r')
					zaphash();
				else
					Error(badopt);
				break;
			}
#endif
			if (argv[1]) {
				while (*++argv) {
					if (hashtype(hash_cmd(*argv)) ==
						NOTFOUND) {
						Failure(*argv, notfound);
					}
				}
			} else {
				hashpr();
			}
		}
		break;

#ifdef	DO_SYSPUSHD
	case SYSDIRS:
		ind = opt_LP(argc, argv, &cdopt, "dirs [ -L | -P ]");
		if (ind < 0)
			break;
		pr_dirs(0, cdopt);
		break;
#endif

	case SYSPWD:
		{
#ifdef	DO_POSIX_CD
			ind = opt_LP(argc, argv, &cdopt, "pwd [ -L | -P ]");
			if (ind < 0)
				break;
#endif
			cwdprint(cdopt);
		}
		break;

	case SYSRETURN:			/* POSIX special builtin */
		if (funcnt == 0 && dotcnt == 0)
			error(badreturn);

		if (dotcnt > 0)
			dotbrk = 1;
		else
			execbrk = 1;
		exitval = (a1 ? stoi(a1) : retval);
		break;

	case SYSTYPE:
		if (a1) {
#ifdef	DO_GETOPT_UTILS		/* For all builtins that support -- */
			struct optv	optv;
			int		c;

			optinit(&optv);
			while ((c = optnext(argc, argv, &optv, "F",
					"type [-F] [name ...]")) != -1) {
				if (c == 0)	/* Was -help */
					goto out;
				else if (c == 'F') {
					if (argv[optv.optind] == NULL)
						namscan(printfunc);
					else
						failure(argv[0], toomanyargs);
					goto out;
				}
			}
			argv += --optv.optind;
#endif
			/* return success only if all names are found */
			while (*++argv) {
				exitval |= whatis(*argv, 2);
			}
		}
		break;

	case SYSUNS:			/* POSIX special builtin */
		if (a1) {
			int	uflg = 0;
#ifdef	DO_POSIX_UNSET
			struct optv	optv;
			int		c;

			optinit(&optv);
			optv.optflag |= OPT_SPC;

			while ((c = optnext(argc, argv, &optv, "fv",
					"unset [-f | -v] [name ...]")) != -1) {
				if (c == 0)	/* Was -help */
					goto out;
				else if (c == 'f')
					uflg = UNSET_FUNC;
				else if (c == 'v')
					uflg = UNSET_VAR;
			}
			argv += --optv.optind;
#endif
			while (*++argv)
				unset_name(*argv, uflg);
		}
		break;

	case SYSGETOPT: {
		int getoptval;
		struct namnod *n;
		extern unsigned char numbuf[];
		unsigned char *varnam;
		unsigned char c[3];	/* '+', 'c' and '\0' */
		unsigned char *cptr;	/* points to + or to c */
		unsigned char *cmdp = *argv;
		unsigned char *optstring;

#ifdef	DO_GETOPT_UTILS		/* For all builtins that support -- */
		ind = optskip(argc, argv,
					"getopts optstring name [arg ...]");
		if (ind-- < 0)
			break;
		argc -= ind;
		argv += ind;
#endif
		if (argc < 3) {
			failure(cmdp, mssgargn);
			break;
		}
		exitval = 0;
		n = lookup(UC "OPTIND");
		optind = stoi(n->namval);
		if (optind <= 0)		/* Paranoia */
			optind = 1;
		varnam = argv[2];
		optstring = argv[1];
#ifndef	DO_GETOPT_PLUS
		if (*optstring == '+')
			optstring++;
#endif
		if (argc > 3) {
			argv[2] = dolv[0];
			getoptval = getopt(argc-2,
					(char **)&argv[2], (char *)optstring);
			argv[2] = varnam;
		} else {
			getoptval = getopt(dolc+1,
					(char **)dolv, (char *)optstring);
		}
		if (getoptval == -1) {
			itos(optind);
			assign(n, numbuf);
			n = lookup(varnam);
#ifdef	DO_GETOPT_POSIX
			assign(n, UC "?");
#else
			assign(n, UC nullstr);
#endif
			exitval = 1;
			break;
		}
		itos(optind);
		assign(n, numbuf);
		c[0] = '+';
		c[1] = (char)getoptval;
		c[2] = '\0';
		cptr = &c[1];
#ifdef	DO_GETOPT_PLUS
		if (optflg & GETOPT_PLUS_FL)
			cptr = c;
#endif
		n = lookup(varnam);
		if (getoptval > 256) {
			/*
			 * We should come here only with the Schily enhanced
			 * getopt() from libgetopt.
			 */
#ifdef	DO_GETOPT_LONGONLY
			itos(getoptval);
#ifdef	DO_GETOPT_PLUS
			if (optflg & GETOPT_PLUS_FL) {
				unsigned char pnumbuf[64];
				strcpy(C pnumbuf, "+");
				strcat(C pnumbuf, C numbuf);
				assign(n, pnumbuf);
			} else
#endif	/* DO_GETOPT_PLUS */
				assign(n, numbuf);
#else
			assign(n, UC "?");	/* Pretend illegal option */
#endif
		} else
			assign(n, cptr);
		n = lookup(UC "OPTARG");
		assign(n, UC optarg);
#ifdef	DO_GETOPT_POSIX
		/*
		 * In case that the option string starts with ':',
		 * POSIX requires to set OPTARG to the option
		 * character that causes getopt() to return '?'/':'.
		 */
		if (optarg == NULL && argv[1][0] == ':' &&
		    (getoptval == '?' || getoptval == ':')) {
			c[1] = optopt;
			assign(n, &c[1]);
		}
#endif
		}
		break;

#ifdef	INTERACTIVE
	case SYSHISTORY:
		syshist(argc, argv, t, xflags);
		if (intrptr) {		/* Was set by syshist()?	*/
			*intrptr = 0;	/* Clear interrupt counter	*/
			intrptr = 0;	/* Disable intrptr for now	*/
		}
		break;

	case SYSSAVEHIST:
		shedit_bshist(&intrptr);

		if (intrptr) {		/* Was set by shedit_bshist()?	*/
			*intrptr = 0;	/* Clear interrupt counter	*/
			intrptr = 0;	/* Disable intrptr for now	*/
		}
		break;

	case SYSMAP:
		{
			int		f = STDOUT_FILENO;
			struct optv	optv;
			int		c;
			int		rfl = 0;
			int		ufl = 0;
			unsigned char	*cmdp = *argv;

			optinit(&optv);

			while ((c = optnext(argc, argv, &optv, "ru",
			    "map [-r | -u] [fromstr [tostr [comment]]]")) !=
			    -1) {
				if (c == 0)	/* Was -help */
					goto out;
				else if (c == 'r')
					rfl++;
				else if (c == 'u')
					ufl++;
			}
			argv += --optv.optind;
			argc -= optv.optind;

			if (argc == 1 && (rfl+ufl) == 0) {
				shedit_list_map(&f);
			} else if (argc == 1 && rfl) {
				shedit_remap();
			} else if (argc == 2 && ufl) {
				shedit_del_map((char *)argv[1]);
			} else if (argc == 3 || argc == 4) {
				/*
				 * argv[1] is map from
				 * argv[2] is map to
				 * argv[3] is the optional comment
				 */
				if (!shedit_add_map((char *)argv[1],
						(char *)argv[2],
						(char *)argv[3])) {
					prs(cmdp);
					prs(UC ": ");
					prs(UC "already defined\n");
					gfailure(UC "bad map", NULL);
				}
			} else if (argc > 4) {
				gfailure(UC arglist, NULL);
			} else {
				gfailure(UC mssgargn, NULL);
			}
		}
		break;
#endif	/* INTERACTIVE */

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
		break;
#endif

#ifdef	DO_SYSBUILTIN
	case SYSBUILTIN:
		sysbuiltin(argc, argv);
		break;
#endif

#ifdef	HAVE_LOADABLE_LIBS
	case SYSLOADABLE: {
		int		exval;
		struct sysnod2	*s2;

		s2 = sh_findbuiltin(*argv);
		if (s2 == NULL) {
			failure(*argv, notfound);
			break;
		}
		exval = (*s2->sysptr)(argc, argv, &bosh);
		if (exval)
			exitval = exval;
	}
		break;
#endif

#ifdef	DO_SYSFIND
	case SYSFIND: {
		int	exval = sysfind(argc, argv, &bosh);

		if (exval)
			exitval = exval;
	}
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

			ind = optskip(argc, argv, "errstr errno");
#ifndef	HAVE_STRERROR
#define	strerror(a)	errmsgstr(a)
#endif
			if (ind < 0)
				break;

			a1 = argv[ind];

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

#ifdef	DO_SYSPRINTF
	case SYSPRINTF:
		sysprintf(argc, argv);
		break;
#endif

#ifdef	DO_SYSCOMMAND
	case SYSCOMMAND:
		syscommand(argc, argv, t, xflags);
		break;
#endif

#ifdef	DO_SYSFC
	case SYSFC:
		syshist(argc, argv, t, xflags);
		if (intrptr) {		/* Was set by syshist()?	*/
			*intrptr = 0;	/* Clear interrupt counter	*/
			intrptr = 0;	/* Disable intrptr for now	*/
		}
		break;
#endif

	default:
		prs_buff(_gettext("unknown builtin\n"));
	}
#if	defined(DO_POSIX_EXPORT) || defined(DO_POSIX_UNSET) || \
	defined(DO_GETOPT_UTILS) || defined(INTERACTIVE) || \
	defined(DO_POSIX_CD) || defined(DO_POSIX_FAILURE)
out:
#endif
	flushb();		/* Flush print buffer */
	restore(fdindex);	/* Restore file descriptors */
	exval_set(exitval);	/* Prepare ${.sh.*} parameters */
#ifdef	DO_ERR_TRAP
	if ((trapnote & TRAPSET) ||
	    (exitval &&
	    (xflags & XEC_NOSTOP) == 0))
#endif
		chktrap();	/* Run installed traps */
}

#ifdef	DO_POSIX_CD
static int
opt_LP(argc, argv, opts, use)
	int		argc;
	unsigned char	**argv;
	int		*opts;
	const	 char	*use;
{
	struct optv	optv;
	int		c;
	int		opt = *opts;

	optinit(&optv);
	if (opt < 0) {
		optv.optflag |= OPT_NOFAIL;
		*opts = 0;
	}
	while ((c = optnext(argc, argv, &optv, "LP", use)) != -1) {
		if (c == 0) {	/* Was -help or bad opt */
			return (-1);
		} else if (c == '?') {
			if (opt < 0) {
				/*
				 * If *opts has been preinitialzed with -1,
				 * accept e.g. -2 as a a pushd offset arg.
				 */
				c = argv[optv.ooptind][1];
				if (c >= '0' && c <= '9')
					return (optv.ooptind);
				optbad(argc, argv, &optv);
				gfailure((unsigned char *)usage, use);
				return (-1);
			}
		} else if (c == 'L') {
			*opts = CHDIR_L;
		} else if (c == 'P') {
			*opts = CHDIR_P;
		}
	}

	/*
	 * POSIX decided in 1992 to introduce an incompatible default for "cd".
	 * Even though this is incompatible with the Bourne Shell behavior, it
	 * may be too late to go back.
	 */
	if ((flags2 & posixflg) && *opts == 0)
		*opts = CHDIR_L;

	return (optv.optind);
}
#endif	/* DO_POSIX_CD */

static int
whatis(arg, verbose)
	unsigned char	*arg;
	int		verbose;
{
#ifdef	DO_SYSALIAS
	char		*val;

	if ((val = ab_value(LOCAL_AB, (char *)arg, NULL,
				AB_BEGIN)) != NULL) {
		if (verbose) {
			prs_buff(arg);
			prs_buff(_gettext(" is a local alias for '"));
		}
		prs_buff(UC val);
		if (verbose)
			prs_buff(UC "'\n");
		else
			prs_buff(UC "\n");
		return (0);
	} else if ((val = ab_value(GLOBAL_AB, (char *)arg, NULL,
				AB_BEGIN)) != NULL) {
		if (verbose) {
			prs_buff(arg);
			prs_buff(_gettext(" is a global alias for '"));
		}
		prs_buff(UC val);
		if (verbose)
			prs_buff(UC "'\n");
		else
			prs_buff(UC "\n");
		return (0);
	}
#endif	/* DO_SYSALIAS */
	return (what_is_path(arg, verbose));
}

#ifdef	DO_SYSCOMMAND
static void
syscommand(argc, argv, t, xflags)
	int		argc;
	unsigned char	**argv;
	struct trenod	*t;
	int		xflags;
{
	struct optv	optv;
	int		c;
	int		pflg = 0;
	int		vflg = 0;
	int		Vflg = 0;
	char		*cusage = "command [-p][-v | -V] [name [arg ...]]";

	optinit(&optv);

	while ((c = optnext(argc, argv, &optv, "pvV", cusage)) != -1) {
		if (c == 0) {	/* Was -help */
			return;
		} else if (c == 'p') {
			pflg++;
		} else if (c == 'v') {
			if (Vflg) {
				optbad(argc, UCP argv, &optv);
				gfailure((unsigned char *)usage, cusage);
				return;
			}
			vflg++;
		} else if (c == 'V') {
			if (vflg) {
				optbad(argc, UCP argv, &optv);
				gfailure((unsigned char *)usage, cusage);
				return;
			}
			Vflg++;
		}
	}
	argc -= optv.optind;
	argv += optv.optind;
	if ((vflg + Vflg) && *argv == NULL) {
		gfailure((unsigned char *)usage, cusage);
		return;
	}
	if (pflg) {
		flags |= ppath;
	}
	if (vflg) {
		exitval = whatis(*argv, 0);
	} else if (Vflg) {
		exitval = whatis(*argv, 1);
	} else if (*argv) {
		short		cmdhash;
		unsigned long	oflags = flags;

		/*
		 * A function may overlay a builtin command.
		 * We therefore do not search for functions.
		 */
		flags |= nofuncs;
		cmdhash = pathlook(argv[0], 0, (struct argnod *)0);
		flags = oflags;

		if (hashtype(cmdhash) == BUILTIN) {
			struct ionod	*iop = t->treio;

			if (cmdhash & SPC_BUILTIN)
				flags |= noexit;
			t->treio = NULL;
			builtin(cmdhash, argc, argv, t, xflags);
			t->treio = iop;
			flags &= ~(ppath | noexit);
			return;
		} else {
			unsigned char		*sav = savstak();
			struct ionod		*iosav = iotemp;
			struct comnod	cnod;

			cnod.comtyp = TCOM;
			cnod.comio  = NULL;
			cnod.comarg = (struct argnod *)argv;
			cnod.comset = comptr(t)->comset;

			flags |= nofuncs;
			execute((struct trenod *)&cnod, xflags | XEC_HASARGV,
				0, no_pipe, no_pipe);
			flags &= ~nofuncs;
			tdystak(sav, iosav);
		}
	}
	flags &= ~ppath;
}
#endif	/* DO_SYSCOMMAND */

#if	defined(INTERACTIVE) || defined(DO_SYSFC)
#define	LFLAG	1	/* Do listing		*/
#define	SFLAG	2	/* Re-execute		*/
#define	EFLAG	4	/* Edit and Re-execute	*/
#define	ALLFLAG	1024	/* Show full history	*/
/*
 * The implementation for the "history" and the "fc" builtin command.
 *
 * The "fc" builtin is an historic artefact from ksh.
 * It is based on the original ksh history concept that has never been used in
 * our history editor that is rather based on the history editor in "bsh", the
 * shell from H.Berthold AG that had a fully integrated history editor in 1984
 * already; based on a concept from 1982. Ksh originally called an external
 * editor command to modify the history.
 *
 * As "fc" is part of the POSIX standard, we have to implement it even though
 * the features required by POSIX do not match the features from a modern
 * history editor, so we try to do the best we can...
 *
 * The "history" implementation currently just disables the re-run and edit
 * features.
 */
static void
syshist(argc, argv, t, xflags)
	int		argc;
	unsigned char	**argv;
	struct trenod	*t;
	int		xflags;
{
	struct optv	optv;
	int		c;
	int		fd;
	int		first = 0;
	int		last = 0;
	int		flg = 0;
	int		hflg = HI_INTR|HI_TAB;
	unsigned char	*av0 = argv[0];
	unsigned char	*cp;
	unsigned char	*editor = NULL;
	unsigned char	*substp = NULL;
	struct tempblk	tb;
	char		*fusage =
			"fc [-r][-l][-n][-s][-e editor] [first [last]]";
	char		*husage =
			"history [-r][-n] [first [last]]";
	char		*xusage = fusage;

	if (*av0 == 'h') {
		xusage = husage;
		flg |= LFLAG;
	}

	optinit(&optv);

	while ((c = optnext(argc, argv, &optv,
				"rlnse:1234567890", xusage)) != -1) {
		if (c == 0) {	/* Was -help */
			return;
		} else if (c == 'r') {
			hflg |= HI_REVERSE;
		} else if (c == 'l') {
			flg |= LFLAG;
		} else if (c == 'n') {
			hflg |= HI_NONUM;
		} else if (c == 's') {
			flg |= SFLAG;
		} else if (c == 'e') {
			editor = UC optv.optarg;
		} else if (digit(c)) {
			first = stosi(argv[optv.ooptind]);
			argc--;
			argv++;
			break;
		}
	}
	argc -= optv.optind;
	argv += optv.optind;

	if ((flg & (LFLAG | SFLAG)) == 0)
		flg |= EFLAG;

	if ((flg & SFLAG) && argc > 0) {
		/*
		 * POSIX requires new=old only with "fc -s".
		 */
		if (anys(UC "=", argv[0])) {
			substp = argv[0];
			argc--;
			argv++;
		}
	}

	if (!first && argc > 0) {
		cp = argv[0];
		if (*cp == '+')
			cp++;
		if (digit(*cp)) {
			first = stoi(UC cp);
			if (first == 0)
				flg |= ALLFLAG;
		} else {
			first = shedit_search_history(&intrptr,
							hflg, 0, C argv[0]);
		}
		argc--;
		argv++;
	}
	if (first == 0 && (flg & ALLFLAG) == 0) {
		if (flg & LFLAG)
			first = -15;
		else
			first = -1;
	}
	if (flg & SFLAG) {
		last = first;
	} else if (argc > 0) {
		cp = argv[0];
		if (*cp == '+')
			cp++;
		if (digit(*cp)) {
			last = stosi(UC cp);
		} else {
			last = shedit_search_history(&intrptr,
							hflg, 0, C argv[0]);
		}
		argc--;
		argv++;
	}
	if (argc > 0) {
		gfailure(av0, toomanyargs);
		return;
	}
	if (last == 0) {
		if (flg & LFLAG) {
			unsigned	lastn;

			shedit_histrange(NULL, &lastn, NULL);
			last = lastn;
		} else {
			last = first;
		}
	}

	if (flg & LFLAG) {
		if (isatty(STDOUT_FILENO))
			hflg |= HI_PRETTYP;
		/*
		 * We do not remove the new "fc -l" command but should include
		 * the last command in our listing.
		 */
		if (first < 0)
			first--;
	} else {
		unsigned	lastn;

		shedit_histrange(NULL, &lastn, NULL);
		/*
		 * Remove this "fc" command from the history.
		 */
		(void) shedit_remove_history(&intrptr,
						hflg, lastn, NULL);
		if (last == lastn) {
			shedit_histrange(NULL, &lastn, NULL);
			last = lastn;
		}
		fd = tmpfil(&tb);
	}
	/*
	 * Even in case we write the history into a file, we do not
	 * convert ASCII newlines into ANSI newlines, so we may see
	 * more lines in the file than history entries selected.
	 */
	if (shedit_history((flg & LFLAG) ? NULL : &fd,
			&intrptr, hflg, first, last, C substp) != 0) {
		gfailure(av0, "history specification out of range");
		return;
	}
	if (flg & LFLAG)
		return;

	if (flg & EFLAG) {
		char	*av[2];

		av[0] = C make(tmpout);
		av[1] = NULL;

		if (editor == NULL)
			editor = fcenod.namval;
		if (editor == NULL)
			editor = UC fcename;
		execexp(editor, (Intptr_t)av, xflags);
		unlink(av[0]);
		free(av[0]);
		if (exitval) {
			close(fd);
			return;
		}
	} else {
		unlink(C tmpout);
	}

	lseek(fd, (off_t)0, SEEK_SET);
	execexp(0, (Intptr_t)fd, xflags);
	/*
	 * Add re-executed commands to the history.
	 */
	lseek(fd, (off_t)0, SEEK_SET);
	shedit_read_history(&fd, &intrptr, hflg);

	close(fd);
}
#endif	/* defined(INTERACTIVE) || defined(DO_SYSFC) */

#ifdef	DO_TILDE
static unsigned char *
etilde(arg, s)
	unsigned char	*arg;		/* complete encironment entry	*/
	unsigned char	*s;		/* points to '='		*/
{
	UIntptr_t	b = relstak();
	unsigned char	*p;

	++s;				/* Point to 1st char of value	*/
	while (arg < s) {		/* Copy parts up to '='		*/
		GROWSTAKTOP();
		pushstak(*arg++);
	}
	while (*arg) {
		p = UC strchr(C arg, '~');
		if (p == NULL)
			break;
		while (arg < p) {	/* Copy up to '~'		*/
			GROWSTAKTOP();
			pushstak(*arg++);
		}
		if (p > s && p[-1] != ':') {
			/* EMPTY */
			;
		} else if ((p = do_tilde(arg)) != NULL) {
			unsigned char	c;

			while (*p) {	/* Copy expanded directory	*/
				GROWSTAKTOP();
				pushstak(*p++);
			}
			do {		/* Skip unexpanded input	*/
				c = *++arg;
			} while (c && c != '/' && c != ':');
			continue;
		}
		GROWSTAKTOP();
		pushstak(*arg++);	/* Copy '~'			*/
	}
	while (*arg) {			/* Copy rest			*/
		GROWSTAKTOP();
		pushstak(*arg++);
	}
	zerostak();
	return (absstak(b));
}
#endif
