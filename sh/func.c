/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
 * Copyright 1996 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#if defined(sun)
#pragma ident	"@(#)func.c	1.11	05/09/13 SMI"
#endif

#include "defs.h"

/*
 * This file contains modifications Copyright 2008-2015 J. Schilling
 *
 * @(#)func.c	1.14 15/03/31 2008-2015 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)func.c	1.14 15/03/31 2008-2015 J. Schilling";
#endif

/*
 * UNIX shell
 */

	void	freefunc	__PR((struct namnod  *n));
	void	freetree	__PR((struct trenod *));
static void	free_arg	__PR((struct argnod *));
static void	freeio		__PR((struct ionod *));
static void	freereg		__PR((struct regnod *));
static	void	prbgnlst	__PR((void));
static	void	prendlst	__PR((void));
	void	prcmd		__PR((struct trenod *t));
	void	prf		__PR((struct trenod *t));
static void	prarg		__PR((struct argnod *argp));
static void	prio		__PR((struct ionod *iop));

void
freefunc(n)
	struct namnod	*n;
{
	freetree((struct trenod *)(n->namenv));
}

void
freetree(t)
	struct trenod	*t;
{
	if (t)
	{
		int type = t->tretyp & COMMSK;

		type = t->tretyp & COMMSK;

		switch (type)
		{
			case TFND: {
				struct fndnod *f = fndptr(t);

				if (f->fndref > 0) {
					f->fndref--;
					return;
				}
				free(f->fndnam);
				freetree(f->fndval);
				break;
			}

			case TCOM:
				freeio(comptr(t)->comio);
				free_arg(comptr(t)->comarg);
				free_arg(comptr(t)->comset);
				break;

			case TFORK:
				freeio(forkptr(t)->forkio);
				freetree(forkptr(t)->forktre);
				break;

			case TPAR:
				freetree(parptr(t)->partre);
				break;

			case TFIL:
			case TLST:
			case TAND:
			case TORF:
				freetree(lstptr(t)->lstlef);
				freetree(lstptr(t)->lstrit);
				break;

			case TFOR:
			{
				struct fornod *f = (struct fornod *)t;

				free(f->fornam);
				freetree(f->fortre);
				if (f->forlst)
				{
					freeio(f->forlst->comio);
					free_arg(f->forlst->comarg);
					free_arg(f->forlst->comset);
					free(f->forlst);
				}
			}
			break;

			case TWH:
			case TUN:
				freetree(whptr(t)->whtre);
				freetree(whptr(t)->dotre);
				break;

			case TIF:
				freetree(ifptr(t)->iftre);
				freetree(ifptr(t)->thtre);
				freetree(ifptr(t)->eltre);
				break;

			case TSW:
				free(swptr(t)->swarg);
				freereg(swptr(t)->swlst);
				break;
		}
		free(t);
	}
}

static void
free_arg(argp)
	struct argnod	*argp;
{
	struct argnod	*sav;

	while (argp)
	{
		sav = argp->argnxt;
		free(argp);
		argp = sav;
	}
}

static void
freeio(iop)
	struct ionod	*iop;
{
	struct ionod *sav;

	while (iop)
	{
		if (iop->iofile & IODOC)
		{

#ifdef DEBUG
			prs("unlinking ");
			prs(iop->ioname);
			newline();
#endif

			unlink(iop->ioname);

			if (fiotemp == iop)
				fiotemp = iop->iolst;
			else
			{
				struct ionod *fiop = fiotemp;

				while (fiop->iolst != iop)
					fiop = fiop->iolst;

				fiop->iolst = iop->iolst;
			}
		}
		free(iop->ioname);
		free(iop->iolink);
		sav = iop->ionxt;
		free(iop);
		iop = sav;
	}
}

static void
freereg(regp)
	struct regnod	*regp;
{
	struct regnod	*sav;

	while (regp)
	{
		free_arg(regp->regptr);
		freetree(regp->regcom);
		sav = regp->regnxt;
		free(regp);
		regp = sav;
	}
}


static int nonl = 0;

static void
prbgnlst()
{
	if (nonl)
		prc_buff(SPACE);
	else
		prc_buff(NL);
}

static void
prendlst()
{
	if (nonl) {
		prc_buff(';');
		prc_buff(SPACE);
	}
	else
		prc_buff(NL);
}

void
prcmd(t)
	struct trenod	*t;
{
	nonl++;
	prf(t);
	nonl = 0;
}

void
prf(t)
	struct trenod	*t;
{
	sigchk();

	if (t)
	{
		int	type;

		type = t->tretyp & COMMSK;

		switch (type) {
			case TFND:
			{
				struct fndnod *f = (struct fndnod *)t;

				prs_buff(f->fndnam);
				prs_buff(UC "(){");
				prbgnlst();
				prf(f->fndval);
				prbgnlst();
				prs_buff(UC "}");
				break;
			}

			case TCOM:
#ifdef	PARSE_DEBUG
				prs_buff(UC "TCOM ");
#endif
				if (comptr(t)->comset) {
#ifdef	PARSE_DEBUG
				prs_buff(UC "COMSET ");
#endif
					prarg(comptr(t)->comset);
					prc_buff(SPACE);
				}
#ifdef	PARSE_DEBUG
				prs_buff(UC "COMARG ");
#endif
				prarg(comptr(t)->comarg);
				prio(comptr(t)->comio);
				break;

			case TFORK:
#ifdef	PARSE_DEBUG
				prs_buff(UC "TFORK ");
#endif
				prf(forkptr(t)->forktre);
				prio(forkptr(t)->forkio);
				if (forkptr(t)->forktyp & FAMP)
					prs_buff(UC " &");
				break;

			case TPAR:
#ifdef	PARSE_DEBUG
				prs_buff(UC "TPAR ");
#endif
				prs_buff(UC "(");
				prf(parptr(t)->partre);
				prs_buff(UC ")");
				break;

			case TFIL:
				prf(lstptr(t)->lstlef);
				prs_buff(UC " | ");
				prf(lstptr(t)->lstrit);
				break;

			case TLST:
				prf(lstptr(t)->lstlef);
				prendlst();
				prf(lstptr(t)->lstrit);
				break;

			case TAND:
				prf(lstptr(t)->lstlef);
				prs_buff(UC " && ");
				prf(lstptr(t)->lstrit);
				break;

			case TORF:
				prf(lstptr(t)->lstlef);
				prs_buff(UC " || ");
				prf(lstptr(t)->lstrit);
				break;

			case TFOR:
				{
					struct argnod	*arg;
					struct fornod	*f = (struct fornod *)t;

					prs_buff(UC "for ");
					prs_buff(f->fornam);

					if (f->forlst)
					{
						arg = f->forlst->comarg;
						prs_buff(UC " in");

						while (arg != ENDARGS)
						{
							prc_buff(SPACE);
							prs_buff(arg->argval);
							arg = arg->argnxt;
						}
					}

					prendlst();
					prs_buff(UC "do");
					prbgnlst();
					prf(f->fortre);
					prendlst();
					prs_buff(UC "done");
				}
				break;

			case TWH:
			case TUN:
				if (type == TWH)
					prs_buff(UC "while ");
				else
					prs_buff(UC "until ");
				prf(whptr(t)->whtre);
				prendlst();
				prs_buff(UC "do");
				prbgnlst();
				prf(whptr(t)->dotre);
				prendlst();
				prs_buff(UC "done");
				break;

			case TIF:
			{
				struct ifnod *f = (struct ifnod *)t;

				prs_buff(UC "if ");
				prf(f->iftre);
				prendlst();
				prs_buff(UC "then");
				prendlst();
				prf(f->thtre);

				if (f->eltre)
				{
					prendlst();
					prs_buff(UC "else");
					prendlst();
					prf(f->eltre);
				}

				prendlst();
				prs_buff(UC "fi");
				break;
			}

			case TSW:
				{
					struct regnod	*swl;

					prs_buff(UC "case ");
					prs_buff(swptr(t)->swarg);
					prs_buff(UC " in ");

					swl = swptr(t)->swlst;
					while (swl)
					{
						struct argnod	*arg;

						arg = swl->regptr;
						if (arg)
						{
							prs_buff(arg->argval);
							arg = arg->argnxt;
						}

						while (arg)
						{
							prs_buff(UC " | ");
							prs_buff(arg->argval);
							arg = arg->argnxt;
						}

						prs_buff(UC ")");
						prf(swl->regcom);
						prs_buff(UC ";;");
						swl = swl->regnxt;
					}
				}
				break;
			}
		}

	sigchk();
}

static void
prarg(argp)
	struct argnod	*argp;
{
#ifdef	PARSE_DEBUG
	int i = 0;
#endif
	while (argp) {
#ifdef	PARSE_DEBUG
		prs_buff(UC "AV[");
		prn_buff(i++);
		prs_buff(UC "]: ");
#endif
		prs_buff(argp->argval);
		argp = argp->argnxt;
		if (argp)
			prc_buff(SPACE);
	}
#ifdef	PARSE_DEBUG
	if (i > 0)
		prs_buff(UC "AVEND ");
#endif
}

static void
prio(iop)
	struct ionod	*iop;
{
	int	iof;
	unsigned char	*ion;

	while (iop)
	{
		iof = iop->iofile;
		ion = (unsigned char *) iop->ioname;

		if (*ion)
		{
			prc_buff(SPACE);

			prn_buff(iof & IOUFD);

			if (iof & IODOC)
				prs_buff(UC "<<");
			else if (iof & IOMOV)
			{
				if (iof & IOPUT)
					prs_buff(UC ">&");
				else
					prs_buff(UC "<&");

			} else if ((iof & IOPUT) == 0)
				prc_buff('<');
			else if (iof & IOAPP)
				prs_buff(UC ">>");
			else
				prc_buff('>');

			prs_buff(ion);
		}
		iop = iop->ionxt;
	}
}
