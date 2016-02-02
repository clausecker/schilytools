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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

#if defined(sun)
#pragma ident	"@(#)cmd.c	1.17	08/01/29 SMI"
#endif

#include "defs.h"

/*
 * Copyright 2008-2016 J. Schilling
 *
 * @(#)cmd.c	1.38 16/02/02 2008-2016 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)cmd.c	1.38 16/02/02 2008-2016 J. Schilling";
#endif

/*
 * UNIX shell
 */

#include	"sym.h"

static	unsigned char *getstor	__PR((int asize));
	struct trenod *makefork	__PR((int flgs, struct trenod *i));
static struct trenod *makelist	__PR((int type, struct trenod *i,
						struct trenod *r));
	struct trenod *cmd	__PR((int sym, int flg));
static	struct trenod *list	__PR((int flg));
static	struct trenod *term	__PR((int flg));
static	struct regnod *syncase	__PR((int esym));
static	struct trenod *item	__PR((BOOL flag));
static	int	skipnl		__PR((int flag));
static	struct ionod *inout	__PR((struct ionod *lastio));
static	void	chkword		__PR((void));
static	void	chksym		__PR((int sym));
static	void	prsym		__PR((int sym));
static	void	synbad		__PR((void));

int	abegin;

/* ======== storage allocation for functions ======== */

static unsigned char *
getstor(asize)
	int asize;
{
	if (fndef)
		return ((unsigned char *)alloc(asize));
	else
		return (getstak((Intptr_t)asize));
}


/* ========	command line decoding	======== */

struct trenod *
makefork(flgs, i)
	int	flgs;
	struct trenod *i;
{
	struct forknod *t;

	t = (struct forknod *)getstor(sizeof (struct forknod));
	t->forktyp = flgs|TFORK;
	t->forktre = i;
	t->forkio = 0;
	return ((struct trenod *)t);
}

static struct trenod *
makelist(type, i, r)
	int	type;
	struct trenod *i, *r;
{
	struct lstnod *t = 0;

	if (i == 0 || r == 0) {
		synbad();
	} else {
		t = (struct lstnod *)getstor(sizeof (struct lstnod));
		t->lsttyp = type;
		t->lstlef = i;
		t->lstrit = r;
	}
	return ((struct trenod *)t);
}

/*
 * cmd
 *	empty
 *	list
 *	list & [ cmd ]
 *	list [ ; cmd ]
 *
 * This is the main parser entry point that is called as cmd(NL, MTFLG)
 * from main.c::exfile(). MTFLG permits empty commands in the main loop.
 *
 * It cmd() is called with NLFLG in flg at top level, this causes the whole
 * file to be read at once and a single treenode * to be constructed from that.
 * This method is e.g. used by "eval" and ".".
 */
struct trenod *
cmd(sym, flg)
	int	sym;
	int		flg;
{
	struct trenod *i, *e;

	i = list(flg);
	if (wdval == NL)
	{
		if (flg & NLFLG)
		{
			wdval = ';';
			chkpr();
		}
	} else if (i == 0 && (flg & MTFLG) == 0) {
		synbad();
	}
	switch (wdval)
	{
	case '&':
		if (i)
			i = makefork(FAMP, i);
		else
			synbad();
		/* FALLTHROUGH */

	case ';':
		if ((e = cmd(sym, flg | MTFLG)) != NULL)
			i = makelist(TLST, i, e);
		else if (i == 0) {
			synbad();
		}
		break;

	case EOFSYM:
		if (sym == NL)
			break;
		/* FALLTHROUGH */

	default:
		if (sym)
			chksym(sym);
	}
	return (i);
}

/*
 * list
 *	term
 *	list && term
 *	list || term
 */
static struct trenod *
list(flg)
	int	flg;
{
	struct trenod *r;
	int		b;
	r = term(flg);
	while (r && ((b = (wdval == ANDFSYM)) != 0 || wdval == ORFSYM))
		r = makelist((b ? TAND : TORF), r, term(NLFLG));
	return (r);
}

/*
 * term
 *	item
 *	item |^ term
 */
static struct trenod *
term(flg)
	int	flg;
{
	struct trenod *t;

	abegin = 1;
	reserv++;
	if (flg & NLFLG)
		skipnl(0);
	else
		word();

#if defined(DO_NOTSYM) || defined(DO_TIME)
#if defined(DO_NOTSYM) && !defined(DO_TIME)
	if (wdval == NOTSYM) {
#else
#if defined(DO_TIME) && !defined(DO_NOTSYM)
	if (wdval == TIMSYM) {
#else
	if (wdval == NOTSYM || wdval == TIMSYM) {
#endif
#endif
		struct parnod	*p;

		p = (struct parnod *)getstor(sizeof (struct parnod));
		p->partyp = TTIME;
		if (wdval == NOTSYM)
			p->partyp = TNOT;
		p->partre = term(0);
		t = treptr(p);
	} else
#endif

	/*
	 * ^ is a relic from the days of UPPER CASE ONLY tty model 33s
	 */
	if ((t = item(TRUE)) != 0 && (wdval == '^' || wdval == '|'))
	{
		struct trenod	*left;
		struct trenod	*right;
		struct trenod	*tr;
		int		pio = wdnum & IOUFD;

		if (wdnum == 0)
			pio = STDOUT_FILENO;
		left = makefork(FPOU|pio, t);
		tr = term(NLFLG);
#if defined(DO_PIPE_SYNTAX_E) || defined(DO_PIPE_PARENT)
		if (tr == NULL)
			synbad();
#endif
#ifdef	DO_PIPE_PARENT
		/*
		 * Build a tree that allows us to make all pipe commands
		 * children from the main shell process.
		 *
		 * Special right nodes:
		 * -	TFORK	() Avoid to add another fork
		 * -	TFIL	Pipe to right pipe, avoid fork
		 */
		switch (tr->tretyp & COMMSK) {
		case TFORK:
			tr->tretyp |= FPIN;
			right = tr;
			break;

		case TFIL:
		case TCOM:
			right = makefork(FPIN, tr);
			right->tretyp |= TNOFORK;
			break;
		default:
			right = makefork(FPIN, tr);
		}
		if ((t->tretyp & COMMSK) == TCOM)
			left->tretyp |= TNOFORK;
		return (makelist(TFIL, left, right));
#else	/* !DO_PIPE_PARENT */
		right = makefork(FPIN, tr);
		return (makefork(0, makelist(TFIL, left, right)));
#endif
	}
	return (t);
}

/*
 * case statement, parse things after "case <word> in" here
 */
static struct regnod *
syncase(esym)
int	esym;
{
	skipnl(0);
	if (wdval == esym)
		return (0);
	else
	{
		struct regnod *r =
		    (struct regnod *)getstor(sizeof (struct regnod));
		struct argnod *argp;

		r->regptr = 0;
#ifdef	DO_POSIX_CASE
		if (wdval == '(')
			skipnl(0);
#endif
		for (;;)
		{
			if (fndef)
			{
				argp = wdarg;
				wdarg = (struct argnod *)
						alloc(length(argp->argval) +
								BYTESPERWORD);
				movstr(argp->argval, wdarg->argval);
			}

			wdarg->argnxt = r->regptr;
			r->regptr = wdarg;

			/* 'in' is not a reserved word in this case */
			if (wdval == INSYM) {
				wdval = 0;
			}
			if (wdval || (word() != ')' && wdval != '|'))
				synbad();
			if (wdval == '|')
				word();
			else
				break;
		}
		r->regcom = cmd(0, NLFLG | MTFLG);
		if (wdval == ECSYM)
			r->regnxt = syncase(esym);
		else
		{
			chksym(esym);
			r->regnxt = 0;
		}
		return (r);
	}
}

/*
 * item
 *
 *	( cmd ) [ < in  ] [ > out ]
 *	word word* [ < in ] [ > out ]
 *	if ... then ... else ... fi
 *	for ... while ... do ... done
 *	case ... in ... esac
 *	begin ... end
 */
#ifdef	PROTOTYPES
static struct trenod *
item(BOOL flag)
#else
static struct trenod *
item(flag)
	BOOL	flag;
#endif
{
	struct trenod *r;
	struct ionod *io;

	if (flag)
		io = inout((struct ionod *)0);
	else
		io = 0;
	abegin--;
	switch (wdval)
	{
	case CASYM:
		{
			struct swnod *t;

			t = (struct swnod *)getstor(sizeof (struct swnod));
			r = (struct trenod *)t;

			chkword();
			if (fndef)
				t->swarg = make(wdarg->argval);
			else
				t->swarg = wdarg->argval;
			skipnl(0);
			chksym(INSYM | BRSYM);
			t->swlst = syncase(wdval == INSYM ? ESSYM : KTSYM);
			t->swtyp = TSW;
			break;
		}

	case IFSYM:
		{
			int	w;
			struct ifnod *t;

			t = (struct ifnod *)getstor(sizeof (struct ifnod));
			r = (struct trenod *)t;

			t->iftyp = TIF;
			t->iftre = cmd(THSYM, NLFLG);
			t->thtre = cmd(ELSYM | FISYM | EFSYM, NLFLG);
			t->eltre = ((w = wdval) == ELSYM ?
					cmd(FISYM, NLFLG) :
						(w == EFSYM ?
						(wdval = IFSYM, item(0)) : 0));
			if (w == EFSYM)
				return (r);
			break;
		}

	case FORSYM:
		{
			struct fornod *t;

			t = (struct fornod *)getstor(sizeof (struct fornod));
			r = (struct trenod *)t;

			t->fortyp = TFOR;
			t->forlst = 0;
			chkword();
			if (fndef)
				t->fornam = make(wdarg->argval);
			else
				t->fornam = wdarg->argval;
			if (skipnl(1) == INSYM)
			{
				chkword();

				nohash++;
				t->forlst = (struct comnod *)item(0);
				nohash--;

				if (wdval != NL && wdval != ';')
					synbad();
				if (wdval == NL)
					chkpr();
				skipnl(0);
#ifdef	DO_POSIX_FOR
			} else if (wdval == ';') {
				/*
				 * "for i; do cmd ...; done" is valid syntax
				 * see Austin bug #581
				 */
				skipnl(0);
#endif
			}
			chksym(DOSYM | BRSYM);
			t->fortre = cmd(wdval == DOSYM ? ODSYM : KTSYM, NLFLG);
			break;
		}

	case WHSYM:
	case UNSYM:
		{
			struct whnod *t;

			t = (struct whnod *)getstor(sizeof (struct whnod));
			r = (struct trenod *)t;

			t->whtyp = (wdval == WHSYM ? TWH : TUN);
			t->whtre = cmd(DOSYM, NLFLG);
			t->dotre = cmd(ODSYM, NLFLG);
			break;
		}

	case BRSYM:
		r = cmd(KTSYM, NLFLG);
		break;

	case '(':
		{
			struct parnod *p;

			p = (struct parnod *)getstor(sizeof (struct parnod));
			p->partre = cmd(')', NLFLG);
			p->partyp = TPAR;
			r = makefork(0, (struct trenod *)p);
			break;
		}

	default:
		if (io == 0)
			return (0);
		/* FALLTHROUGH */

	case 0:
		{
			struct comnod *t;
			struct argnod *argp;
			struct argnod **argtail;
			struct argnod **argset = 0;
#ifndef	ARGS_RIGHT_TO_LEFT
			struct argnod **argstail = (struct argnod **)&argset;
#endif
			int	keywd = 1;
			unsigned char	*com;

			if ((wdval != NL) && ((peekn = skipwc()) == '('))
			{
				struct fndnod *f;
				struct ionod  *saveio;

				saveio = iotemp;
				peekn = 0;
				if (skipwc() != ')')
					synbad();

				/*
				 * We increase fndef before calling getstor(),
				 * so that getstor() uses malloc to allocate
				 * memory instead of stack. This is necessary
				 * since fndnod will be hung on np->namenv,
				 * which persists over command executions.
				 */
				fndef++;
				f = (struct fndnod *)
					getstor(sizeof (struct fndnod));
				r = (struct trenod *)f;

				f->fndtyp = TFND;
				if (fndef)
					f->fndnam = make(wdarg->argval);
				else
					f->fndnam = wdarg->argval;
				f->fndref = 0;
				reserv++;
				skipnl(0);
				f->fndval = (struct trenod *)item(0);
				fndef--;

				if (iotemp != saveio)
				{
					struct ionod	*ioptr = iotemp;

					while (ioptr->iolst != saveio)
						ioptr = ioptr->iolst;

					ioptr->iolst = fiotemp;
					fiotemp = iotemp;
					iotemp = saveio;
				}
				return (r);
			}
			else
			{
				int envbeg = 0;

				t = (struct comnod *)
					getstor(sizeof (struct comnod));
				r = (struct trenod *)t;

				t->comio = io; /* initial io chain */
				argtail = &(t->comarg);

				while (wdval == 0)
				{
					if (fndef)
					{
						argp = wdarg;
						wdarg = (struct argnod *)
						    alloc(length(argp->argval) +
								BYTESPERWORD);
						movstr(argp->argval,
								wdarg->argval);
					}

					argp = wdarg;
					if (wdset && keywd)
					{
						/*
						 * Revert the effect of abegin--
						 * at the begin of this function
						 * in case that we are in a list
						 * of env= definitions.
						 */
						if (abegin == 0) {
							abegin++;
							envbeg++;
						}
#ifdef	ARGS_RIGHT_TO_LEFT		/* old order: var2=val2 var1=val1 */
						argp->argnxt =
							(struct argnod *)argset;
						argset = (struct argnod **)argp;
#else
						argp->argnxt =
							(struct argnod *)0;
						*argstail = argp;
						argstail = &argp->argnxt;
#endif
					}
					else
					{
						/*
						 * If we had env= definitions,
						 * make sure to decrement abegin
						 * To disable begin alias
						 * expansions.
						 */
						if (abegin > 0 && envbeg)
							abegin--;
						*argtail = argp;
						argtail = &(argp->argnxt);
						keywd = flags & keyflg;
					}
					word();
					if (flag)
					{
						if (io) {
							while (io->ionxt)
								io = io->ionxt;
							io->ionxt = inout(
							    (struct ionod *)0);
						} else {
							t->comio = io = inout(
							    (struct ionod *)0);
						}
					}
				}

				t->comtyp = TCOM;
				t->comset = (struct argnod *)argset;
				*argtail = 0;

				if (nohash == 0 &&
				    (fndef == 0 || (flags & hashflg))) {
					if (t->comarg) {
						com = t->comarg->argval;
						if (*com && *com != DOLLAR) {
							pathlook(com, 0,
								t->comset);
						}
					}
				}

				return (r);
			}
		}

	}
	reserv++;
	word();
	if ((io = inout(io)) != NULL)
	{
		r = makefork(0, r);
		r->treio = io;
	}
	return (r);
}

/* ARGSUSED */
static int
skipnl(flag)
	int	flag;
{
	while ((reserv++, word() == NL))
		chkpr();
#ifdef	DO_PIPE_SEMI_SYNTAX_E
	if (!flag && wdval == ';')
		synbad();
#endif
	return (wdval);
}

static struct ionod *
inout(lastio)
	struct ionod *lastio;
{
	int	iof;
	struct ionod *iop;
	unsigned int	c;
	int	obegin = abegin;

	iof = wdnum;
	switch (wdval)
	{
	case DOCSYM:	/*	<<	*/
		iof |= IODOC|IODOC_SUBST;
		break;

	case APPSYM:	/*	>>	*/
	case '>':
		if (wdnum == 0)
			iof |= 1;
		iof |= IOPUT;
		if (wdval == APPSYM)
		{
			iof |= IOAPP;
			break;
		}
#ifdef	DO_NOCLOBBER
		else {
			if ((c = nextwc()) == '|')
				iof |= IOCLOB;
			else
				peekn = c | MARK;
		}
#endif
		/* FALLTHROUGH */

	case '<':
		if ((c = nextwc()) == '&')
			iof |= IOMOV;
		else if (c == '>')
			iof |= IORDW;
		else
			peekn = c | MARK;
		break;

	default:
		return (lastio);
	}

	abegin = 0;
	chkword();
	abegin = obegin;
	iop = (struct ionod *)getstor(sizeof (struct ionod));

	if (fndef)
		iop->ioname = (char *)make(wdarg->argval);
	else
		iop->ioname = (char *)(wdarg->argval);

	iop->iolink = 0;
	iop->iofile = iof;
	if (iof & IODOC)
	{
		iop->iolst = iopend;
		iopend = iop;
	}
	word();
	iop->ionxt = inout(lastio);
	return (iop);
}

static void
chkword()
{
	if (word())
		synbad();
}

static void
chksym(sym)
int sym;
{
	int	x = sym & wdval;

	if (((x & SYMFLG) ? x : sym) != wdval)
		synbad();
}

static void
prsym(sym)
int sym;
{
	if (sym & SYMFLG) {
		const struct sysnod *sp = reserved;

		while (sp->sysval && sp->sysval != sym)
			sp++;
		prs((unsigned char *)sp->sysnam);
	} else if (sym == EOFSYM) {
		prs(_gettext(endoffile));
	} else {
		if (sym & SYMREP)
			prc(sym);
		if (sym == NL)
			prs(_gettext(nlorsemi));
		else
			prc(sym);
	}
}

static void
synbad()
{
	prp();
	prs(_gettext(synmsg));
	if ((flags & ttyflg) == 0) {
		prs(_gettext(atline));
		prn(standin->flin);
	}
	prs((unsigned char *)colon);
	prc(LQ);
	if (wdval)
		prsym(wdval);
	else
		prs_cntl(wdarg->argval);
	prc(RQ);
	prs(_gettext(unexpected));
	newline();
	exitsh(SYNBAD);
}
