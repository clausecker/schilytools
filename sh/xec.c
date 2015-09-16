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
 * Copyright 2008-2015 J. Schilling
 *
 * @(#)xec.c	1.47 15/09/11 2008-2015 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)xec.c	1.47 15/09/11 2008-2015 J. Schilling";
#endif

/*
 *
 * UNIX shell
 *
 */

#include	"sym.h"
#include	"hash.h"
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

	int	execute		__PR((struct trenod *argt,
					int xflags, int errorflg,
					int *pf1, int *pf2));
	void	execexp		__PR((unsigned char *s, Intptr_t f));
static	void	execprint	__PR((unsigned char **));

#define	no_pipe	(int *)0

/* ========	command execution	======== */

/*VARARGS3*/
int
execute(argt, xflags, errorflg, pf1, pf2)
struct trenod *argt;
int xflags, errorflg;
int *pf1, *pf2;
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

		switch (type)
		{
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
				} else {
					free(n->namval);
					free(n->namenv);

					n->namval = 0;
					n->namflg &= ~(N_EXPORT | N_ENVCHG);
				}
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
				 * We hang a fndnod on the namenv so that
				 * ref cnt(fndref) can be increased while
				 * running in the function.
				 */
				n->namenv = (unsigned char *)f;
				attrib(n, N_FUNCTN);
				hash_func(n->namid);
				break;
			}

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

				gchain = 0;
				argn = getarg((struct comnod *)t);
				com = scan(argn);
				gchain = schain;

				if (argn != 0)
					cmdhash = pathlook(com[0],
							1, comptr(t)->comset);

				if (argn == 0 ||
				    (comtype = hashtype(cmdhash)) == BUILTIN) {
#ifdef	DO_POSIX_SPEC_BLTIN
					sp = sysnlook(com[0],
							commands, no_commands);
					if (sp && (sp->sysflg & BLT_SPC) == 0)
						pushov = N_PUSHOV;
#endif
					setlist(comptr(t)->comset, pushov);
				}

				if (argn && (flags&noexec) == 0)
				{

					/* print command if execpr */
					if (flags & execpr)
						execprint(com);

					if (comtype == NOTFOUND) {
						pos = hashdata(cmdhash);
						ex.ex_status = C_NOEXEC;
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
						break;
					} else if (comtype == PATH_COMMAND) {
						pos = -1;
					} else if (comtype &
						    (COMMAND | REL_COMMAND)) {
						pos = hashdata(cmdhash);
					} else if (comtype == BUILTIN) {
#ifdef	DO_TIME
						struct job	*jp = NULL;

						if ((flags2 & timeflg) &&
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
						builtin(hashdata(cmdhash),
								argn, com, t);
#ifdef	DO_POSIX_SPEC_BLTIN
						if (pushov)
							namscan(popval);
#endif
#ifdef	DO_TIME
						if (jp) {
							prtime(jp);
							deallocjob(jp);
						}
#endif
						freejobs();
						break;
					} else if (comtype == FUNCTION) {
						struct dolnod *olddolh;
						struct namnod *n;
						struct fndnod *f;
						short idx;
						unsigned char **olddolv = dolv;
						int olddolc = dolc;
						n = findnam(com[0]);
						f = fndptr(n->namenv);
						/* just in case */
						if (f == NULL)
							break;
					/* save current positional parameters */
						olddolh = (struct dolnod *)
								savargs(funcnt);
						f->fndref++;
						funcnt++;
						idx = initio(io, 1);
						setargs(com);
						execute(f->fndval, xflags,
						    errorflg, pf1, pf2);
						execbrk = 0;
						restore(idx);
						(void) restorargs(olddolh,
								funcnt);
						dolv = olddolv;
						dolc = olddolc;
						funcnt--;
						/*
						 * n->namenv may have been
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
#endif

			exitval = 0;
			exval_clear();

			if (!(xflags & XEC_EXECED) || treeflgs&(FPOU|FAMP))
			{

				int forkcnt = 1;

				if (!(treeflgs&FPOU))
				{
					monitor = (!(xflags & XEC_NOSTOP) &&
					    (flags&(monitorflg|jcflg|jcoff))
					    == (monitorflg|jcflg));
					if (monitor) {
						int save_fd;

						save_fd = setb(-1);
						prcmd(t);
						allocjob((char *)stakbot,
							cwdget(), monitor);
						(void) setb(save_fd);
					} else {
						allocjob("",
							(unsigned char *)"", 0);
					}
				}

				if (treeflgs & (FPOU|FAMP)) {
					link_iodocs(iotemp);
					linked = 1;
				}
#ifdef	HAVE_VFORK
				else if (type == TCOM) {
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
script:
				while ((parent = isvfork?vfork():fork()) == -1)
#else
				while ((parent = fork()) == -1)
#endif
				{
				/*
				 * FORKLIM is the max period between forks -
				 * power of 2 usually.	Currently shell tries
				 * after 2,4,8,16, and 32 seconds and then quits
				 */

				if ((forkcnt = (forkcnt * 2)) > FORKLIM)
				{
					switch (errno)
					{
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

				if (parent) {
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
					}
#endif
					if (monitor)
						setpgid(parent, 0);
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
						namscan(popval);
						restoresigs();
						if (exflag == 2) {
							exflag = 0;
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
			}

			/*
			 * Forked process:  assume it is not a subshell for
			 * now.  If it is, the presence of a left parenthesis
			 * will trigger the jcoff flag to be turned off.
			 * When jcoff is turned on, monitoring is not going on
			 * and waitpid will not look for WUNTRACED.
			 */

			flags |= (forked|jcoff);

			fiotemp  = 0;

			if (linked == 1) {
				swap_iodoc_nm(iotemp);
				xflags |= XEC_LINKED;
			} else if (!(xflags & XEC_LINKED))
				iotemp = 0;
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
			if (treeflgs & FPIN)
			{
				renamef(pf1[INPIPE], STDIN_FILENO);
				close(pf1[OTPIPE]);
			}

			if (treeflgs & FPOU)
			{
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
				if (isvfork)
					rmtemp(oiot);
				else
#endif
				{
					rmtemp(0);
					clearjobs();
				}
				execa(com, pos, isvfork, NULL);
			}
			done(0);
			/* NOTREACHED */
		}

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
				if (execute(lstptr(t)->lstlef,
				    xflags & XEC_NOSTOP, errorflg,
				    pf1, pv) == 0) {
					execute(lstptr(t)->lstrit,
						xflags, errorflg,
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
			{
				struct namnod *n = lookup(forptr(t)->fornam);
				unsigned char	**args;
				struct dolnod *argsav = 0;

				if (forptr(t)->forlst == 0)
				{
					args = dolv + 1;
					argsav = useargs();
				}
				else
				{
					struct argnod *schain = gchain;

					gchain = 0;
					args = scan(getarg(forptr(t)->forlst));
					gchain = schain;
				}
				loopcnt++;
				while (*args != ENDARGS && execbrk == 0)
				{
					assign(n, *args++);
					execute(forptr(t)->fortre,
						XEC_NOSTOP, errorflg,
						no_pipe, no_pipe);
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
	sigchk();
	tdystak(sav, iosav);
	flags |= eflag;
	return (exitval);
}

void
execexp(s, f)
	unsigned char	*s;
	Intptr_t	f;
{
	struct fileblk	fb;

	push(&fb);
	if (s)
	{
		estabf(s);
		fb.feval = (unsigned char **)(f);
	} else if (f >= 0)
		initf(f);
	execute(cmd(NL, NLFLG | MTFLG),
		0, (int)(flags & errflg), no_pipe, no_pipe);
	pop();
}

static void
execprint(com)
	unsigned char	**com;
{
	int	argn = 0;
	unsigned char	*s;

#ifdef	DO_PS34
	prs(macro(ps4nod.namval?ps4nod.namval:UC execpmsg));
#else
	prs(_gettext(execpmsg));
#endif
	while (com[argn] != ENDARGS)
	{
		s = com[argn++];
		write(output, s, length(s) - 1);
		blank();
	}

	newline();
}
