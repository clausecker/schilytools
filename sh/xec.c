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
#pragma ident	"@(#)xec.c	1.25	06/06/16 SMI"
#endif

#include "defs.h"

/*
 * Copyright 2008-2017 J. Schilling
 *
 * @(#)xec.c	1.94 17/11/07 2008-2017 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)xec.c	1.94 17/11/07 2008-2017 J. Schilling";
#endif

/*
 *
 * UNIX shell
 *
 */

#include	"sym.h"
#include	"hash.h"
#ifdef	DO_TIME
#include	"jobs.h"
#endif
#ifdef	SCHILY_INCLUDES
#include	<schily/times.h>
#include	<schily/vfork.h>
#include	<schily/errno.h>
#else
#include	<sys/types.h>
#include	<sys/times.h>
#include	<errno.h>
#endif

pid_t parent;
#ifdef	DO_PIPE_PARENT
static jmp_buf	forkjmp;	/* To go back to TNOFORK in case of builtins */
#endif

	int	execute		__PR((struct trenod *argt,
					int xflags, int errorflg,
					int *pf1, int *pf2));
#ifdef	DO_PS34
	unsigned char *ps_macro	__PR((unsigned char *as, int perm));
#endif
	void	execexp		__PR((unsigned char *s, Intptr_t f,
					int xflags));
static	void	execprint	__PR((unsigned char **));
static	int	ismonitor	__PR((int xflags));
static	int	exallocjob	__PR((struct trenod *t, int xflags));

#define	no_pipe	(int *)0

/* ========	command execution	======== */

/*VARARGS3*/
int
execute(argt, xflags, errorflg, pf1, pf2)
	struct trenod	*argt;
	int		xflags;
	int		errorflg;
	int		*pf1;
	int		*pf2;
{
	/*
	 * `stakbot' is preserved by this routine
	 */
	struct trenod	*t;
	unsigned char		*sav = savstak();
	struct ionod		*iosav = iotemp;

	sigchk();
	if (!errorflg)
		flags &= ~errflg;

	if ((t = argt) != 0 && execbrk == 0) {
		int treeflgs;
		unsigned char **com = NULL;
		int type;
		short pos = 0;

		treeflgs = t->tretyp;
		type = treeflgs & COMMSK;

		/*
		 * If we ever like to move a global exitval = 0; exval_clear();
		 * here, we need to take care about the last exit val for a
		 * "exit" call without arguments.
		 */
		switch (type) {
		case TFND:		/* function definition */
			{
				struct fndnod	*f = fndptr(t);
				struct namnod	*n = lookup(f->fndnam);

				exitval = 0;
				exval_clear();

				if (n->namflg & N_RDONLY)
					failed(n->namid, wtfailed);

				if (flags & rshflg && (n == &pathnod ||
					eq(n->namid, "SHELL")))
					failed(n->namid, restricted);
				/*
				 * If function of same name is previously
				 * defined, it will no longer be used.
				 */
				if (n->namflg & N_FUNCTN) {
					freefunc(n);
				}
#ifndef	DO_POSIX_UNSET
				else {
					free(n->namval);
					free(n->namenv);

					n->namval = 0;
					n->namflg &= ~(N_EXPORT | N_ENVCHG);
				}
#endif
				/*
				 * If function is defined within function,
				 * we don't want to free it along with the
				 * free of the defining function. If we are
				 * in a loop, fndnod may be reused, so it
				 * should never be freed.
				 */
				if (funcnt != 0 || loopcnt != 0)
					f->fndref++;

				/*
				 * We hang a fndnod on the funcval so that
				 * ref cnt(fndref) can be increased while
				 * running in the function.
				 */
				n->funcval = (unsigned char *)f;
				attrib(n, N_FUNCTN);
				hash_func(n->namid);
				break;
			}

#ifdef	DO_TIME
		case TTIME:		/* "time" command prefix */
			{
				struct parnod	*p = parptr(t);
				struct job	j;
				int		osystime = flags2 & systime;
				int		oflags = flags;

				exitval = 0;
				exval_clear();

				gettimeofday(&j.j_start, NULL);
				ruget(&j.j_rustart);
				flags2 |= systime;
				execute(p->partre, xflags, 0, pf1, pf2);
				prtime(&j);
				if (!osystime)
					flags2 &= ~systime;
				flags = oflags;
				break;
			}
#endif

#ifdef	DO_NOTSYM
		case TNOT:		/* NOT command prefix */
			{
				struct parnod	*p = parptr(t);

				exitval = 0;
				exval_clear();

				execute(p->partre, xflags, 0, pf1, pf2);
				/*
				 * In extended Bourne Shell mode, exitval does
				 * not suffer from the exitcode mod 256 problem
				 * and exitval is != 0 if exitcode is != 0, even
				 * when exitcode is masked by 0xFF.
				 */
				ex.ex_status = exitval = !exitval;
				break;
			}
#endif

		case TCOM:		/* some kind of command */
			{
				int	argn;
				struct argnod	*schain = gchain;
				struct ionod	*io = t->treio;
				short	cmdhash = 0;
				short	comtype = 0;
#ifdef	DO_POSIX_SPEC_BLTIN
				const struct sysnod	*sp = 0;
#endif
				int			pushov = 0;

				exitval = 0;
				exval_clear();

				if (xflags & XEC_HASARGV) {
					/*
					 * Cheat code to support "command cmd"
					 */
					com = (unsigned char **)
							comptr(t)->comarg;
					argn = 1;	/* Cheat */
				} else {
					gchain = 0;
					argn = getarg((struct comnod *)t);
					com = scan(argn);
					gchain = schain;
				}
				if (argn != 0)
					cmdhash = pathlook(com[0],
							1, comptr(t)->comset);

#ifdef	DO_PIPE_PARENT
				if (xflags & XEC_NOBLTIN) {
					/*
					 * Check whether we cannot run the
					 * builtin in the main shell because we
					 * would need to fork when in the
					 * middle of a longer pipe.
					 */
					comtype = hashtype(cmdhash);
					if (comtype == BUILTIN ||
					    comtype == FUNCTION) {
						tdystak(sav, iosav);
						longjmp(forkjmp, 1);
					}
				}
#endif	/* DO_PIPE_PARENT */

				if (argn == 0 ||
				    (comtype = hashtype(cmdhash)) == BUILTIN) {
#ifdef	DO_POSIX_SPEC_BLTIN
					sp = sysnlook(com[0],
							commands, no_commands);
					if (sp && (sp->sysflg & BLT_SPC) == 0)
						pushov = N_PUSHOV;
#endif
#ifdef	DO_POSIX_EXPORT
					/*
					 * Exporting a shell variable with
					 * "VAR=val exec cmd" is not required
					 * by POSIX but looks more orthogonal.
					 */
					setlist(comptr(t)->comset,
						(argn > 0?N_EXPORT:0)|pushov);
#else
					setlist(comptr(t)->comset, pushov);
#endif

				}

				if (argn && (flags&noexec) == 0) {

					/* print command if execpr */
					if (flags & execpr)
						execprint(com);

					if (comtype == NOTFOUND) {
#ifdef	DO_POSIX_REDIRECT
						short	fdindex;
#endif
#ifdef	DO_PIPE_PARENT
						resetjobfd();	/* Rest stdin */
						if (ismonitor(xflags)) {
							settgid(mypgid,
								curpgid());
						}
#endif
						pos = hashdata(cmdhash);
						ex.ex_status = C_NOEXEC;
#ifdef	DO_POSIX_REDIRECT
						fdindex = initio(t->treio, 1);
#endif
						if (pos == 1) {
							ex.ex_status =
							ex.ex_code = C_NOTFOUND;
							failurex(ERR_NOTFOUND,
								*com, notfound);
						} else if (pos == 2) {
							ex.ex_code = C_NOEXEC;
							failurex(ERR_NOEXEC,
								*com, badexec);
						} else {
							ex.ex_code = C_NOEXEC;
							failurex(ERR_NOEXEC,
								*com, badperm);
						}
#ifdef	DO_POSIX_REDIRECT
						restore(fdindex);
#endif
						break;
					} else if (comtype == PATH_COMMAND) {
						pos = -1;
					} else if (comtype &
						    (COMMAND | REL_COMMAND)) {
						pos = hashdata(cmdhash);
					} else if (comtype == BUILTIN) {
#ifdef	DO_PIPE_PARENT
						pid_t	pgid = curpgid();
						int	monitor =
							    ismonitor(xflags);
#endif
#ifdef	DO_TIME
						struct job	*jp = NULL;

						if (((flags2 &
						    (timeflg|systime)) ==
						    timeflg) &&
						    !(xflags & XEC_EXECED) &&
						    !(treeflgs&(FPOU|FAMP))) {
							/*
							 * treeflgs is always 0
							 * here and if we don't
							 * check xflags as well
							 * we come here even
							 * for the left side of
							 * a pipe or backgr job.
							 */
							allocjob("", UC "", 0);
							jp =
							postjob(parent, 1, 1);
						}
#endif
						builtin(cmdhash,
							argn, com, t, xflags);
#ifdef	DO_PIPE_PARENT
						/*
						 * Rest stdin if needed
						 */
						if (xflags & XEC_STDINSAV)
							resetjobfd();

						if (monitor)
							settgid(mypgid, pgid);
#endif
#ifdef	DO_POSIX_SPEC_BLTIN
						if (pushov && comptr(t)->comset)
							popvars();
#endif
#ifdef	DO_TIME
						if (jp) {
							prtime(jp);
							deallocjob(jp);
						}
#endif
						freejobs();
#ifdef	DO_POSIX_RETURN
						if (dotcnt > 0 && dotbrk) {
							dotbrk = 0;
							longjmp(dotshell->jb,
								    1);
						}
#endif
						break;
					} else if (comtype == FUNCTION) {
						unsigned long	oflags = flags;
						struct dolnod *olddolh;
						struct namnod *n;
						struct fndnod *f;
						short idx;
						unsigned char **olddolv = dolv;
						int olddolc = dolc;
						void *olocalp = localp;
						int olocalcnt = localcnt;
						int odotcnt = dotcnt;

						n = findnam(com[0]);
						f = fndptr(n->funcval);
						/* just in case */
						if (f == NULL)
							break;
					/* save current positional parameters */
						olddolh = (struct dolnod *)
								savargs(funcnt);
						f->fndref++;
						funcnt++;
						dotcnt = 0;
#ifdef	DO_POSIX_FAILURE
						flags |= noexit;
#endif
						idx = initio(io, 1);
						flags = oflags;
						setargs(com);
						/*
						 * If flags & noexit is not set,
						 * we do not come here in case
						 * exitval != 0,
						 */
						if (exitval == 0) {
							localp = t;
							localcnt = 0;
							execute(f->fndval,
							    xflags,
							    errorflg, pf1, pf2);
#ifdef	DO_SYSLOCAL
							if (localcnt > 0) {
								localp = t;
								poplvars();
							}
#endif
							localp = olocalp;
							localcnt = olocalcnt;
						}
						execbrk = 0;
						restore(idx);
#ifdef	DO_PIPE_PARENT
						/*
						 * Rest stdin if needed
						 */
						if (xflags & XEC_STDINSAV)
							resetjobfd();
#endif
						(void) restorargs(olddolh,
								funcnt);
						dolv = olddolv;
						dolc = olddolc;
						funcnt--;
						dotcnt = odotcnt;
						/*
						 * n->funcval may have been
						 * pointing different func.
						 * Therefore, we can't use
						 * freefunc(n).
						 */
						freetree((struct trenod *)f);

						break;
					}
				} else if (t->treio == 0) {
					chktrap();
					break;
				}

			}
			/* FALLTHROUGH */

		case TFORK:		/* running forked cmd */
#ifdef	DO_PIPE_PARENT
		dofork:			/* In case we modify a TNOFORK cmd */
#endif
		{
			int monitor = 0;
			int linked = 0;
			int isvfork = 0;
#ifdef	HAVE_VFORK
			int oflags = flags;
			int oserial = serial;
			pid_t opid = mypid;
			pid_t opgid = mypgid;
			struct ionod *ofiot = fiotemp;
			struct ionod *oiot = iotemp;
			struct fileblk *ostandin = standin;
			struct excode oex;
#endif

			exitval = 0;
			exval_clear();
#ifdef	HAVE_VFORK
			oex = ex;
#endif

#ifdef	DO_TRAP_EXIT
			if (trapcom[0])
				xflags &= ~XEC_EXECED;
#endif
			if (!(xflags & XEC_EXECED) || treeflgs&(FPOU|FAMP)) {
				int forkcnt = 1;

#ifdef	DO_PIPE_PARENT
				if (!(xflags & XEC_ALLOCJOB) ||
				    !(treeflgs & FPOU)) {
#else
				if (!(treeflgs & FPOU)) {
#endif
					/*
					 * Allocate job slot
					 * Make sure, this happens even when
					 * XEC_ALLOCJOB is set but curjob()
					 * returns NULL.
					 */
#ifdef	DO_PIPE_PARENT
					if (!curjob())
						xflags &= ~XEC_ALLOCJOB;
#endif
					monitor = exallocjob(t, xflags);
#ifdef	DO_PIPE_PARENT
					xflags |= XEC_ALLOCJOB;
#endif
				}

#ifdef	HAVE_VFORK
				if (type == TCOM) {
					if (com != NULL && com[0] != ENDARGS &&
					    !(flags & vforked)) {
						isvfork = TRUE;
						flags |= vforked;
						/*
						 * exflag does not need to be
						 * set here already as done()
						 * only calls exit()
						 * if (flags & subsh) is set.
						 */
					}
					/*
					 * Cygwin has no real vfork() as it runs
					 * parent and child simultaneously. This
					 * is the last chance to save things
					 * that would be clobbered by vfork().
					 */
				}
				if (!isvfork &&
				    (treeflgs & (FPOU|FAMP))) {
					linked |= link_iodocs(iotemp);
					linked |= link_iodocs(fiotemp);
				}
script:
				while ((parent = isvfork?vfork():fork()) == -1)
#else
				if (treeflgs & (FPOU|FAMP)) {
					link_iodocs(iotemp);
					link_iodocs(fiotemp);
					linked = 1;
				}
				while ((parent = fork()) == -1)
#endif
				{
				/*
				 * FORKLIM is the max period between forks -
				 * power of 2 usually.	Currently shell tries
				 * after 2,4,8,16, and 32 seconds and then quits
				 */

				if ((forkcnt = (forkcnt * 2)) > FORKLIM) {
					switch (errno) {
					case ENOMEM:
						deallocjob(NULL);
						error(noswap);
						break;
					default:
						deallocjob(NULL);
						error(nofork);
						break;
					}
				} else if (errno == EPERM) {
					deallocjob(NULL);
					error(eacces);
					break;
				}
				sigchk();
				sh_sleep(forkcnt);
				}

				if (parent) {	/* Parent != 0 -> Child pid */
#ifdef	DO_PIPE_PARENT
					pid_t pgid = curpgid();

					/*
					 * The first process in this command
					 * Remember the id as process group.
					 */
					if (pgid == 0 && monitor)
						setjobpgid(parent);
#endif
					/*
					 * XXX Do we need to call restoresigs()
					 * XXX here too on Solaris?
					 */
#ifdef	HAVE_VFORK
					if (isvfork) {
						/*
						 * Needed by the jobcontrol
						 * called from postjob().
						 * So we need to restore these
						 * variables immediately.
						 */
						mypid = opid;
						mypgid = opgid;
						flags = oflags;
						ex = oex;
					}
#endif
					if (monitor)
#ifdef	DO_PIPE_PARENT
						setpgid(parent, pgid);
#else
						setpgid(parent, 0);
#endif
					if (treeflgs & FPIN)
						closepipe(pf1);
					if (!(treeflgs&FPOU)) {
						postjob(parent,
							!(treeflgs&FAMP), 0);
						freejobs();
					}
#ifdef	HAVE_VFORK
					if (isvfork) {
						/*
						 * Restore evereything that was
						 * overwritten by the vforked
						 * child. As Cygwin does not
						 * have a real vfork() (see
						 * above), we need to make sure
						 * the child did start up before
						 * we restore global variables.
						 */
						mypid = opid;
						mypgid = opgid;
						flags = oflags;
						settmp();
						serial = oserial;
						isvfork = 0;
						fiotemp = ofiot;
						iotemp = oiot;
						standin = ostandin;
						if (comptr(t)->comset)
							popvars();
						restoresigs();
						if (exflag == 2) {
							exflag = 0;

							if (treeflgs & FPOU)
								goto script;
							/*
							 * Allocate job slot
							 */
#ifdef	DO_PIPE_PARENT
							if (!curjob()) {
								xflags &=
								~XEC_ALLOCJOB;
							}
#endif
							monitor = exallocjob(t,
								    xflags);
#ifdef	DO_PIPE_PARENT
							xflags |= XEC_ALLOCJOB;
#endif
							goto script;
						} else {
							exflag = 0;
						}
					}
#endif
					chktrap();
					break;		/* From case TFORK: */
				}
				mypid = getpid();	/* This is the child */
#ifdef	DO_PIPE_PARENT
				if (monitor) {
					pid_t pgid = curpgid();

					/*
					 * The first process in this command
					 * Remember the id as process group.
					 *
					 * In special when using vfork together
					 * with the new pipe setup, we need to
					 * set the process group for every
					 * child but cannot do this from the
					 * parent as the parent process is
					 * blocked until the child called exec()
					 */
					if (pgid == 0) {
						pgid = mypid;
						setjobpgid(pgid);
						/*
						 * If this is a subshell with a
						 * new job entry (pgid == 0),
						 * we need to remember the pgid
						 * as mypgid to avoid to reset
						 * the tty process group from
						 * inside waitjob(). This
						 * assignment to mypgid used to
						 * be in makejob().
						 */
						if (type == TFORK)
							mypgid = pgid;
					}
					setpgid(mypid, pgid);
					if (!(treeflgs & FAMP))
						settgid(pgid, mypgid);
				}
#endif	/* DO_PIPE_PARENT */
			}

			/*
			 * Forked process:  assume it is not a subshell for
			 * now.  If it is, the presence of a left parenthesis
			 * will trigger the jcoff flag to be turned off.
			 * When jcoff is turned on, monitoring is not going on
			 * and waitpid will not look for WSTOPPED.
			 */

			flags |= (forked|jcoff);

			if (linked == 1) {
				swap_iodoc_nm(iotemp);
				swap_iodoc_nm(fiotemp);
				xflags |= XEC_LINKED;
			} else if (!(xflags & XEC_LINKED)) {
				iotemp = 0;
				fiotemp  = 0;
			}
#ifdef ACCT
			suspacct();
#endif
			settmp();			/* /tmp/sh<pid> */
			oldsigs(!isvfork);		/* Calls clrsig/free */

			/*
			 * Job control: pgrp / TTY-signal handling
			 */
			if (!(treeflgs & FPOU))
				makejob(monitor, !(treeflgs & FAMP));

			/*
			 * pipe in or out
			 */
			if (treeflgs & FPIN) {
				renamef(pf1[INPIPE], STDIN_FILENO);
				close(pf1[OTPIPE]);
			}

			if (treeflgs & FPOU) {
				close(pf2[INPIPE]);
				/*
				 * pipe fd # is in low bits of treeflgs
				 */
				renamef(pf2[OTPIPE], treeflgs & IOUFD);
			}

			/*
			 * io redirection
			 */
			initio(t->treio, 0);

			if (type == TFORK) {
				execute(forkptr(t)->forktre,
					xflags | XEC_EXECED,
					errorflg, no_pipe, no_pipe);
			} else if (com != NULL && com[0] != ENDARGS) {
				int	pushov = isvfork?N_PUSHOV:0;

				eflag = 0;
				setlist(comptr(t)->comset, N_EXPORT|pushov);
#ifdef	HAVE_VFORK
				if (isvfork) {
					rmtemp(oiot);
					rmfunctmp(ofiot);
				} else
#endif
				{
					rmtemp(0);
					rmfunctmp(0);
					clearjobs();
				}
				execa(com, pos, isvfork, NULL);
			}
			done(0);
			/* NOTREACHED */
		}

#ifdef	DO_PIPE_PARENT
		case TNOFORK:		/* running avoid fork cmd */
		{
			struct trenod	*anod = forkptr(t)->forktre;

			/*
			 * pipe-in only -> last element of a pipeline
			 * we execute builtin command in the main shell
			 */
			if ((treeflgs & ~COMMSK) == FPIN) {
				int		sfd = -1;
				extern short	topfd;

				if ((xflags & XEC_STDINSAV) == 0) {
					/*
					 * Move stdin to save it. This move is
					 * restored from inside postjob().
					 */
					sfd = topfd;
					fdmap[topfd].org_fd = STDIN_FILENO;
					fdmap[topfd].dup_fd =
							savefd(STDIN_FILENO);
					setjobfd(fdmap[topfd++].dup_fd, sfd);
				}
				renamef(pf1[INPIPE], STDIN_FILENO);
				close(pf1[OTPIPE]);
				execute(anod,
					xflags | XEC_STDINSAV,
					errorflg, pf1, pf2);
				break;
			}

			/*
			 * Other cases: need to fork if a builtin is part
			 * of a pipeline. This construct helps to use vfork for
			 * non-builtin commands.
			 */
			if (setjmp(forkjmp)) {
				type = TFORK;
				xflags |= XEC_ALLOCJOB;	/* Was in a register? */
				goto dofork;
			} else {
				struct trenod	*tptr;
				struct comnod	cnod;
				struct lstnod	lnod;

				/*
				 * A double cast (first to void *) is needed to
				 * make gcc quiet.
				 * comnod is an int and three pointers
				 * lstnod is an int and two pointers
				 */
				switch (anod->tretyp & COMMSK) {
				case TFIL:
					lnod = *lstptr(anod);
					tptr = treptr((void *)&lnod);
					break;
				case TCOM:
					cnod = *comptr(anod);
					tptr = treptr((void *)&cnod);
					break;
				default:
					/*
					 * Should never happen
					 */
					failed(UC "TNOFORK", "botch");
					goto out;
				}
				tptr->tretyp |= treeflgs & (FPIN|FPOU|IOFMSK);
				execute(tptr,
					xflags | XEC_NOBLTIN,
					errorflg, pf1, pf2);
				break;
			}
		}
			/* NOTREACHED */
#endif	/* DO_PIPE_PARENT */

#ifdef	DO_SETIO_NOFORK
		case TSETIO:		/* save and reset IO streams */
		{
			short	fdindex;

			fdindex = initio(t->treio, 1);
			execute(forkptr(t)->forktre,
				xflags, errorflg,
				no_pipe, no_pipe);
			restore(fdindex);
			break;
		}
#endif	/* DO_SETIO_NOFORK */

		case TPAR:		/* "()" parentized cmd */
			/*
			 * Forked process is subshell:  may want job control
			 * but not for left hand sides of of a pipeline.
			 */
			if ((xflags & XEC_LINKED) == 0)
				flags &= ~jcoff;
			clearjobs();
			execute(parptr(t)->partre,
				xflags, errorflg,
				no_pipe, no_pipe);
			done(0);
			/* NOTREACHED */

		case TFIL:		/* PIPE "|" filter */
			{
				int pv[2];

				chkpipe(pv);

#ifdef	DO_PIPE_PARENT
				if (!(xflags & XEC_ALLOCJOB) &&
				    !(treeflgs&FPOU)) {
					/*
					 * Allocate job slot
					 */
					exallocjob(t, xflags);
					xflags |= XEC_ALLOCJOB;
				}
#endif	/* DO_PIPE_PARENT */
				if (execute(lstptr(t)->lstlef,
				    xflags & (XEC_NOSTOP|XEC_ALLOCJOB),
				    errorflg,
				    pf1, pv) == 0) {
					execute(lstptr(t)->lstrit,
						xflags,
						errorflg,
						pv, pf2);
				} else {
					closepipe(pv);
				}
			}
			break;

		case TLST:		/* ";" separated command list */
			execute(lstptr(t)->lstlef,
				xflags&XEC_NOSTOP, errorflg,
				no_pipe, no_pipe);
			/* Update errorflg if set -e is invoked in the sub-sh */
			execute(lstptr(t)->lstrit,
				xflags, (errorflg | (eflag & errflg)),
				no_pipe, no_pipe);
			break;

		case TAND:		/* "&&" command */
		case TORF:		/* "||" command */
		{
			int xval;
			xval = execute(lstptr(t)->lstlef,
					XEC_NOSTOP, 0,
					no_pipe, no_pipe);
			if ((xval == 0) == (type == TAND))
				execute(lstptr(t)->lstrit,
					xflags|XEC_NOSTOP, errorflg,
					no_pipe, no_pipe);
			break;
		}

		case TFOR:		/* for ... do .. done */
		case TSELECT:		/* select ... do .. done */
			{
				struct namnod *n = lookup(forptr(t)->fornam);
				unsigned char	**args;
				struct dolnod *argsav = 0;
				int	argn;
#ifdef	DO_SELECT
				int	printlist = 1;
#endif

				if (forptr(t)->forlst == 0) {
					argn = dolc;
					args = dolv + 1;
					argsav = useargs();
				} else {
					struct argnod *schain = gchain;

					gchain = 0;
					argn = getarg(forptr(t)->forlst);
					args = scan(argn);
					gchain = schain;
				}
				loopcnt++;
				while (*args != ENDARGS && execbrk == 0) {
#ifdef	DO_SELECT
					if (type == TSELECT) {
						int		c;
						unsigned char	*cp;

						if (printlist) {
							for (c = 0; c < argn;
									c++) {
								prn(c+1);
								prs(UC ") ");
								prs(args[c]);
								prc(NL);
							}
							printlist = 0;
						}

						prs(ps3nod.namval);
						rwait = 1;
						exitval = readvar(0, NULL);
						rwait = 0;
						if (exitval)
							break;
						if (repnod.namval == NULL ||
						    *repnod.namval == '\0') {
							printlist++;
							continue;
						}

						cp = repnod.namval;
						while ((c = *cp++) != '\0') {
							if (!digit(c))
								break;
						}
						if (c == 0) {
							c = stoi(repnod.namval);
							cp = args[c-1];
						} else {
							cp = UC nullstr;
						}
						assign(n, cp);
					} else
#endif
						assign(n, *args++);
					execute(forptr(t)->fortre,
						XEC_NOSTOP, errorflg,
						no_pipe, no_pipe);
#ifdef	DO_SELECT
					if (type == TSELECT) {
						/*
						 * Check whether RESULT has
						 * been cleared from the code
						 * above.
						 */
						if (repnod.namval == NULL ||
						    *repnod.namval == '\0')
							printlist++;
					}
#endif
					if (breakcnt < 0)
						execbrk = (++breakcnt != 0);
				}
				if (breakcnt > 0)
						execbrk = (--breakcnt != 0);

				loopcnt--;
				if (argsav)
					argfor = (struct dolnod *)
							freeargs(argsav);
			}
			break;

		case TWH:		/* "while" loop */
		case TUN:		/* "until" loop */
			{
				int		i = 0;
				struct excode	savex;

				exval_clear();
				savex = ex;
				loopcnt++;
				while (execbrk == 0 && (execute(whptr(t)->whtre,
				    XEC_NOSTOP, 0,
				    no_pipe, no_pipe) == 0) == (type == TWH) &&
				    (flags&noexec) == 0) {
					exval_clear();
					i = execute(whptr(t)->dotre,
						XEC_NOSTOP, errorflg,
						no_pipe, no_pipe);
					savex = ex;
					if (breakcnt < 0)
						execbrk = (++breakcnt != 0);
				}
				if (breakcnt > 0)
						execbrk = (--breakcnt != 0);

				loopcnt--;
				exitval = i;
				ex = savex;
			}
			break;

		case TIF:		/* if ... then ... */
			if (execute(ifptr(t)->iftre,
			    XEC_NOSTOP, 0,
			    no_pipe, no_pipe) == 0) {
				execute(ifptr(t)->thtre,
					xflags|XEC_NOSTOP, errorflg,
					no_pipe, no_pipe);
			} else if (ifptr(t)->eltre) {
				execute(ifptr(t)->eltre,
					xflags|XEC_NOSTOP, errorflg,
					no_pipe, no_pipe);
			} else {
				/* force zero exit for if-then-fi */
				exitval = 0;
				exval_clear();
				exval_set(0);
			}
			break;

		case TSW:		/* "case command */
			{
				unsigned char	*r = mactrim(swptr(t)->swarg);
				struct regnod *regp;

#ifdef	DO_POSIX_CASE
				exitval = 0;
				exval_clear();
#endif
				regp = swptr(t)->swlst;
				while (regp) {
					struct argnod *rex = regp->regptr;

					while (rex) {
						unsigned char	*s;

						if (gmatch((char *)r, (char *)
						    (s = macro(rex->argval))) ||
						    (trim(s), eq(r, s))) {
							execute(regp->regcom,
								XEC_NOSTOP,
								errorflg,
								no_pipe,
								no_pipe);
							regp = 0;
							break;
						}
						else
							rex = rex->argnxt;
					}
					if (regp)
						regp = regp->regnxt;
				}
			}
			break;
		}
		exitset();
	}
#ifdef	DO_PIPE_PARENT
out:
#endif
	sigchk();
	tdystak(sav, iosav);
	if (flags & errflg && exitval)
		done(0);
	flags |= eflag;
	return (exitval);
}

void
execexp(s, f, xflags)
	unsigned char	*s;
	Intptr_t	f;
	int		xflags;
{
	struct fileblk	fb;

	push(&fb);
	if (s) {
		estabf(s);
		fb.feval = (unsigned char **)(f);
	} else if (f >= 0)
		initf(f);

	/*
	 * xflags != 0 currently only happens from bltin.c
	 * so we switch off XEC_EXECED here. This may change
	 * in the future.
	 * We need to do this because "eval cmd&" may otherwise
	 * leave shell tempfiles if "cmd" contains a here document.
	 */
	xflags &= ~XEC_EXECED;
	execute(cmd(NL, NLFLG | MTFLG | SEMIFLG),
		xflags, (int)(flags & errflg), no_pipe, no_pipe);
	pop();
}

#ifdef	DO_PS34
unsigned char *
ps_macro(as, perm)
	unsigned char	*as;
	int		perm;
{
extern	int		macflag;
	int		oflags = flags;
	int		omacflag = macflag;
	int		otrapnote = trapnote;
	unsigned char	*res;

	/*
	 * Disable -x and -v to avoid recursion.
	 */
	flags &= ~(execpr|readpr);

	/*
	 * Disable exit() and longjmp() in case of errors,
	 * needed to avoid endless longjmp() loops in case of
	 * unsuported substitustions (e.g. a ksh93 .profile).
	 */
	flags &= ~errflg; 
	flags |= noexit; 

	if ((flags2 & promptcmdsubst) == 0)
		macflag |= M_NOCOMSUBST;
	trapnote = 0;
	res = _macro(as);
	staktop = res;		/* Restore to previous stakbot offset */
	macflag = omacflag;
	flags = oflags;
	trapnote = otrapnote;

	if (perm)
		return (make(res));

	return (res);
}
#endif

static void
execprint(com)
	unsigned char	**com;
{
	int	argn = 0;
	unsigned char	*s;

#ifdef	DO_PS34
	prs(ps_macro(ps4nod.namval?ps4nod.namval:UC execpmsg, FALSE));
#else
	prs(_gettext(execpmsg));
#endif
	while (com[argn] != ENDARGS) {
		s = com[argn++];
		write(output, s, length(s) - 1);
		blank();
	}

	newline();
}

static int
ismonitor(xflags)
	int		xflags;
{
	return (!(xflags & XEC_NOSTOP) &&
		    (flags&(monitorflg|jcflg|jcoff)) == (monitorflg|jcflg));
}

static int
exallocjob(t, xflags)
	struct trenod	*t;
	int		xflags;
{
	int	monitor = ismonitor(xflags);

	if (xflags & XEC_ALLOCJOB) {
		/* EMPTY */;
	} else if (monitor) {
		int save_fd;

		save_fd = setb(-1);
		if (xflags & XEC_HASARGV) {
			unsigned char	**av = UCP comptr(t)->comarg;

			/*
			 * Cheat code to support "command cmd"
			 */
			while (*av) {
				prs_buff(*av++);
				if (*av)
					prc_buff(SPACE);
			}
		} else {
			prcmd(t);
		}
		/*
		 * We use cwdget(CHDIR_L) as this does not modify PWD
		 * unless it is definitely invalid.
		 */
		allocjob((char *)stakbot, cwdget(CHDIR_L), monitor);
		(void) setb(save_fd);
	} else {
		allocjob("", (unsigned char *)"", 0);
	}
	return (monitor);
}
