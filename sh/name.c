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
#pragma ident	"@(#)name.c	1.25	06/06/16 SMI"
#endif

#include "defs.h"

/*
 * Copyright 2008-2016 J. Schilling
 *
 * @(#)name.c	1.49 16/02/07 2008-2016 J. Schilling
 */
#ifndef lint
static	UConst char sccsid[] =
	"@(#)name.c	1.49 16/02/07 2008-2016 J. Schilling";
#endif

/*
 * UNIX shell
 */

#ifdef INTERACTIVE
#include	<schily/shedit.h>
#endif
#ifdef	HAVE_STROPTS_H
#include	<stropts.h>
#endif

extern int	mailchk;

	int	syslook		__PR((unsigned char *w,
					const struct sysnod syswds[], int n));
	const struct sysnod *
		sysnlook	__PR((unsigned char *w,
					const struct sysnod syswds[], int n));
	void	setlist		__PR((struct argnod *arg, int xp));
	void	setname		__PR((unsigned char *, int));
	void	replace		__PR((unsigned char **a, unsigned char *v));
	void	dfault		__PR((struct namnod *n, unsigned char *v));
	void	assign		__PR((struct namnod *n, unsigned char *v));
static void	set_builtins_path	__PR((void));
static int	patheq		__PR((unsigned char *component, char *dir));
	int	readvar		__PR((int namec, unsigned char **names));
	void	assnum		__PR((unsigned char **p, long i));
unsigned char	*make		__PR((unsigned char *v));
struct namnod	*lookup		__PR((unsigned char *nam));
static	BOOL	chkid		__PR((unsigned char *nam));
	void	namscan		__PR((void (*fn)(struct namnod *n)));
static void	namwalk		__PR((struct namnod *));
	void	printnam	__PR((struct namnod *n));
#ifdef	DO_LINENO
	unsigned char *linenoval __PR((void));
#endif
	void	printro		__PR((struct namnod *n));
	void	printpro	__PR((struct namnod *n));
	void	printexp	__PR((struct namnod *n));
#if !defined(NO_VFORK) || defined(DO_POSIX_SPEC_BLTIN)
static void	pushval		__PR((struct namnod *n));
	void	popval		__PR((struct namnod *n));
#endif
	void	setup_env	__PR((void));
static void	countnam	__PR((struct namnod *n));
static void	pushnam		__PR((struct namnod *n));
	unsigned char **local_setenv __PR((int flg));
	struct namnod *findnam	__PR((unsigned char *nam));
	void	unset_name	__PR((unsigned char *name, int uflg));
static void	dolocale	__PR((char *nm));

#ifndef	HAVE_ISASTREAM
static	int	isastream	__PR((int fd));
#endif
#ifdef INTERACTIVE
	char	*getcurenv	__PR((char *name));
	void	ev_insert	__PR((char *name));
#endif

struct namnod ps2nod =			/* PS2= */
{
	(struct namnod *)NIL,
	&acctnod,
	(struct namnod *)NIL,
	(unsigned char *)ps2name
};
struct namnod envnod =			/* ENV= */
{
	(struct namnod *)NIL,
	(struct namnod *)NIL,
	(struct namnod *)NIL,
	(unsigned char *)envname
};
struct namnod cdpnod =			/* CDPATH= */
{
	(struct namnod *)NIL,
	&envnod,
	(struct namnod *)NIL,
	(unsigned char *)cdpname
};
struct namnod ppidnod =			/* PPID= */
{
	(struct namnod *)NIL,
	(struct namnod *)NIL,
	(struct namnod *)NIL,
	(unsigned char *)ppidname
};
struct namnod pathnod =			/* PATH= */
{
	&mailpnod,
	&ppidnod,
	(struct namnod *)NIL,
	(unsigned char *)pathname
};
struct namnod ifsnod =			/* IFS= */
{
	&homenod,
	&mailnod,
	(struct namnod *)NIL,
	(unsigned char *)ifsname
};
struct namnod ps1nod =			/* PS1= */
{
	&pathnod,
	&ps2nod,
	(struct namnod *)NIL,
	(unsigned char *)ps1name
};
struct namnod ps4nod =			/* PS4= */
{
	(struct namnod *)NIL,
	(struct namnod *)NIL,
	(struct namnod *)NIL,
	(unsigned char *)ps4name
};
struct namnod ps3nod =			/* PS3= */
{
	(struct namnod *)NIL,
	&ps4nod,
	(struct namnod *)NIL,
	(unsigned char *)ps3name
};
struct namnod homenod =			/* HOME= */
{
	&cdpnod,
	(struct namnod *)NIL,
	(struct namnod *)NIL,
	(unsigned char *)homename
};
struct namnod linenonod =		/* LINENO= */
{
	(struct namnod *)NIL,
	(struct namnod *)NIL,
	(struct namnod *)NIL,
	(unsigned char *)linenoname
};
struct namnod mailnod =			/* MAIL= */
{
	&linenonod,
	(struct namnod *)NIL,
	(struct namnod *)NIL,
	(unsigned char *)mailname
};
struct namnod mchknod =			/* MAILCHECK= */
{
	&ifsnod,
	&ps1nod,
	(struct namnod *)NIL,
	(unsigned char *)mchkname
};
struct namnod pwdnod =			/* PWD= */
{
	&ps3nod,
	(struct namnod *)NIL,
	(struct namnod *)NIL,
	(unsigned char *)pwdname,
};
struct namnod opwdnod =			/* OLDPWD= */
{
	(struct namnod *)NIL,
	(struct namnod *)NIL,
	(struct namnod *)NIL,
	(unsigned char *)opwdname,
};
struct namnod timefmtnod =		/* TIMEFORMAT= */
{
	(struct namnod *)NIL,
	(struct namnod *)NIL,
	(struct namnod *)NIL,
	(unsigned char *)timefmtname,
};
struct namnod acctnod =			/* SHACCT= */
{
	&pwdnod,
	&timefmtnod,
	(struct namnod *)NIL,
	(unsigned char *)acctname
};
struct namnod mailpnod =		/* MAILPATH= */
{
	(struct namnod *)NIL,
	&opwdnod,
	(struct namnod *)NIL,
	(unsigned char *)mailpname
};


struct namnod *namep = &mchknod;

/* ========	variable and string handling	======== */

/*
 * Return numeric identifier for reserved words and builtin commands.
 */
int
syslook(w, syswds, n)
	unsigned char	*w;
	const struct sysnod	syswds[];
	int		n;
{
	const struct sysnod	*res = sysnlook(w, syswds, n);

	if (res == NULL)
		return (0);
	return (res->sysval);
}

/*
 * Lookup function for reserved words and builtin commands,
 * return sysnod structure.
 */
const struct sysnod *
sysnlook(w, syswds, n)
	unsigned char	*w;
	const struct sysnod	syswds[];
	int		n;
{
	int	low;
	int	high;
	int	mid;
	int	cond;

	if (w == 0 || *w == 0)
		return (0);

	low = 0;
	high = n - 1;

	while (low <= high)
	{
		mid = (low + high) / 2;

		if ((cond = cf(w, (unsigned char *)syswds[mid].sysnam)) < 0)
			high = mid - 1;
		else if (cond > 0)
			low = mid + 1;
		else
			return (&syswds[mid]);
	}
	return (0);
}

/*
 * Set up the list of local shell variable definitions to be
 * exported for the next command.
 */
void
setlist(arg, xp)
	struct argnod	*arg;
	int		xp;
{
	if (flags & exportflg)
		xp |= N_EXPORT;

	while (arg)
	{
		unsigned char *s = mactrim(arg->argval);
		setname(s, xp);
		arg = arg->argnxt;
		if (flags & execpr)
		{
			prs(s);
			if (arg)
				blank();
			else
				newline();
		}
	}
}

/*
 * does parameter assignments for cases when a NAME=value string exists
 */
void
setname(argi, xp)
	unsigned char	*argi;
	int		xp;
{
	unsigned char *argscan = argi;
	struct namnod *n;

	if (letter(*argscan))
	{
		while (alphanum(*argscan))
			argscan++;

		if (*argscan == '=')
		{
			*argscan = 0;	/* make name a cohesive string */

			n = lookup(argi);
			*argscan++ = '=';
#ifdef	DO_POSIX_EXPORT
			if ((n->namflg & N_FUNCTN) && (xp & N_EXPORT))
				error(badexport);
#endif

			if (xp & N_ENVNAM)
			{
				n->namenv = n->namval = argscan;
				if (n == &pathnod)
					set_builtins_path();
			} else {
#if !defined(NO_VFORK) || defined(DO_POSIX_SPEC_BLTIN)
				if (xp & N_PUSHOV)
					pushval(n);
#endif
				assign(n, argscan);
			}
			attrib(n, xp);	/* readonly attrib after assignement */

			dolocale((char *)n->namid);
			return;
		}
	}
}

void
replace(a, v)
	unsigned char	**a;
	unsigned char	*v;
{
	free(*a);
	*a = make(v);
}

/*
 * Assign a default value to a shell variable that is currently unset.
 */
void
dfault(n, v)
	struct namnod	*n;
	unsigned char	*v;
{
	if (n->namval == 0)
		assign(n, v);
}

/*
 * Unconditionally assign a value to a shell variable.
 */
void
assign(n, v)
	struct namnod	*n;
	unsigned char	*v;
{
	if (n->namflg & N_RDONLY)
		failed(n->namid, wtfailed);

#ifndef RES

	else if (flags & rshflg)
	{
		if (n == &pathnod || eq(n->namid, "SHELL"))
			failed(n->namid, restricted);
	}
#endif

#ifndef	DO_POSIX_UNSET
	else if (n->namflg & N_FUNCTN)
	{
		func_unhash(n->namid);
		freefunc(n);

		n->funcval = 0;
		n->namflg = N_DEFAULT;
	}
#endif
	if (n == &mchknod)
	{
		mailchk = stoi(v);
	}

	replace(&n->namval, v);
	attrib(n, N_ENVCHG);

	if (n == &pathnod)
	{
		zaphash();
		set_dotpath();
		set_builtins_path();
		return;
	}

	if (flags & prompt)
	{
		if ((n == &mailpnod) ||
		    (n == &mailnod && mailpnod.namflg == N_DEFAULT)) {
			setmail(n->namval);
		}
	}
}

static void
set_builtins_path()
{
	unsigned char *path;

	ucb_builtins = 0;
	path = getpath((unsigned char *)"");
	while (path && *path) {
		if (patheq(path, "/usr/ucb")) {
			ucb_builtins++;
			break;
		} else if (patheq(path, "/usr/bin"))
			break;
		else if (patheq(path, "/bin"))
			break;
		else if (patheq(path, "/usr/5bin"))
			break;
		path = nextpath(path);
	}
}

static int
patheq(component, dir)
	unsigned char	*component;
	char		*dir;
{
	unsigned char	c;

	for (;;) {
		c = *component++;
		if (c == COLON)
			c = '\0';	/* end of component of path */
		if (c != *dir++)
			return (0);
		if (c == '\0')
			return (1);
	}
}

/*
 * The read(1) builtin.
 */
int
readvar(namec, names)
	int		namec;
	unsigned char	**names;
{
	struct fileblk	fb;
	struct fileblk *f = &fb;
	unsigned char	c[MULTI_BYTE_MAX+1];
	int	rc = 0;
	struct namnod	*n;
	unsigned char	*rel;
	unsigned char *oldstak;
	unsigned char *pc, *rest;
	int		d;
	unsigned int	(*nextwchar)__PR((void));
	unsigned char	*ifs;

#ifdef	DO_READ_R
	struct optv	optv;
	int		ch;
	unsigned char	*cmdp = *names;

	optinit(&optv);

	nextwchar = nextwc;
	while ((ch = optnext(namec, names, &optv, "r",
			    "read [-r] name ...")) != -1) {
		if (ch == 0)	/* Was -help */
			return (1);
		else if (ch == 'r')
			nextwchar = readwc;
	}
	names += optv.optind;
	if (*names == NULL) {
		Failure(cmdp, mssgargn);
		return (1);
	}
#else
	names++;
	nextwchar = nextwc;
#endif
	ifs = ifsnod.namval;
	if (ifs == NULL)
		ifs = (unsigned char *)sptbnl;

	n = lookup(*names++);		/* done now to avoid storage mess */
	rel = (unsigned char *)relstak();

	push(f);
	initf(dup(STDIN_FILENO));

	/*
	 * If stdin is a pipe then this lseek(2) will fail with ESPIPE, so
	 * the read buffer size is set to 1 because we will not be able
	 * lseek(2) back towards the beginning of the file, so we have
	 * to read a byte at a time instead
	 *
	 */
	if (lseek(STDIN_FILENO, (off_t)0, SEEK_CUR) == -1)
		f->fsiz = 1;

	/*
	 * If stdin is a socket then this isastream(3C) will return 1, so
	 * the read buffer size is set to 1 because we will not be able
	 * lseek(2) back towards the beginning of the file, so we have
	 * to read a byte at a time instead
	 *
	 */
	if (isastream(STDIN_FILENO) == 1)
		f->fsiz = 1;

	/*
	 * strip leading IFS characters
	 */
	for (;;)
	{
		d = nextwchar();
		if (eolchar(d))
			break;
		rest = readw(d);
		pc = c;
		while ((*pc++ = *rest++) != '\0')
			/* LINTED */
			;
		if (!anys(c, ifs))
			break;
	}

	oldstak = curstak();
	for (;;)
	{
		if ((*names && anys(c, ifs)) || eolchar(d))
		{
			GROWSTAKTOP();
			zerostak();
#ifdef	DO_READ_ALLEXPORT
			if (flags & exportflg)
				n->namflg |= N_EXPORT;
#endif
			assign(n, absstak(rel));
			setstak(rel);
			if (*names)
				n = lookup(*names++);
			else
				n = 0;
			if (eolchar(d)) {
				break;
			} else		/* strip imbedded IFS characters */
				/* CONSTCOND */
				while (1) {
					d = nextwchar();
					if (eolchar(d))
						break;
					rest = readw(d);
					pc = c;
					while ((*pc++ = *rest++) != '\0')
						/* LINTED */
						;
					if (!anys(c, ifs))
						break;
				}
		}
		else
		{
			if (d == '\\' && nextwchar == nextwc) {
				d = readwc();
				rest = readw(d);
				while ((d = *rest++) != '\0') {
					GROWSTAKTOP();
					pushstak(d);
				}
				oldstak = staktop;
			}
			else
			{
				pc = c;
				while ((d = *pc++) != '\0') {
					GROWSTAKTOP();
					pushstak(d);
				}
				if (!anys(c, ifs))
					oldstak = staktop;
			}
			d = nextwchar();

			if (eolchar(d))
				staktop = oldstak;
			else
			{
				rest = readw(d);
				pc = c;
				while ((*pc++ = *rest++) != '\0')
					/* LINTED */
					;
			}
		}
	}
	while (n)
	{
#ifdef	DO_READ_ALLEXPORT
		if (flags & exportflg)
			n->namflg |= N_EXPORT;
#endif
		assign(n, (unsigned char *)nullstr);
		if (*names)
			n = lookup(*names++);
		else
			n = 0;
	}

	if (eof)
		rc = 1;

	if (isastream(STDIN_FILENO) != 1)
		/*
		 * If we are reading on a stream do not attempt to
		 * lseek(2) back towards the start because this is
		 * logically meaningless, but there is nothing in
		 * the standards to pervent the stream implementation
		 * from attempting it and breaking our code here
		 *
		 */
		lseek(STDIN_FILENO, (off_t)(f->nxtoff - f->endoff), SEEK_CUR);

	pop();
	return (rc);
}

void
assnum(p, i)
	unsigned char	**p;
	long		i;
{
	int j = ltos(i);
	replace(p, &numbuf[j]);
}

unsigned char *
make(v)
unsigned char	*v;
{
	unsigned char	*p;

	if (v)
	{
		movstr(v, p = (unsigned char *)alloc(length(v)));
		return (p);
	}
	else
		return (0);
}

/*
 * Lookup a shell variable and allocate a new node if the
 * variable does not yet exist.
 */
struct namnod *
lookup(nam)
	unsigned char	*nam;
{
	struct namnod *nscan = namep;
	struct namnod **prev = NULL;	/* Make stupid GCC happy */
	int		LR;

	if (!chkid(nam))
		failed(nam, notid);

	while (nscan)
	{
		if ((LR = cf(nam, nscan->namid)) == 0)
			return (nscan);

		else if (LR < 0)
			prev = &(nscan->namlft);
		else
			prev = &(nscan->namrgt);
		nscan = *prev;
	}
	/*
	 * add name node
	 */
	nscan = (struct namnod *)alloc(sizeof (*nscan));
	nscan->namlft = nscan->namrgt = nscan->nampush = (struct namnod *)NIL;
	nscan->namid = make(nam);
	nscan->namval = 0;
	nscan->namflg = N_DEFAULT;
	nscan->namenv = 0;
#ifdef	DO_POSIX_UNSET
	nscan->funcval = 0;
#endif

#ifdef	NAME_DEBUG
	/*
	 * Allow to set up handcrafted pointers in *nod structures.
	 */
	printf("prev = %p name '%s' ME %p\n", prev, nam, nscan);
#endif
	return (*prev = nscan);
}

/*
 * A valid shell variable name starts with a letter and
 * contains only letters or digits.
 */
static BOOL
chkid(nam)
unsigned char	*nam;
{
	unsigned char *cp = nam;

	if (!letter(*cp))
		return (FALSE);
	else
	{
		while (*++cp)
		{
			if (!alphanum(*cp))
				return (FALSE);
		}
	}
	return (TRUE);
}

static void (*namfn) __PR((struct namnod *n));
static int    namflg;

void
namscan(fn)
	void	(*fn) __PR((struct namnod *n));
{
	namfn = fn;
	namwalk(namep);
}

static void
namwalk(np)
	struct namnod	*np;
{
	if (np)
	{
		namwalk(np->namlft);
		(*namfn)(np);
		namwalk(np->namrgt);
	}
}

void
printnam(n)
	struct namnod	*n;
{
	unsigned char	*s;

	sigchk();

	if (n->namflg & N_FUNCTN) {
		struct fndnod *f = fndptr(n->funcval);

		prs_buff(n->namid);
		prs_buff((unsigned char *)"(){\n");
		if (f != NULL)
			prf((struct trenod *)f->fndval);
		prs_buff((unsigned char *)"\n}\n");
#ifndef	DO_POSIX_UNSET
		return;
#endif
	}
#ifdef	DO_LINENO
	if (n == &linenonod) {
		prs_buff(n->namid);
		prc_buff('=');
		prs_buff(linenoval());
		prc_buff(NL);
	} else
#endif
	if ((s = n->namval) != NULL) {
		prs_buff(n->namid);
		prc_buff('=');
#ifdef	DO_POSIX_UNSET
		qprs_buff(s);
#else
		prs_buff(s);
#endif
		prc_buff(NL);
	}
}

#ifdef	DO_LINENO
unsigned char *
linenoval()
{
	struct fileblk *f = standin;

#ifdef	LINENO_DEBUG
	while (1) {
		printf("F %p des %d\n", f, f->fdes);
		if (f->fstak == NULL)
			break;
		f = f->fstak;
	}
	f = standin;
#endif
	if (f->fstak != NULL)
		f = f->fstak;
	/*
	 * Subtract 1 as parsing of the current
	 * was already done.
	 */
	itos(f->flin - 1);
	return (numbuf);
}
#endif

static int namec;

void
printro(n)
	struct namnod	*n;
{
	if (n->namflg & N_RDONLY)
	{
		prs_buff(_gettext(readonly));
		prc_buff(SPACE);
		prs_buff(n->namid);
		prc_buff(NL);
	}
}

#ifdef	DO_POSIX_EXPORT
void
printpro(n)
	struct namnod	*n;
{
	if (n->namflg & N_RDONLY)
	{
		unsigned char	*s;

		prs_buff(UC readonly);
		prc_buff(SPACE);
		prs_buff(n->namid);
		if ((s = n->namval) != NULL) {
			prs_buff(UC "='");
			prs_buff(s);
			prc_buff('\'');
		}
		prc_buff(NL);
	}
}
#endif

void
printexp(n)
	struct namnod	*n;
{
	if (n->namflg & N_EXPORT)
	{
		prs_buff(_gettext(export));
		prc_buff(SPACE);
		prs_buff(n->namid);
		prc_buff(NL);
	}
}

#ifdef	DO_POSIX_EXPORT
void
printpexp(n)
	struct namnod	*n;
{
	if (n->namflg & N_EXPORT)
	{
		unsigned char	*s;

		prs_buff(UC export);
		prc_buff(SPACE);
		prs_buff(n->namid);
		if ((s = n->namval) != NULL) {
			prs_buff(UC "='");
			prs_buff(s);
			prc_buff('\'');
		}
		prc_buff(NL);
	}
}
#endif

#if !defined(NO_VFORK) || defined(DO_POSIX_SPEC_BLTIN)
/*
 * Push the current value by dulicating the content in
 * preparation of a later change of the node value.
 */
static void
pushval(n)
	struct namnod	*n;
{
	if (n->namval) {
		struct namnod *nscan = (struct namnod *)alloc(sizeof (*nscan));

		*nscan = *n;
		n->namval = make(n->namval);
		n->nampush = nscan;
	}
	attrib(n, N_PUSHOV);
}

/*
 * If the node has a pushed value, restore the original value.
 */
void
popval(n)
	struct namnod	*n;
{
	if ((n->namflg & N_PUSHOV) == 0)
		return;

	if (n->nampush) {
		struct namnod	*p = n->nampush;

		if (n->namval)
			free(n->namval);
		n->namval = p->namval;
		n->namflg = p->namflg;
		n->nampush = 0;
		free(p);
	} else {
		if (n->namval)
			free(n->namval);
		n->namval = 0;
		n->namflg = N_DEFAULT;
	}
}
#endif

void
setup_env()
{
	unsigned char **e = (unsigned char **)environ;

	while (*e)
		setname(*e++, N_ENVNAM);
}


static unsigned char **argnam;

static void
countnam(n)
	struct namnod	*n;
{
	if (n->namval)
		namec++;
}

/*
 * Set up a single environ entry for a new external command, called only
 * local_setenv().
 */
static void
pushnam(n)
	struct namnod	*n;
{
	int	flg = n->namflg;
	unsigned char	*p;
	unsigned char	*namval;

#ifndef	DO_POSIX_UNSET
	if (((flg & N_ENVCHG) && (flg & N_EXPORT)) || (flg & N_FUNCTN)) {
#else
	if ((flg & N_ENVCHG) && (flg & N_EXPORT)) {
#endif
		namval = n->namval;
	} else {
		/* Discard Local variable in child process */
		if (!(flg & ~N_ENVCHG)) {
			if (!(namflg & ENV_NOFREE)) {
				n->namflg = 0;
				n->namenv = 0;
				if (n->namval) {
					/* Release for re-use */
					free(n->namval);
					n->namval = (unsigned char *)NIL;
				}
			}
			namval = (unsigned char *)NIL;
		} else {
			namval = n->namenv;
		}
	}

	if (namval) {
		p = movstrstak(n->namid, staktop);
		p = movstrstak((unsigned char *)"=", p);
		p = movstrstak(namval, p);
		*argnam++ =
			getstak((Intptr_t)(p + 1 - (unsigned char *)(stakbot)));
	}
}

/*
 * Prepare the environ for a new external command.
 */
unsigned char **
local_setenv(flg)
	int	flg;
{
	unsigned char	**er;

	namec = 0;
	namscan(countnam);

	argnam = er = (unsigned char **)getstak((Intptr_t)namec * BYTESPERWORD +
								BYTESPERWORD);
	namflg = flg;
	namscan(pushnam);
	namflg = 0;
	*argnam++ = 0;
	return (er);
}

struct namnod *
findnam(nam)
	unsigned char	*nam;
{
	struct namnod	*nscan = namep;
	int		LR;

	if (!chkid(nam))
		return (0);
	while (nscan)
	{
		if ((LR = cf(nam, nscan->namid)) == 0)
			return (nscan);
		else if (LR < 0)
			nscan = nscan->namlft;
		else
			nscan = nscan->namrgt;
	}
	return (0);
}

void
unset_name(name, uflg)
	unsigned char	*name;
	int		uflg;
{
	struct namnod	*n;
	unsigned char	call_dolocale = 0;

	if ((n = findnam(name)) != NULL)
	{
		if (n->namflg & N_RDONLY) {
#ifdef	DO_POSIX_UNSET
			if (uflg == 0 || uflg == UNSET_VAR)
#endif
				failed(name, wtfailed);
		}

#ifndef	DO_POSIX_UNSET
		if (n == &pathnod ||
		    n == &ifsnod ||
		    n == &ps1nod ||
		    n == &ps2nod ||
		    n == &mchknod)
		{
			failed(name, badunset);
		}
#else
		if (n == &mchknod)
			mailchk = 0;
#endif

#ifndef RES

		if ((flags & rshflg) && eq(name, "SHELL"))
			failed(name, restricted);

#endif

#ifndef	DO_POSIX_UNSET
		if (n->namflg & N_FUNCTN) {
			func_unhash(name);
			freefunc(n);
		} else {
			call_dolocale++;
			free(n->namval);
			free(n->namenv);
		}
		n->namval = n->namenv = 0;
		n->namflg = N_DEFAULT;
#else
		if ((n->namflg & N_FUNCTN) && (uflg & UNSET_FUNC)) {
			func_unhash(name);
			freefunc(n);
			n->funcval = 0;
			n->namflg &= ~N_FUNCTN;
		} else {
			call_dolocale++;
			free(n->namval);
			free(n->namenv);
			n->namval = n->namenv = 0;
			n->namflg &= N_FUNCTN;
		}
#endif

		if (call_dolocale)
			dolocale((char *)name);

		if (flags & prompt)
		{
			if (n == &mailpnod)
				setmail(mailnod.namval);
			else if (n == &mailnod &&
				    (mailpnod.namflg & ~N_FUNCTN) == N_DEFAULT)
				setmail(0);
		}
	}
}

/*
 * The environment variables which affect locale.
 * Note: if all names in this list do not begin with 'L',
 * you MUST modify dolocale().  Also, be sure that the
 * fake_env has the same number of elements as localevar.
 */
static char *localevar[] = {
	"LC_ALL",
	"LC_CTYPE",
	"LC_MESSAGES",
	"LANG",
	0
};

static char *fake_env[] = {
	0,
	0,
	0,
	0,
	0
};

/*
 * If name is one of several special variables which affect the locale,
 * do a setlocale().
 */
static void
dolocale(nm)
	char *nm;
{
	char **real_env;
	struct namnod *n;
	int lv, fe;
	int i;

#ifdef INTERACTIVE
	/*
	 * Only set history size in libshedit if we are
	 * in interactive mode. This allows to avoid  to
	 * load libshedit when interpreting shell scripts.
	 */
	if ((flags & prompt) &&
	    (*nm == 'H') && eq(nm, "HISTORY")) {
			char	*hv = getcurenv(nm);

		shedit_chghistory(hv ? hv:"0");
	}
#endif

	/*
	 * Take advantage of fact that names of these vars all start
	 * with 'L' to avoid unnecessary work.
	 * Do locale processing only if /usr is mounted.
	 */
	if ((*nm != 'L') || !localedir_exists ||
	    (!(eq(nm, "LC_ALL") || eq(nm, "LC_CTYPE") ||
	    eq(nm, "LANG") || eq(nm, "LC_MESSAGES"))))
		return;

	/*
	 * setlocale() has all the smarts built into it, but
	 * it works by examining the environment.  Unfortunately,
	 * when you set an environment variable, the shell does
	 * not modify its own environment; it just remembers that the
	 * variable needs to be exported to any children.  We hack around
	 * this by consing up a fake environment for the use of setlocale()
	 * and substituting it for the real env before calling setlocale().
	 */

	/*
	 * Build the fake environment.
	 * Look up the value of each of the special environment
	 * variables, and put their value into the fake environment,
	 * if they are exported.
	 */
	for (lv = 0, fe = 0; localevar[lv]; lv++) {
		if ((n = findnam((unsigned char *)localevar[lv]))) {
			char *p, *q;

			if (!n->namval)
				continue;

			fake_env[fe++] = p = alloc(length((unsigned char *)
							    localevar[lv])
						+ length(n->namval) + 2);
			/* copy name */
			q = localevar[lv];
			while (*q)
				*p++ = *q++;

			*p++ = '=';

			/* copy value */
			q = (char *)(n->namval);
			while (*q)
				*p++ = *q++;
			*p++ = '\0';
		}
	}
	fake_env[fe] = (char *)0;

	/*
	 * Switch fake env for real and call setlocale().
	 */
	real_env = (char **)environ;
	environ = (char **)fake_env;

	if (setlocale(LC_ALL, "") == NULL)
		prs(_gettext(badlocale));

	/*
	 * Switch back and tear down the fake env.
	 */
	environ = (char **)real_env;
	for (i = 0; i < fe; i++) {
		free(fake_env[i]);
		fake_env[i] = (char *)0;
	}
}

#ifndef	HAVE_ISASTREAM
static int
isastream(fd)
	int	fd;
{
	int	ret = 0;

	if (isatty(fd))
		return (1);

#ifdef	I_CANPUT
	ret = ioctl(fd, I_CANPUT, 0);
	if (ret == -1 && errno == EBADF)
		return (-1);

	errno = 0;
	if (ret == 0 || ret == 1)
		ret = 1;
#endif
	return (ret);
}
#endif

#ifdef INTERACTIVE
/*
 * Functions needed by the history editor.
 */
char *
getcurenv(name)
	char	*name;
{
	struct namnod *n;

	n = findnam((unsigned char *)name);
	return (n ? (char *)n->namval:NULL);
}

void
ev_insert(name)
	char	*name;
{
	struct namnod	*n;
	char		*p = strchr(name, '=');

	if (p == NULL)
		return;

	*p = '\0';
	n = lookup((unsigned char *)name);
	*p = '=';
	if (n == 0)
		return;

	assign(n, (unsigned char *)&p[1]);

	n->namflg |= N_EXPORT;
}
#endif /* INTERACTIVE */
