/*#define	PLUS_DEBUG*/
/* @(#)find.c	1.74 08/04/17 Copyright 2004-2008 J. Schilling */
#ifndef lint
static	char sccsid[] =
	"@(#)find.c	1.74 08/04/17 Copyright 2004-2008 J. Schilling";
#endif
/*
 *	Another find implementation...
 *
 *	Copyright (c) 2004-2008 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifdef	__FIND__
#define	FIND_MAIN
#endif

#include <schily/mconfig.h>
#include <stdio.h>
#include <schily/unistd.h>
#include <schily/stdlib.h>
#ifdef	HAVE_FCHDIR
#include <schily/fcntl.h>
#else
#include <schily/maxpath.h>
#endif
#include <schily/stat.h>
#include <schily/time.h>
#include <schily/wait.h>
#include <schily/string.h>
#include <schily/utypes.h>	/* incl. limits.h (_POSIX_ARG_MAX/ARG_MAX) */
#ifdef	HAVE_SYS_PARAM_H
#include <sys/param.h>		/* #defines NCARGS on old systems */
#endif
#ifndef	DEV_BSIZE
#define	DEV_BSIZE	512
#endif
#include <schily/btorder.h>
#include <schily/getcwd.h>
#include <schily/patmatch.h>
#if	defined(HAVE_FNMATCH_H) & defined(HAVE_FNMATCH)
#include <fnmatch.h>
#endif
#include <schily/standard.h>
#include <schily/jmpdefs.h>
#include <schily/schily.h>
#include <pwd.h>
#include <grp.h>

#include <schily/nlsdefs.h>

#ifdef	__FIND__
char	strvers[] = "1.3";	/* The pure version string	*/
#endif

typedef struct {
	char	*left;
	char	*right;
	char	*this;
	int	op;
	union {
		int	i;
		long	l;
		dev_t	dev;
		ino_t	ino;
		mode_t	mode;
		nlink_t	nlink;
		uid_t	uid;
		gid_t	gid;
		size_t	size;
		time_t	time;
	} val, val2;
} findn_t;

#include <schily/walk.h>
#define	FIND_NODE
#include <schily/find.h>
#include "find_list.h"
#include "find_misc.h"

LOCAL	char	*tokennames[] = {
#define	OPEN	0
	"(",
#define	CLOSE	1
	")",
#define	LNOT	2
	"!",
#define	AND	3
	"a",
#define	LOR	4
	"o",
#define	ATIME	5
	"atime",
#define	CTIME	6
	"ctime",
#define	DEPTH	7
	"depth",
#define	EXEC	8
	"exec",
#define	FOLLOW	9	/* POSIX Extension */
	"follow",
#define	FSTYPE	10	/* POSIX Extension */
	"fstype",
#define	GROUP	11
	"group",
#define	INUM	12	/* POSIX Extension */
	"inum",
#define	LINKS	13
	"links",
#define	LOCL	14	/* POSIX Extension */
	"local",
#define	LS	15	/* POSIX Extension */
	"ls",
#define	MODE	16	/* POSIX Extension */
	"mode",
#define	MOUNT	17	/* POSIX Extension */
	"mount",
#define	MTIME	18
	"mtime",
#define	NAME	19
	"name",
#define	NEWER	20
	"newer",
#define	NOGRP	21
	"nogroup",
#define	NOUSER	22
	"nouser",
#define	OK_EXEC	23
	"ok",
#define	PERM	24
	"perm",
#define	PRINT	25
	"print",
#define	PRINTNNL 26	/* POSIX Extension */
	"printnnl",
#define	PRUNE	27
	"prune",
#define	SIZE	28
	"size",
#define	TIME	29	/* POSIX Extension */
	"time",
#define	TYPE	30
	"type",
#define	USER	31
	"user",
#define	XDEV	32
	"xdev",
#define	PATH	33	/* POSIX Extension */
	"path",
#define	LNAME	34	/* POSIX Extension */
	"lname",
#define	PAT	35	/* POSIX Extension */
	"pat",
#define	PPAT	36	/* POSIX Extension */
	"ppat",
#define	LPAT	37	/* POSIX Extension */
	"lpat",
#define	PACL	38	/* POSIX Extension */
	"acl",
#define	XATTR	39	/* POSIX Extension */
	"xattr",
#define	LINKEDTO 40	/* POSIX Extension */
	"linkedto",
#define	NEWERAA	41	/* POSIX Extension */
	"neweraa",
#define	NEWERAC	42	/* POSIX Extension */
	"newerac",
#define	NEWERAM	43	/* POSIX Extension */
	"neweram",
#define	NEWERCA	44	/* POSIX Extension */
	"newerca",
#define	NEWERCC	45	/* POSIX Extension */
	"newercc",
#define	NEWERCM	46	/* POSIX Extension */
	"newercm",
#define	NEWERMA	47	/* POSIX Extension */
	"newerma",
#define	NEWERMC	48	/* POSIX Extension */
	"newermc",
#define	NEWERMM	49	/* POSIX Extension */
	"newermm",
#define	SPARSE	50	/* POSIX Extension */
	"sparse",
#define	LTRUE	51	/* POSIX Extension */
	"true",
#define	LFALSE	52	/* POSIX Extension */
	"false",
#define	MAXDEPTH 53	/* POSIX Extension */
	"maxdepth",
#define	MINDEPTH 54	/* POSIX Extension */
	"mindepth",
#define	HELP	55	/* POSIX Extension */
	"help",
#define	CHOWN	56	/* POSIX Extension */
	"chown",
#define	CHGRP	57	/* POSIX Extension */
	"chgrp",
#define	CHMOD	58	/* POSIX Extension */
	"chmod",
#define	DOSTAT	59	/* POSIX Extension */
	"dostat",
#define	ENDPRIM	60
	0,
#define	EXECPLUS 61
	"exec",
	0
};
#define	NTOK	((sizeof (tokennames) / sizeof (tokennames[0])) - 1)

/*
 *	The struct plusargs and the adjacent space that holds the
 *	arg vector and the string table. The struct plusargs member "av"
 *	is also part of the ARG_MAX sized space that follows.
 *
 *	---------------------------------
 *	| Other struct plusargs fields	|	Don't count against ARG_MAX
 *	---------------------------------
 *	---------------------------------
 *	| 	New Arg vector[0]	|	Space for ARG_MAX starts here
 *	---------------------------------
 *	|		.		|
 *	|		.		|	Arg space grows upwards
 *	|		V		|
 *	---------------------------------
 *	|	 Arg vector end		|	"nextargp" points here
 *	---------------------------------
 *	---------------------------------
 *	| Space for first arg string	|
 *	---------------------------------	"laststr" points here
 *	|		^		|
 *	|		.		|	String space "grows" downwards
 *	|		.		|
 *	---------------------------------
 *	| Space for first arg string	|	Space for ARG_MAX ends here
 *	---------------------------------	"endp" points here
 */
struct plusargs {
	struct plusargs	*next;		/* Next in list for flushing	*/
	char		*endp;		/* Points to end of data block	*/
	char		**nextargp;	/* Points to next av[] entry	*/
	char		*laststr;	/* points to last used string	*/
	int		ac;		/* The argc for our command	*/
	char		*av[1];		/* The argv for our command	*/
};

#ifdef	PLUS_DEBUG			/* We are no longer reentrant	*/
LOCAL struct plusargs *plusp;		/* Avoid PLUS_DEBUG if possible	*/
#endif

#define	MINSECS		(60)
#define	HOURSECS	(60 * MINSECS)
#define	DAYSECS		(24 * HOURSECS)
#define	YEARSECS	(365 * DAYSECS)

extern	time_t	find_sixmonth;		/* 6 months before limit (ls)	*/
extern	time_t	find_now;		/* now limit (ls)		*/

LOCAL	findn_t	Printnode = { 0, 0, 0, PRINT };

#ifndef	__GNUC__
#define	inline
#endif

EXPORT	void	find_argsinit	__PR((finda_t *fap));
EXPORT	void	find_timeinit	__PR((time_t now));
EXPORT	findn_t	*find_printnode	__PR((void));
EXPORT	findn_t	*find_addprint	__PR((findn_t *np, finda_t *fap));
LOCAL	findn_t	*allocnode	__PR((finda_t *fap));
EXPORT	void	find_free	__PR((findn_t *t, finda_t *fap));
LOCAL	void	find_freenode	__PR((findn_t *t));
LOCAL	void	nexttoken	__PR((finda_t *fap));
LOCAL	BOOL	_nexttoken	__PR((finda_t *fap));
LOCAL	void	errjmp		__PR((finda_t *fap, int err));
EXPORT	int	find_token	__PR((char *word));
EXPORT	char	*find_tname	__PR((int op));
LOCAL	char	*nextarg	__PR((finda_t *fap, findn_t *t));
EXPORT	findn_t	*find_parse	__PR((finda_t *fap));
LOCAL	findn_t	*parse		__PR((finda_t *fap));
LOCAL	findn_t	*parseand	__PR((finda_t *fap));
LOCAL	findn_t	*parseprim	__PR((finda_t *fap));
EXPORT	void	find_firstprim	__PR((int *pac, char *const **pav));
EXPORT	BOOL	find_primary	__PR((findn_t *t, int op));
EXPORT	BOOL	find_pname	__PR((findn_t *t, char *word));
#ifdef	FIND_MAIN
LOCAL	int	walkfunc	__PR((char *nm, struct stat *fs, int type, struct WALK *state));
#endif
#ifdef	__FIND__
LOCAL	inline BOOL find_expr	__PR((char *f, char *ff, struct stat *fs, struct WALK *state, findn_t *t));
#else
EXPORT	BOOL	find_expr	__PR((char *f, char *ff, struct stat *fs, struct WALK *state, findn_t *t));
#endif
LOCAL	BOOL	doexec		__PR((char *f, int ac, char **av, struct WALK *state));
LOCAL	int	argsize		__PR((void));
LOCAL	BOOL	pluscreate	__PR((FILE *f, int ac, char **av, finda_t *fap));
LOCAL	BOOL	plusexec	__PR((char *f, findn_t *t, struct WALK *state));
EXPORT	int	find_plusflush	__PR((void *p, struct WALK *state));
EXPORT	void	find_usage	__PR((FILE *f));
#ifdef	FIND_MAIN
LOCAL	int	getflg		__PR((char *optstr, long *argp));
EXPORT	int	main		__PR((int ac, char **av));
#endif


EXPORT void
find_argsinit(fap)
	finda_t	*fap;
{
	fap->Argc = 0;
	fap->Argv = (char **)NULL;
	fap->std[0] = stdin;
	fap->std[1] = stdout;
	fap->std[2] = stderr;
	fap->primtype = 0;
	fap->found_action = FALSE;
	fap->patlen = 0;
	fap->walkflags = 0;
	fap->maxdepth = -1;
	fap->mindepth = -1;
	fap->plusp = (struct plusargs *)NULL;
	fap->jmp = NULL;
	fap->error = 0;
}

EXPORT void
find_timeinit(now)
	time_t	now;
{
	find_now	= now + 60;
	find_sixmonth	= now - 6L*30L*24L*60L*60L;
}

EXPORT findn_t *
find_printnode()
{
	return (&Printnode);
}

/*
 * Add a -print node to the parsed tree if there is no action already.
 */
EXPORT findn_t *
find_addprint(np, fap)
	findn_t	*np;
	finda_t	*fap;
{
	findn_t	*n;

	n = allocnode(fap);
	if (n == NULL) {
		find_freenode(np);
		return ((void *)NULL);
	}
	n->op = AND;
	n->left = (char *)np;
	n->right = (char *)&Printnode;
	return (n);
}

/*
 * allocnode is currently called by:
 *	find_addprint(), parse(), parseand(), parseprim()
 */
LOCAL findn_t *
allocnode(fap)
	finda_t	*fap;
{
	findn_t *n;

	n = __fjmalloc(fap->std[2], sizeof (findn_t), "allocnode", JM_RETURN);
	if (n == NULL)
		return (n);
	n->left = 0;
	n->right = 0;
	n->this = 0;
	n->op = 0;
	n->val.l = 0;
	n->val2.l = 0;
	return (n);
}

EXPORT void
find_free(t, fap)
	findn_t	*t;
	finda_t	*fap;
{
	if (fap != NULL) {
		struct plusargs *p;
		struct plusargs *np = NULL;

		for (p = fap->plusp; p != NULL; p = np) {
			np = p->next;
			free(p);
		}
	}

	find_freenode(t);
}

LOCAL void
find_freenode(t)
	findn_t	*t;
{
	if (t == (findn_t *)NULL || t == &Printnode)
		return;

	switch (t->op) {

	case OPEN:
	case LNOT:
		find_freenode((findn_t *)t->this);
		break;
	case AND:
	case LOR:
		find_freenode((findn_t *)t->left);
		find_freenode((findn_t *)t->right);
		break;
	case PAT:
	case PPAT:
	case LPAT:
		if (t->right != NULL)
			free(t->right);	/* aux array for patcompile() */
		break;
	default:
		;
	}
	free(t);
}

LOCAL void
nexttoken(fap)
	register finda_t	*fap;
{
	if (!_nexttoken(fap)) {
		errjmp(fap, EX_BAD);
		/* NOTREACHED */
	}
}

/*
 * No errjmp() variant of nexttoken(), returns FALSE on error.
 */
LOCAL BOOL
_nexttoken(fap)
	register finda_t	*fap;
{
	register char	*word;
	register char	*tail;

	if (fap->Argc <= 0) {
		fap->primtype = FIND_ENDARGS;
		return (TRUE);
	}
	word = *fap->Argv;
	if ((tail = strchr(word, '=')) != NULL) {
#ifdef	XXX
		if (*tail == '\0') {
			fap->Argv++; fap->Argc--;
		} else
#endif
			*fap->Argv = ++tail;
	} else {
		fap->Argv++; fap->Argc--;
	}
	if ((fap->primtype = find_token(word)) >= 0)
		return (TRUE);

	ferrmsgno(fap->std[2], EX_BAD, gettext("Bad Option: '%s'.\n"), word);
	find_usage(fap->std[2]);
	fap->primtype = FIND_ERRARG;	/* Mark as "parse aborted"	*/
	return (FALSE);
}

LOCAL void
errjmp(fap, err)
	register finda_t	*fap;
		int		err;
{
	fap->primtype	= FIND_ERRARG;	/* Mark as "parse aborted"	*/
	fap->error	= err;		/* Set error return		*/

	siglongjmp(((sigjmps_t *)fap->jmp)->jb, 1);
	/* NOTREACHED */
}

EXPORT int
find_token(word)
	register char	*word;
{
	char	**tp;
	char	*equalp;
	int	type;

	if ((equalp = strchr(word, '=')) != NULL)
		*equalp = '\0';

	if (*word == '-') {
		/*
		 * Do not allow -(, -), -!
		 */
		if (word[1] == '\0' || !strchr("()!", word[1]))
			word++;
	} else if (!strchr("()!", word[0]) && (!equalp || equalp[1] == '\0')) {
		goto bad;
	}
	for (type = 0, tp = tokennames; *tp; tp++, type++) {
		if (streql(*tp, word)) {
			if (equalp)
				*equalp = '=';
			return (type);
		}
	}
bad:
	if (equalp)
		*equalp = '=';

	return (-1);
}

EXPORT char *
find_tname(op)
	int	op;
{
	if (op >= 0 && op < NTOK)
		return (tokennames[op]);
	return ("unknown");
}

LOCAL char *
nextarg(fap, t)
	finda_t	*fap;
	findn_t	*t;
{
	if (fap->Argc-- <= 0) {
		char	*prim	= NULL;
		int	pt	= t->op;

		if (pt >= 0 && pt < NTOK)
			prim = tokennames[pt];
		if (prim) {
			ferrmsgno(fap->std[2], EX_BAD,
				gettext("Missing arg for '%s%s'.\n"),
				pt > LNOT ? "-":"", prim);
		} else {
			ferrmsgno(fap->std[2], EX_BAD,
				gettext("Missing arg.\n"));
		}
		errjmp(fap, EX_BAD);
		/* NOTREACHED */
		return ((char *)0);
	} else {
		return (*fap->Argv++);
	}
}

EXPORT findn_t *
find_parse(fap)
	finda_t	*fap;
{
	findn_t		*ret;

	if (!_nexttoken(fap))
		return ((findn_t *)NULL);	/* Immediate parse error */
	if (fap->primtype == FIND_ENDARGS)
		return ((findn_t *)NULL);	/* Empty command	 */

	ret = parse(fap);
	if (ret)
		return (ret);

	if (fap->primtype == HELP) {
		fap->primtype = FIND_ERRARG;
	} else if (fap->error == 0) {
		fap->primtype = FIND_ERRARG;
		fap->error = geterrno();
		if (fap->error == 0)
			fap->error = EX_BAD;
	}
	return (ret);
}

LOCAL findn_t *
parse(fap)
	finda_t	*fap;
{
	findn_t	*n;

	n = parseand(fap);
	if (n == NULL)
		return (n);
	if (fap->primtype == LOR) {
		findn_t	*l = allocnode(fap);

		if (l == NULL)
			goto err;
		l->left = (char *)n;
		l->op = fap->primtype;
		if (_nexttoken(fap))
			l->right = (char *)parse(fap);
		if (l->right == NULL) {
			find_freenode(l);
			n = NULL;		/* Do not free twice		*/
			goto err;
		}
		return (l);
	}
	return (n);
err:
	find_freenode(n);
	fap->primtype = FIND_ERRARG;		/* Mark as "parse aborted"	*/
	return ((findn_t *)NULL);
}

LOCAL findn_t *
parseand(fap)
	finda_t	*fap;
{
	findn_t	*n;

	n = parseprim(fap);
	if (n == NULL)
		return (n);

	if ((fap->primtype == AND) ||
	    (fap->primtype != LOR && fap->primtype != CLOSE &&
	    fap->primtype != FIND_ENDARGS)) {
		findn_t	*l = allocnode(fap);
		BOOL	ok = TRUE;

		if (l == NULL)
			goto err;
		l->left = (char *)n;
		l->op = AND;		/* If no Operator, default to AND -a */
		if (fap->primtype == AND) /* Fetch Operator for next node */
			ok = _nexttoken(fap);
		if (ok)
			l->right = (char *)parseand(fap);
		if (l->right == NULL) {
			find_freenode(l);
			n = NULL;		/* Do not free twice		*/
			goto err;
		}
		return (l);
	}
	return (n);
err:
	find_freenode(n);
	fap->primtype = FIND_ERRARG;		/* Mark as "parse aborted"	*/
	return ((findn_t *)NULL);
}

LOCAL findn_t *
parseprim(fap)
	finda_t	*fap;
{
	sigjmps_t	jmp;
	sigjmps_t	*ojmp = fap->jmp;
	register findn_t *n;
	register char	*p;
		Llong	ll;

	n = allocnode(fap);
	if (n == (findn_t *)NULL) {
		fap->primtype = FIND_ERRARG;	/* Mark as "parse aborted"	*/
		return ((findn_t *)NULL);
	}

	fap->jmp = &jmp;
	if (sigsetjmp(jmp.jb, 1) != 0) {
		/*
		 * We come here from siglongjmp()
		 */
		find_freenode(n);
		fap->jmp = ojmp;		/* Restore old jump target */
		return ((findn_t *)NULL);
	}
	switch (n->op = fap->primtype) {

	/*
	 * Use simple to old (historic) shell globbing.
	 */
	case NAME:
	case PATH:
	case LNAME:
#if	defined(HAVE_FNMATCH_H) & defined(HAVE_FNMATCH)
		n->this = nextarg(fap, n);
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
#endif
		/* FALLTHRU */
		/* Implement "fallback" to patmatch() if we have no fnmatch() */

	/*
	 * Use patmatch() which is a regular expression matcher that implements
	 * extensions that are compatible to old (historic) shell globbing.
	 */
	case PAT:
	case PPAT:
	case LPAT: {
		int	plen;

		plen = strlen(n->this = nextarg(fap, n));
		if (plen > fap->patlen)
			fap->patlen = plen;
		n->right = __fjmalloc(fap->std[2], sizeof (int)*plen,
						"space for pattern", fap->jmp);

		if ((n->val.i = patcompile((Uchar *)n->this, plen, (int *)n->right)) == 0) {
			ferrmsgno(fap->std[2],
				EX_BAD, gettext("Bad pattern in '-%s %s'.\n"),
						tokennames[n->op], n->this);
			errjmp(fap, EX_BAD);
			/* NOTREACHED */
		}
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	}

	case SIZE: {
		char	*numarg;

		fap->walkflags &= ~WALK_NOSTAT;

		p = n->left = nextarg(fap, n);
		numarg = p;
		if (p[0] == '-' || p[0] == '+')
			numarg = ++p;
		p = astoll(p, &ll);
		if (p[0] == '\0') {
			/* EMPTY */
			;
		} else if (p[0] == 'c' && p[1] == '\0') {
			n->this = p;
		} else if (getllnum(numarg, &ll) == 1) {
			n->this = p;
		} else if (*p) {
			ferrmsgno(fap->std[2], EX_BAD,
			gettext("Non numeric character '%c' in '-size %s'.\n"),
				*p, n->left);
			errjmp(fap, EX_BAD);
			/* NOTREACHED */
		}
		n->val.size = ll;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	}

	case LINKS:
		fap->walkflags &= ~WALK_NOSTAT;

		p = n->left = nextarg(fap, n);
		if (p[0] == '-' || p[0] == '+')
			p++;
		p = astoll(p, &ll);
		if (*p) {
			ferrmsgno(fap->std[2], EX_BAD,
			gettext("Non numeric character '%c' in '-links %s'.\n"),
				*p, n->left);
			errjmp(fap, EX_BAD);
			/* NOTREACHED */
		}
		n->val.nlink = ll;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case INUM:
		fap->walkflags &= ~WALK_NOSTAT;

		p = n->left = nextarg(fap, n);
		if (p[0] == '-' || p[0] == '+')
			p++;
		p = astoll(p, &ll);
		if (*p) {
			ferrmsgno(fap->std[2], EX_BAD,
			gettext("Non numeric character '%c' in '-inum %s'.\n"),
				*p, n->left);
			errjmp(fap, EX_BAD);
			/* NOTREACHED */
		}
		n->val.ino = ll;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case LINKEDTO: {
		struct stat ns;

		fap->walkflags &= ~WALK_NOSTAT;

		if (stat(n->left = nextarg(fap, n), &ns) < 0) {
			ferrmsg(fap->std[2],
				gettext("Cannot stat '%s'.\n"), n->left);
			errjmp(fap, geterrno());
			/* NOTREACHED */
		}
		n->val.ino = ns.st_ino;
		n->val2.dev = ns.st_dev;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	}

	case TIME:
	case ATIME:
	case CTIME:
	case MTIME: {
		int	len;

		fap->walkflags &= ~WALK_NOSTAT;

		p = n->left = nextarg(fap, n);
		if (p[0] == '-' || p[0] == '+')
			p++;
		if (gettnum(p, &n->val.time) != 1) {
			ferrmsgno(fap->std[2], EX_BAD,
				gettext("Bad timespec in '-%s %s'.\n"),
				tokennames[n->op], n->left);
			errjmp(fap, EX_BAD);
			/* NOTREACHED */
		}
		len = strlen(p);
		if (len > 0) {
			len = (Uchar)p[len-1];
			if (!(len >= '0' && len <= '9'))
				n->val2.i = 1;
		}
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	}

	case NEWERAA:
	case NEWERCA:
	case NEWERMA: {
		struct stat ns;

		fap->walkflags &= ~WALK_NOSTAT;

		if (stat(n->left = nextarg(fap, n), &ns) < 0) {
			ferrmsg(fap->std[2],
				gettext("Cannot stat '%s'.\n"), n->left);
			errjmp(fap, geterrno());
			/* NOTREACHED */
		}
		n->val.time = ns.st_atime;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	}

	case NEWERAC:
	case NEWERCC:
	case NEWERMC: {
		struct stat ns;

		fap->walkflags &= ~WALK_NOSTAT;

		if (stat(n->left = nextarg(fap, n), &ns) < 0) {
			ferrmsg(fap->std[2],
				gettext("Cannot stat '%s'.\n"), n->left);
			errjmp(fap, geterrno());
			/* NOTREACHED */
		}
		n->val.time = ns.st_ctime;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	}

	case NEWERAM:
	case NEWERCM:
	case NEWERMM:
	case NEWER: {
		struct stat ns;

		fap->walkflags &= ~WALK_NOSTAT;

		if (stat(n->left = nextarg(fap, n), &ns) < 0) {
			ferrmsg(fap->std[2],
				gettext("Cannot stat '%s'.\n"), n->left);
			errjmp(fap, geterrno());
			/* NOTREACHED */
		}
		n->val.time = ns.st_mtime;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	}

	case TYPE:
		fap->walkflags &= ~WALK_NOSTAT;

		n->this = (char *)nextarg(fap, n);
		switch (*(n->this)) {

		case 'b': case 'c': case 'd': case 'D':
		case 'e': case 'f': case 'l': case 'p':
		case 's':
			if ((n->this)[1] == '\0') {
				nexttoken(fap);
				fap->jmp = ojmp; /* Restore old jump target */
				return (n);
			}
		}
		ferrmsgno(fap->std[2], EX_BAD,
			gettext("Bad type '%c' in '-type %s'.\n"),
			*n->this, n->this);
		errjmp(fap, EX_BAD);
		/* NOTREACHED */
		break;

	case FSTYPE:
		fap->walkflags &= ~WALK_NOSTAT;

#ifdef	HAVE_ST_FSTYPE
		n->this = (char *)nextarg(fap, n);
#else
		ferrmsgno(fap->std[2], EX_BAD,
			gettext("-fstype not supported by this OS.\n"));
		errjmp(fap, EX_BAD);
		/* NOTREACHED */
#endif
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case LOCL:
		fap->walkflags &= ~WALK_NOSTAT;

#ifndef	HAVE_ST_FSTYPE
		ferrmsgno(fap->std[2], EX_BAD,
			gettext("-local not supported by this OS.\n"));
		errjmp(fap, EX_BAD);
		/* NOTREACHED */
#endif
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

#ifdef	CHOWN
	case CHOWN:
#endif
	case USER: {
		struct  passwd  *pw;
		char		*u;

		fap->walkflags &= ~WALK_NOSTAT;

		u = n->left = nextarg(fap, n);
		if (u[0] == '-' || u[0] == '+')
			u++;
		if ((pw = getpwnam(u)) != NULL) {
			n->val.uid = pw->pw_uid;
		} else {
			if (*astoll(n->left, &ll)) {
				ferrmsgno(fap->std[2], EX_BAD,
				gettext("User '%s' not in passwd database.\n"),
				n->left);
				errjmp(fap, EX_BAD);
				/* NOTREACHED */
			}
			n->val.uid = ll;
		}
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	}

#ifdef	CHGRP
	case CHGRP:
#endif
	case GROUP: {
		struct  group	*gr;
		char		*g;

		fap->walkflags &= ~WALK_NOSTAT;

		g = n->left = nextarg(fap, n);
		if (g[0] == '-' || g[0] == '+')
			g++;
		if ((gr = getgrnam(g)) != NULL) {
			n->val.gid = gr->gr_gid;
		} else {
			if (*astoll(n->left, &ll)) {
				ferrmsgno(fap->std[2], EX_BAD,
				gettext("Group '%s' not in group database.\n"),
				n->left);
				errjmp(fap, EX_BAD);
				/* NOTREACHED */
			}
			n->val.gid = ll;
		}
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	}

#ifdef	CHMOD
	case CHMOD:
#endif
	case PERM:
		fap->walkflags &= ~WALK_NOSTAT;

		n->left = nextarg(fap, n);
		if (getperm(fap->std[2], n->left, tokennames[n->op],
				&n->val.mode, (mode_t)0,
				n->op == PERM ? GP_FPERM|GP_XERR:GP_NOX) < 0) {
			errjmp(fap, EX_BAD);
			/* NOTREACHED */
		}
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case MODE:
		fap->walkflags &= ~WALK_NOSTAT;

		ferrmsgno(fap->std[2], EX_BAD,
				gettext("-mode not yet implemented.\n"));
		errjmp(fap, EX_BAD);
		/* NOTREACHED */
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case XDEV:
	case MOUNT:
		fap->walkflags &= ~WALK_NOSTAT;
		fap->walkflags |= WALK_MOUNT;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	case DEPTH:
		fap->walkflags |= WALK_DEPTH;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);
	case FOLLOW:
		fap->walkflags &= ~WALK_PHYS;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case MAXDEPTH:
	case MINDEPTH:
		p = n->left = nextarg(fap, n);
		p = astoll(p, &ll);
		if (*p) {
			ferrmsgno(fap->std[2], EX_BAD,
			gettext("Non numeric character '%c' in '-%s %s'.\n"),
				*p, tokennames[n->op], n->left);
			errjmp(fap, EX_BAD);
			/* NOTREACHED */
		}
		n->val.l = ll;
		if (n->op == MAXDEPTH)
			fap->maxdepth = ll;
		else
			fap->mindepth = ll;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case NOUSER:
	case NOGRP:
	case PACL:
	case XATTR:
	case SPARSE:
	case DOSTAT:
		fap->walkflags &= ~WALK_NOSTAT;
		/* FALLTHRU */
	case PRUNE:
	case LTRUE:
	case LFALSE:
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case OK_EXEC:
	case EXEC: {
		int	i = 1;

		n->this = (char *)fap->Argv;	/* Cheat: Pointer is pointer */
		nextarg(fap, n);		/* Eat up cmd name	    */
		while ((p = nextarg(fap, n)) != NULL) {
			if (streql(p, ";"))
				break;
			else if (streql(p, "+") && streql(fap->Argv[-2], "{}")) {
				n->op = fap->primtype = EXECPLUS;
				if (!pluscreate(fap->std[2], --i, (char **)n->this, fap)) {
					errjmp(fap, EX_BAD);
					/* NOTREACHED */
				}
				n->this = (char *)fap->plusp;
				break;
			}
			i++;
		}
		n->val.i = i;
#ifdef	PLUS_DEBUG
		if (0) {
			char **pp = (char **)n->this;
			for (i = 0; i < n->val.i; i++, pp++)
				printf("ARG %d '%s'\n", i, *pp);
		}
#endif
	}
	/* FALLTHRU */

	case LS:
		fap->walkflags &= ~WALK_NOSTAT;
	case PRINT:
	case PRINTNNL:
		fap->found_action = TRUE;
		nexttoken(fap);
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case FIND_ENDARGS:
#ifdef	DEBUG
		ferrmsgno(fap->std[2], EX_BAD,
				gettext("ENDARGS in parseprim()\n"));
#endif
		ferrmsgno(fap->std[2], EX_BAD,
				gettext("Incomplete expression.\n"));
		find_freenode(n);
		fap->jmp = ojmp;		/* Restore old jump target */
		return ((findn_t *)NULL);

	case OPEN:
		nexttoken(fap);
		n->this = (char *)parse(fap);
		if (fap->primtype != CLOSE) {
			ferrmsgno(fap->std[2], EX_BAD,
				gettext("Found '%s', but ')' expected.\n"),
				fap->Argv[-1]);
			errjmp(fap, EX_BAD);
			/* NOTREACHED */
		} else {
			nexttoken(fap);
			fap->jmp = ojmp;	/* Restore old jump target */
			return (n);
		}
		break;

	case CLOSE:
		ferrmsgno(fap->std[2], EX_BAD, gettext("Missing '('.\n"));
		errjmp(fap, EX_BAD);
		/* NOTREACHED */

	case LNOT:
		nexttoken(fap);
		n->this = (char *)parseprim(fap);
		if (n->this == NULL) {
			find_freenode(n);
			return ((findn_t *)NULL);
		}
		fap->jmp = ojmp;		/* Restore old jump target */
		return (n);

	case AND:
	case LOR:
		ferrmsgno(fap->std[2], EX_BAD,
		gettext("Invalid expression with -%s.\n"), tokennames[n->op]);
		errjmp(fap, EX_BAD);
		/* NOTREACHED */

	case HELP:
		find_usage(fap->std[2]);
		find_freenode(n);
		return ((findn_t *)NULL);

	default:
		ferrmsgno(fap->std[2], EX_BAD,
				gettext("Internal malfunction.\n"));
		errjmp(fap, EX_BAD);
		/* NOTREACHED */
	}
	fap->jmp = ojmp;			/* Restore old jump target */
	fap->primtype = FIND_ERRARG;		/* Mark as "parse aborted" */
	return (0);
}

#define	S_ALLPERM	(S_IRWXU|S_IRWXG|S_IRWXO)
#define	S_ALLFLAGS	(S_ISUID|S_ISGID|S_ISVTX)
#define	S_ALLMODES	(S_ALLFLAGS | S_ALLPERM)

EXPORT void
find_firstprim(pac, pav)
	int	*pac;
	char    *const *pav[];
{
	register int	cac  = *pac;
	register char *const *cav = *pav;
	register char	c;

	while (cac > 0 &&
		(c = **cav) != '-' && c != '(' && c != ')' && c != '!') {
		cav++;
		cac--;
	}
	*pac = cac;
	*pav = cav;
}

EXPORT BOOL
find_primary(t, op)
	findn_t	*t;
	int	op;
{
	BOOL	ret = FALSE;

	if (t->op == op) {
		return (TRUE);
	}
	switch (t->op) {

	case OPEN:
		ret = find_primary((findn_t *)t->this, op);
		break;
	case LNOT:
		ret = find_primary((findn_t *)t->this, op);
		break;
	case AND:
		ret = find_primary((findn_t *)t->left, op);
		if (ret)
			return (ret);
		ret = find_primary((findn_t *)t->right, op);
		break;
	case LOR:
		ret = find_primary((findn_t *)t->left, op);
		if (ret)
			return (ret);
		ret = find_primary((findn_t *)t->right, op);
		break;

	default:
		;
	}
	return (ret);
}

EXPORT BOOL
find_pname(t, word)
	findn_t	*t;
	char	*word;
{
	if (streql(word, "-exec+"))
		return (find_primary(t, EXECPLUS));
	return (find_primary(t, find_token(word)));
}

#ifdef	FIND_MAIN
LOCAL int
walkfunc(nm, fs, type, state)
	char		*nm;
	struct stat	*fs;
	int		type;
	struct WALK	*state;
{
	if (type == WALK_NS) {
		ferrmsg(state->std[2], gettext("Cannot stat '%s'.\n"), nm);
		state->err = 1;
		return (0);
	} else if (type == WALK_SLN && (state->walkflags & WALK_PHYS) == 0) {
		ferrmsg(state->std[2],
				gettext("Cannot follow symlink '%s'.\n"), nm);
		state->err = 1;
		return (0);
	} else if (type == WALK_DNR) {
		if (state->flags & WALK_WF_NOCHDIR) {
			ferrmsg(state->std[2],
				gettext("Cannot chdir to '%s'.\n"), nm);
		} else {
			ferrmsg(state->std[2],
				gettext("Cannot read '%s'.\n"), nm);
		}
		state->err = 1;
		return (0);
	}

	if (state->maxdepth >= 0 && state->level >= state->maxdepth)
		state->flags |= WALK_WF_PRUNE;
	if (state->mindepth >= 0 && state->level < state->mindepth)
		return (0);

	find_expr(nm, nm + state->base, fs, state, state->tree);
	return (0);
}
#endif

#ifdef	__FIND__
LOCAL inline BOOL
#else
EXPORT BOOL
#endif
find_expr(f, ff, fs, state, t)
	char		*f;	/* path name */
	char		*ff;	/* file name */
	struct stat	*fs;
	struct WALK	*state;
	findn_t		*t;
{
	time_t	xtime;
	char	*p;
	char	lname[8192];

	switch (t->op) {

	case LNAME: {
		int	lsize;

		if (!S_ISLNK(fs->st_mode))
			return (FALSE);

		if (state->lname != NULL) {
			p = state->lname;
			goto nmatch;
		}
		lname[0] = '\0';
		lsize = readlink(state->level ? ff : f, lname, sizeof (lname));
		if (lsize < 0) {
			ferrmsg(state->std[2],
				gettext("Cannot read link '%s'.\n"), ff);
			return (FALSE);
		}
		lname[sizeof (lname)-1] = '\0';
		if (lsize >= 0)
			lname[lsize] = '\0';
		p = lname;
		goto nmatch;
	}
	case PATH:
		p = f;
		goto nmatch;
	case NAME:
		p = ff;
	nmatch:
#if	defined(HAVE_FNMATCH_H) & defined(HAVE_FNMATCH)
		return (!fnmatch(t->this, p, 0));
#else
		goto pattern;		/* Use patmatch() as "fallback" */
#endif

	case LPAT: {
		int	lsize;

		if (!S_ISLNK(fs->st_mode))
			return (FALSE);

		if (state->lname != NULL) {
			p = state->lname;
			goto pattern;
		}
		lname[0] = '\0';
		lsize = readlink(state->level ? ff : f, lname, sizeof (lname));
		if (lsize < 0) {
			ferrmsg(state->std[2],
				gettext("Cannot read link '%s'.\n"), ff);
			return (FALSE);
		}
		lname[sizeof (lname)-1] = '\0';
		if (lsize >= 0)
			lname[lsize] = '\0';
		p = lname;
		goto pattern;
	}
	case PPAT:
		p = f;
		goto pattern;
	case PAT:
		p = ff;
	pattern: {
		Uchar	*pr;		/* patmatch() return */

		pr = patmatch((Uchar *)t->this, (int *)t->right,
			(Uchar *)p, 0, strlen(p), t->val.i, state->patstate);
		return (*p && pr && (*pr == '\0'));
	}

	case SIZE:
		switch (*(t->left)) {
		case '+':
			if (t->this)
				return (fs->st_size    > t->val.size);
			return ((fs->st_size+511)/512  > t->val.size);
		case '-':
			if (t->this)
				return (fs->st_size   <  t->val.size);
			return ((fs->st_size+511)/512 <  t->val.size);
		default:
			if (t->this)
				return (fs->st_size   == t->val.size);
			return ((fs->st_size+511)/512 == t->val.size);
		}

	case LINKS:
		switch (*(t->left)) {
		case '+':
			return (fs->st_nlink  > t->val.nlink);
		case '-':
			return (fs->st_nlink <  t->val.nlink);
		default:
			return (fs->st_nlink == t->val.nlink);
		}

	case INUM:
		switch (*(t->left)) {
		case '+':
			return (fs->st_ino  > t->val.ino);
		case '-':
			return (fs->st_ino <  t->val.ino);
		default:
			return (fs->st_ino == t->val.ino);
		}

	case LINKEDTO:
			return ((fs->st_ino == t->val.ino) &&
				(fs->st_dev == t->val2.dev));

	case ATIME:
		xtime = fs->st_atime;
		goto times;
	case CTIME:
		xtime = fs->st_ctime;
		goto times;
	case MTIME:
	case TIME:
		xtime = fs->st_mtime;
	times:
		if (t->val2.i != 0)
			goto timex;

		switch (*(t->left)) {
		case '+':
			return ((find_now-xtime)/DAYSECS >  t->val.time);
		case '-':
			return ((find_now-xtime)/DAYSECS <  t->val.time);
		default:
			return ((find_now-xtime)/DAYSECS == t->val.time);
		}
	timex:
		switch (*(t->left)) {
		case '+':
			return ((find_now-xtime) >  t->val.time);
		case '-':
			return ((find_now-xtime) <  t->val.time);
		default:
			return ((find_now-xtime) == t->val.time);
		}

	case NEWERAA:
	case NEWERAC:
	case NEWERAM:
		return (t->val.time < fs->st_atime);

	case NEWERCA:
	case NEWERCC:
	case NEWERCM:
		return (t->val.time < fs->st_ctime);

	case NEWER:
	case NEWERMA:
	case NEWERMC:
	case NEWERMM:
		return (t->val.time < fs->st_mtime);

	case TYPE:
		switch (*(t->this)) {
		case 'b':
			return (S_ISBLK(fs->st_mode));
		case 'c':
			return (S_ISCHR(fs->st_mode));
		case 'd':
			return (S_ISDIR(fs->st_mode));
		case 'D':
			return (S_ISDOOR(fs->st_mode));
		case 'e':
			return (S_ISEVC(fs->st_mode));
		case 'f':
			return (S_ISREG(fs->st_mode));
		case 'l':
			return (S_ISLNK(fs->st_mode));
		case 'p':
			return (S_ISFIFO(fs->st_mode));
		case 'P':
			return (S_ISPORT(fs->st_mode));
		case 's':
			return (S_ISSOCK(fs->st_mode));
		default:
			return (FALSE);
		}

	case FSTYPE:
#ifdef	HAVE_ST_FSTYPE
		return (streql(t->this, fs->st_fstype));
#else
		return (TRUE);
#endif

	case LOCL:
#ifdef	HAVE_ST_FSTYPE
		if (streql("nfs", fs->st_fstype) ||
		    streql("autofs", fs->st_fstype) ||
		    streql("cachefs", fs->st_fstype))
			return (FALSE);
#endif
		return (TRUE);

#ifdef	CHOWN
	case CHOWN:
		fs->st_uid = t->val.uid;
		return (TRUE);
#endif

	case USER:
		switch (*(t->left)) {
		case '+':
			return (fs->st_uid  > t->val.uid);
		case '-':
			return (fs->st_uid <  t->val.uid);
		default:
			return (fs->st_uid == t->val.uid);
		}

#ifdef	CHGRP
	case CHGRP:
		fs->st_gid = t->val.gid;
		return (TRUE);
#endif

	case GROUP:
		switch (*(t->left)) {
		case '+':
			return (fs->st_gid  > t->val.gid);
		case '-':
			return (fs->st_gid <  t->val.gid);
		default:
			return (fs->st_gid == t->val.gid);
		}

#ifdef	CHMOD
	case CHMOD:
		getperm(state->std[2], t->left, tokennames[t->op],
			&t->val.mode, fs->st_mode & S_ALLMODES,
			(S_ISDIR(fs->st_mode) ||
			(fs->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) != 0) ?
								GP_DOX:GP_NOX);
		fs->st_mode &= ~S_ALLMODES;
		fs->st_mode |= t->val.mode;
		return (TRUE);
#endif

	case PERM:
		if (t->left[0] == '-')
			return ((fs->st_mode & t->val.mode) == t->val.mode);
		else
			return ((fs->st_mode & S_ALLMODES) == t->val.mode);

	case MODE:
		return (TRUE);

	case XDEV:
	case MOUNT:
	case DEPTH:
	case FOLLOW:
	case DOSTAT:
		return (TRUE);

	case NOUSER:
		return (getpwuid(fs->st_uid) == NULL);

	case NOGRP:
		return (getgrgid(fs->st_gid) == NULL);

	case PRUNE:
		state->flags |= WALK_WF_PRUNE;
		return (TRUE);

	case MAXDEPTH:
	case MINDEPTH:
		return (TRUE);

	case PACL:
		if (state->pflags & PF_ACL) {
			return ((state->pflags & PF_HAS_ACL) != 0);
		}
		return (has_acl(state->std[2], f, ff, fs));

	case XATTR:
		if (state->pflags & PF_XATTR) {
			return ((state->pflags & PF_HAS_XATTR) != 0);
		}
		return (has_xattr(state->std[2], ff));

	case SPARSE:
		if (!S_ISREG(fs->st_mode))
			return (FALSE);
#ifdef	HAVE_ST_BLOCKS
		return (fs->st_size > (fs->st_blocks * DEV_BSIZE + DEV_BSIZE));
#else
		return (FALSE);
#endif

	case OK_EXEC: {
		char qbuf[32];

		fflush(state->std[1]);
		fprintf(state->std[2], "< %s ... %s > ? ", ((char **)t->this)[0], f);
		fflush(state->std[2]);
		fgetline(state->std[0], qbuf, sizeof (qbuf) - 1);

		switch (qbuf[0]) {
		case 'y':
			if (qbuf[1] == '\0' || streql(qbuf, "yes")) break;
		default:
			return (FALSE);
		}
	}
	/* FALLTHRU */

	case EXEC:
		return (doexec(f, t->val.i, (char **)t->this, state));

	case EXECPLUS:
		return (plusexec(f, t, state));

	case PRINT:
		filewrite(state->std[1], f, strlen(f));
		putc('\n', state->std[1]);
		return (TRUE);

	case PRINTNNL:
		filewrite(state->std[1], f, strlen(f));
		putc(' ', state->std[1]);
		return (TRUE);

	case LS:
		/*
		 * The third parameter is the file name used for readlink()
		 * (inside find_list()) relatively to the current working
		 * directory. For file names from the command line, we did not
		 * perform a chdir() before, so we need to use the full path
		 * name.
		 */
		find_list(state->std, fs, f, state->level ? ff : f, state);
		return (TRUE);

	case LTRUE:
		return (TRUE);

	case LFALSE:
		return (FALSE);

	case OPEN:
		return (find_expr(f, ff, fs, state, (findn_t *)t->this));
	case LNOT:
		return (!find_expr(f, ff, fs, state, (findn_t *)t->this));
	case AND:
		return (find_expr(f, ff, fs, state, (findn_t *)t->left) ?
			find_expr(f, ff, fs, state, (findn_t *)t->right) : 0);
	case LOR:
		return (find_expr(f, ff, fs, state, (findn_t *)t->left) ?
			1 : find_expr(f, ff, fs, state, (findn_t *)t->right));
	}
	return (FALSE);		/* Unknown operator ??? */
}

LOCAL BOOL
doexec(f, ac, av, state)
	char	*f;
	int	ac;
	char	**av;
	struct WALK *state;
{
	pid_t	pid;
	int	retval;

	if ((pid = fork()) < 0) {
		ferrmsg(state->std[2], gettext("Cannot fork child.\n"));
		return (FALSE);
	}
	if (pid) {
		while (wait(&retval) != pid)
			/* LINTED */
			;
		return (retval == 0);
	} else {
		register int	i;
		register char	**pp = av;

		/*
		 * This is the forked process and for this reason, we may
		 * call fcomerr() here without problems.
		 */
		if (walkhome(state) < 0) {
			fcomerr(state->std[2],
					gettext("Cannot chdir to '.'.\n"));
		}
#ifndef	F_SETFD
		walkclose(state);
#endif

#define	iscurlypair(p)	((p)[0] == '{' && (p)[1] == '}' && (p)[2] == '\0')

		if (f) {
			for (i = 0; i < ac; i++, pp++) {
				register char	*p = *pp;

				if (iscurlypair(p))	/* streql(p, "{}") */
					*pp = f;
			}
		}
#ifdef	PLUS_DEBUG
		error("argsize %d\n",
			(plusp->endp - (char *)&plusp->nextargp[0]) -
			(plusp->laststr - (char *)&plusp->nextargp[1]));
#endif
		av[ac] = NULL;	/* -exec {} \; is not NULL terminated */

		fexecve(av[0], state->std[0], state->std[1], state->std[2],
							av, state->env);
#ifdef	PLUS_DEBUG
		error("argsize %d\n",
			(plusp->endp - (char *)&plusp->nextargp[0]) -
			(plusp->laststr - (char *)&plusp->nextargp[1]));
#endif
		/*
		 * This is the forked process and for this reason, we may
		 * call fcomerr() here without problems.
		 */
		fcomerr(state->std[2],
			gettext("Cannot execute '%s'.\n"), av[0]);
		/* NOTREACHED */
		return (-1);
	}
}

#ifndef	LINE_MAX
#define	LINE_MAX	1024
#endif

#if	defined(IS_MACOS_X) && defined(HAVE_CRT_EXTERNS_H)
/*
 * The MAC OS X linker does not grok "common" varaibles.
 * We need to fetch the address of "environ" using a hack.
 */
#include <crt_externs.h>
#define	environ	*_NSGetEnviron()
#else
extern	char **environ;
#endif

/*
 * Return ARG_MAX - LINE_MAX - size of current environment.
 *
 * The return value is reduced by LINE_MAX to allow the called
 * program to do own exec(2) calls with slightly increased arg size.
 */
LOCAL int
argsize()
{
	static int	ret = 0;

	if (ret == 0) {
		register int	evs = 0;
		register char	**ep;

		for (ep = environ; *ep; ep++) {
			evs += strlen(*ep) + 1 + sizeof (ep);
		}
		evs += sizeof (char **); /* The environ NULL ptr at the end */

#ifdef	_SC_ARG_MAX
		ret = sysconf(_SC_ARG_MAX);
		if (ret < 0)
			ret = _POSIX_ARG_MAX;
#else
#ifdef	ARG_MAX
		ret = ARG_MAX;
#else
#ifdef	NCARGS
		ret = NCARGS;
#endif
#endif
#endif

#ifdef	PLUS_DEBUG
		ret = 3000;
#define		LINE_MAX	100
		error("evs %d\n", evs);
#endif
		if (ret <= 0)
			ret = 10000;	/* Just a guess */

		ret -= evs;
		if ((ret - LINE_MAX) > 0)
			ret -= LINE_MAX;
		else
			ret -= 100;
	}
	return (ret);
}

LOCAL BOOL
pluscreate(f, ac, av, fap)
	FILE	*f;
	int	ac;
	char	**av;
	finda_t	*fap;
{
	struct plusargs	*pp;
	register char	**ap = av;
	register int	i;

#ifdef	PLUS_DEBUG
	printf("Argc %d\n", ac);
	ap = av;
	for (i = 0; i < ac; i++, ap++)
		printf("ARG %d '%s'\n", i, *ap);
#endif

	pp = __fjmalloc(fap->std[2], argsize() + sizeof (struct plusargs),
						"-exec args", fap->jmp);
	pp->laststr = pp->endp = (char *)(&pp->av[0]) + argsize();
	pp->ac = 0;
	pp->nextargp = &pp->av[0];

#ifdef	PLUS_DEBUG
	printf("pp          %d\n", pp);
	printf("pp->laststr %d\n", pp->laststr);
	printf("argsize()   %d\n", argsize());
#endif

	/*
	 * Copy args from command line.
	 */
	ap = av;
	for (i = 0; i < ac; i++, ap++) {
#ifdef	PLUS_DEBUG
		printf("ARG %d '%s'\n", i, *ap);
#endif
		*(pp->nextargp++) = *ap;
		pp->laststr -= strlen(*ap) + 1;
		pp->ac++;
		if (pp->laststr <= (char *)pp->nextargp) {
			ferrmsgno(f, EX_BAD,
				gettext("No space to copy -exec args.\n"));
			free(pp);		/* The exec plusargs struct */
			return (FALSE);
		}
	}
#ifdef	PLUS_DEBUG
	error("lastr %d endp %d diff %d\n",
		pp->laststr, pp->endp, pp->endp - pp->laststr);
#endif
	pp->endp = pp->laststr;	/* Reduce endp by the size of cmdline args */

#ifdef	PLUS_DEBUG
	ap = &pp->av[0];
	for (i = 0; i < pp->ac; i++, ap++) {
		printf("ARG %d '%s'\n", i, *ap);
	}
#endif
#ifdef	PLUS_DEBUG
	printf("pp          %d\n", pp);
	printf("pp->laststr %d\n", pp->laststr);
#endif
	pp->next = fap->plusp;
	fap->plusp = pp;
#ifdef	PLUS_DEBUG
	plusp = fap->plusp;	/* Makes libfind not MT safe */
#endif
	return (TRUE);
}

LOCAL BOOL
plusexec(f, t, state)
	char	*f;
	findn_t	*t;
	struct WALK *state;
{
	register struct plusargs *pp = (struct plusargs *)t->this;
#ifdef	PLUS_DEBUG
	register char	**ap;
	register int	i;
#endif
	size_t	size;
	size_t	slen = strlen(f) + 1;
	char	*cp;
	int	ret = TRUE;

	size = pp->laststr - (char *)&pp->nextargp[2];

	if (pp->laststr < (char *)&pp->nextargp[2] ||
	    slen > size) {
		pp->nextargp[0] = NULL;
		ret = doexec(NULL, pp->ac, pp->av, state);
		pp->laststr = pp->endp;
		pp->ac = t->val.i;
		pp->nextargp = &pp->av[t->val.i];
		size = pp->laststr - (char *)&pp->nextargp[2];
	}
	if (pp->laststr < (char *)&pp->nextargp[2] ||
	    slen > size) {
		ferrmsgno(state->std[2], EX_BAD,
			gettext("No space for arg '%s'.\n"), f);
		return (FALSE);
	}
	cp = pp->laststr - slen;
	strcpy(cp, f);
	pp->nextargp[0] = cp;
	pp->ac++;
	pp->nextargp++;
	pp->laststr -= slen;

#ifdef	PLUS_DEBUG
	ap = &plusp->av[0];
	for (i = 0; i < plusp->ac; i++, ap++) {
		printf("ARG %d '%s'\n", i, *ap);
	}
	error("EXECPLUS '%s'\n", f);
#endif
	return (ret);
}

EXPORT int
find_plusflush(p, state)
	void	*p;
	struct WALK *state;
{
	struct plusargs	*plusp = p;
	BOOL		ret = TRUE;

	/*
	 * Execute all unflushed '-exec .... {} +' expressions.
	 */
	while (plusp) {
#ifdef	PLUS_DEBUG
		error("lastr %p endp %p\n", plusp->laststr, plusp->endp);
#endif
		if (plusp->laststr != plusp->endp) {
			plusp->nextargp[0] = NULL;
			if (!doexec(NULL, plusp->ac, plusp->av, state))
				ret = FALSE;
		}
		plusp = plusp->next;
	}
	return (ret);
}

EXPORT void
find_usage(f)
	FILE	*f;
{
	fprintf(f, gettext("Usage:	%s [options] [path_1 ... path_n] [expression]\n"), get_progname());
	fprintf(f, gettext("Options:\n"));
	fprintf(f, gettext("	-H	follow symbolic links encountered on command line\n"));
	fprintf(f, gettext("	-L	follow all symbolic links\n"));
	fprintf(f, gettext("*	-P	do not follow symbolic links (default)\n"));
	fprintf(f, gettext("*	-help	Print this help.\n"));
	fprintf(f, gettext("*	-version Print version number.\n"));
	fprintf(f, gettext("Operators in decreasing precedence:\n"));
	fprintf(f, gettext("	( )	group an expression\n"));
	fprintf(f, gettext("	!, -a, -o negate a primary (unary NOT), logical AND, logical OR\n"));
	fprintf(f, gettext("Primaries:\n"));
	fprintf(f, gettext("*	-acl	      TRUE if the file has additional ACLs defined\n"));
	fprintf(f, gettext("	-atime #      TRUE if st_atime is in specified range\n"));
#ifdef	CHGRP
	fprintf(f, gettext("*	-chgrp gname/gid always TRUE, sets st_gid to gname/gid\n"));
#endif
#ifdef	CHMOD
	fprintf(f, gettext("*	-chmod mode/onum always TRUE, sets permissions to mode/onum\n"));
#endif
#ifdef	CHOWN
	fprintf(f, gettext("*	-chown uname/uid always TRUE, sets st_uid to uname/uid\n"));
#endif
	fprintf(f, gettext("	-ctime #      TRUE if st_ctime is in specified range\n"));
	fprintf(f, gettext("	-depth	      evaluate directory content before directory (always TRUE)\n"));
	fprintf(f, gettext("*	-dostat	      Do not do stat optimization (always TRUE)\n"));
	fprintf(f, gettext("	-exec program [argument ...] \\;\n"));
	fprintf(f, gettext("	-exec program [argument ...] {} +\n"));
	fprintf(f, gettext("*	-false	      always FALSE\n"));
	fprintf(f, gettext("*	-follow	      outdated: follow all symbolic links (always TRUE)\n"));
	fprintf(f, gettext("*	-fstype type  TRUE if st_fstype matches type\n"));
	fprintf(f, gettext("	-group gname/gid TRUE if st_gid matches gname/gid\n"));
	fprintf(f, gettext("*	-inum #	      TRUE if st_ino is in specified range\n"));
	fprintf(f, gettext("*	-linkedto path TRUE if the file is linked to path\n"));
	fprintf(f, gettext("	-links #      TRUE if st_nlink is in specified range\n"));
	fprintf(f, gettext("*	-lname glob   TRUE if symlink name matches shell glob\n"));
	fprintf(f, gettext("*	-local	      TRUE if st_fstype does not match remote fs types\n"));
	fprintf(f, gettext("*	-lpat pattern TRUE if symlink name matches pattern\n"));
	fprintf(f, gettext("*	-ls	      list files similar to 'ls -ilds' (always TRUE)\n"));
	fprintf(f, gettext("*	-maxdepth #   descend at most # directory levels (always TRUE)\n"));
	fprintf(f, gettext("*	-mindepth #   start tests at directory level # (always TRUE)\n"));
	fprintf(f, gettext("	-mtime #      TRUE if st_mtime is in specified range\n"));
	fprintf(f, gettext("	-name glob    TRUE if path component matches shell glob\n"));
	fprintf(f, gettext("	-newer file   TRUE if st_mtime newer then mtime of file\n"));
	fprintf(f, gettext("	-newerXY file TRUE if [acm]time (X) newer then [acm]time (Y) of file\n"));
	fprintf(f, gettext("	-nogroup      TRUE if not in group database\n"));
	fprintf(f, gettext("	-nouser       TRUE if not in user database\n"));
	fprintf(f, gettext("	-ok program [argument ...] \\;\n"));
	fprintf(f, gettext("*	-pat pattern  TRUE if path component matches pattern\n"));
	fprintf(f, gettext("*	-path glob    TRUE if full path matches shell glob\n"));
	fprintf(f, gettext("	-perm mode/onum TRUE if symbolic/octal permission matches\n"));
	fprintf(f, gettext("*	-ppat pattern TRUE if full path matches pattern\n"));
	fprintf(f, gettext("	-print	      print file names line separated to stdout (always TRUE)\n"));
	fprintf(f, gettext("*	-printnnl     print file names space separated to stdout (always TRUE)\n"));
	fprintf(f, gettext("	-prune	      do not descent current directory (always TRUE)\n"));
	fprintf(f, gettext("	-size #	      TRUE if st_size is in specified range\n"));
	fprintf(f, gettext("*	-sparse	      TRUE if file appears to be sparse\n"));
	fprintf(f, gettext("*	-true	      always TRUE\n"));
	fprintf(f, gettext("	-type c	      TRUE if file type matches, c is from (b c d D e f l p P s)\n"));
	fprintf(f, gettext("	-user uname/uid TRUE if st_uid matches uname/uid\n"));
	fprintf(f, gettext("*	-xattr	      TRUE if the file has extended attributes\n"));
	fprintf(f, gettext("	-xdev, -mount restrict search to current filesystem (always TRUE)\n"));
	fprintf(f, gettext("Primaries marked with '*' are POSIX extensions, avoid them in portable scripts.\n"));
	fprintf(f, gettext("If path is omitted, '.' is used. If expression is omitted, -print is used.\n"));
}

#ifdef FIND_MAIN

/* ARGSUSED */
LOCAL int
getflg(optstr, argp)
	char	*optstr;
	long	*argp;
{
/*	error("optstr: '%s'\n", optstr);*/

	if (optstr[1] != '\0')
		return (-1);

	switch (*optstr) {

	case 'H':
		*(int *)argp |= WALK_ARGFOLLOW;
		return (TRUE);
	case 'L':
		*(int *)argp |= WALK_ALLFOLLOW;
		return (TRUE);
	case 'P':
		*(int *)argp &= ~(WALK_ARGFOLLOW | WALK_ALLFOLLOW);
		return (TRUE);

	default:
		return (-1);
	}
}

EXPORT int
main(ac, av)
	int	ac;
	char	**av;
{
	int	cac  = ac;
	char *	*cav = av;
	char *	*firstpath;
	char *	*firstprim;
	BOOL	help = FALSE;
	BOOL	prversion = FALSE;
	finda_t	fa;
	findn_t	*Tree;
	struct WALK	walkstate;

	save_args(ac, av);

#ifdef	USE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain("SCHILY_FIND", "/opt/schily/lib/locale");
	textdomain("SCHILY_FIND");
#endif
	find_argsinit(&fa);
	fa.walkflags = WALK_CHDIR | WALK_PHYS;
	fa.walkflags |= WALK_NOSTAT;
	fa.walkflags |= WALK_NOEXIT;

	/*
	 * Do not check the return code for getargs() as we may get an error
	 * code from e.g. "find -print" and we do not like to handle this here.
	 */
	cac--, cav++;
	getargs(&cac, (char * const **)&cav, "help,version,&",
			&help, &prversion,
			getflg, (long *)&fa.walkflags);
	if (help) {
		find_usage(stderr);
		return (0);
	}
	if (prversion) {
		printf("sfind release %s (%s-%s-%s) Copyright (C) 2004-2008 Jörg Schilling\n",
				strvers,
				HOST_CPU, HOST_VENDOR, HOST_OS);
		return (0);
	}

	firstpath = cav;	/* Remember first file type arg */
	find_firstprim(&cac, (char *const **)&cav);
	firstprim = cav;	/* Remember first Primary type arg */
	fa.Argv = cav;
	fa.Argc = cac;

	if (cac) {
		Tree = find_parse(&fa);
		if (fa.primtype == FIND_ERRARG) {
			find_free(Tree, &fa);
			return (fa.error);
		}
		if (fa.primtype != FIND_ENDARGS) {
			ferrmsgno(stderr, EX_BAD,
				gettext("Incomplete expression.\n"));
			find_free(Tree, &fa);
			return (EX_BAD);
		}
		if (find_pname(Tree, "-chown") || find_pname(Tree, "-chgrp") ||
		    find_pname(Tree, "-chmod")) {
			ferrmsgno(stderr, EX_BAD,
				gettext("Unsupported primary -chown/-chgrp/-chmod.\n"));
			find_free(Tree, &fa);
			return (EX_BAD);
		}
	} else {
		Tree = 0;
	}
	if (Tree == 0) {
		Tree = find_printnode();
	} else if (!fa.found_action) {
		Tree = find_addprint(Tree, &fa);
		if (Tree == (findn_t *)NULL)
			return (geterrno());
	}
	walkinitstate(&walkstate);
	if (fa.patlen > 0) {
		walkstate.patstate = __jmalloc(sizeof (int) * fa.patlen,
					"space for pattern state", JM_RETURN);
		if (walkstate.patstate == NULL)
			return (geterrno());
	}

	find_timeinit(time(0));

	walkstate.walkflags	= fa.walkflags;
	walkstate.maxdepth	= fa.maxdepth;
	walkstate.mindepth	= fa.mindepth;
	walkstate.lname		= NULL;
	walkstate.tree		= Tree;
	walkstate.err		= 0;
	walkstate.pflags	= 0;

	if (firstpath == firstprim) {
		treewalk(".", walkfunc, &walkstate);
	} else {
		for (cav = firstpath; cav != firstprim; cav++) {
			treewalk(*cav, walkfunc, &walkstate);
			/*
			 * XXX hier break wenn treewalk() Fehler gemeldet
			 */
		}
	}
	/*
	 * Execute all unflushed '-exec .... {} +' expressions.
	 */
	find_plusflush(fa.plusp, &walkstate);
	find_free(Tree, &fa);
	if (walkstate.patstate != NULL)
		free(walkstate.patstate);
	return (walkstate.err);
}

#endif /* FIND_MAIN */
