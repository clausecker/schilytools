/* @(#)cond.c	1.26 17/07/15 Copyright 1985-2017 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)cond.c	1.26 17/07/15 Copyright 1985-2017 J. Schilling";
#endif
/*
 *	Bsh conditional code handling
 *
 *		if .. then .. else .. fi
 *		for .. in .. end
 *		loop .. end
 *		switch .. case .. end
 *		read
 *
 *	Copyright (c) 1985-2017 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/stdio.h>
#include <schily/varargs.h>
#include "bsh.h"
#include "node.h"
#include "str.h"
#include "strsubs.h"
#include <schily/string.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/utypes.h>
#define	REDEFINE_CTYPE		/* Allow to use our local ctype.h */
#include "ctype.h"
#include <schily/patmatch.h>


#define	EXEC		1
#define	SKIP		2
#define	ABORTED		-2

#define	STOPFI		1
#define	STOPEND		2
#define	QUANT		10

typedef struct m_stack {
	char		*s_lp;
	int		s_level;
	struct m_stack	*s_next;
} _MSTK, *MSTK;

#ifdef	DEBUG
extern	int	ttyflg;
#endif
extern	int	delim;

EXPORT	void	push		__PR((int id));
EXPORT	void	freestack	__PR((void));
LOCAL	int	pop		__PR((void));
LOCAL	void	dec_level	__PR((void));
EXPORT	char	*cons_args	__PR((Argvec * vp, int n));
LOCAL	char	*growline	__PR((char *s1, char *s2));
LOCAL	char	*gnextline	__PR((void));
LOCAL	int	readloop	__PR((Argvec * vp, int stopc));
LOCAL	void	freeup		__PR((void));
LOCAL	int	parseuntil	__PR((int task, int ilev, FILE ** std, char **vpp));
LOCAL	BOOL	makevec		__PR((char **vec, va_list args));
LOCAL	int	skipuntil	__PR((int ilev, ...));
LOCAL	int	execuntil	__PR((FILE ** std, int ilev, ...));
LOCAL	void	not_found	__PR((FILE * fp, char *name));
EXPORT	void	bif		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bfor		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bloop		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bread		__PR((Argvec * vp, FILE ** std, int flag));
EXPORT	void	bswitch		__PR((Argvec * vp, FILE ** std, int flag));

int		level = 0,			/* Achtung !!!!		   */
		idsp = 0,			/* alle statischen Variablen */
		idlen = 0,			/* aus cond.c mueszen in   */
		read_delim = 0,			/* call_cmd (in call.c)	   */
		*idstack = (int *) NULL;	/* gerettet werden.	   */
MSTK		firstp = (MSTK) NULL,		/* Wichtig, falls neue	   */
		stackp = (MSTK) NULL,		/* Variablen dazu kommen.  */
		foundline = (MSTK) NULL;

#ifdef	OOO
EXPORT void
push(id)
	int	id;
{
	int	*np;
	size_t	newidlen;

	if (idlen == idsp) {	/* realloc !!! */
		newidlen = idlen + QUANT;
		np = (int *)malloc(newidlen * sizeof (int));
		if (idstack) {
			movebytes((char *) idstack, (char *) np, idlen * sizeof (int));
			free((char *) idstack);
		}
		idlen = newidlen;
		idstack = np;
	}
	idstack[idsp++] = id;
}
#else
EXPORT void
push(id)
	int	id;
{
	if (idlen == idsp) {
		idlen += QUANT;
		if (idstack)
			idstack = (int *)realloc(idstack, idlen * sizeof (int));
		else
			idstack = (int *)malloc(idlen * sizeof (int));
	}
	idstack[idsp++] = id;
}
#endif

EXPORT void
freestack()
{
	free((char *) idstack);
	idstack = (int *) 0;
	idlen = 0;
	idsp = 0;
}

LOCAL int
pop()
{
	int	id;

	id = idstack[--idsp];
	if (idsp == 0)
		freestack();
	return (id);
}

LOCAL void
dec_level()
{
	level--;
	if (!level)
		freeup();
}

EXPORT char *
cons_args(vp, n)
	Argvec	*vp;
	int	n;
{
		char	*cmdln;
	register char	*end;
	register char	**av = vp->av_av;
	register int	ac = vp->av_ac;
	register int	i;
	register size_t	len = 0;

	if (n >= ac)
		return (makestr(nullstr));
	for (i = n; i < ac; i++)
		len += strlen(av[i]);
	len += (ac - n);
	cmdln = end = malloc(len);
	if (!cmdln)
		return (NULL);
#ifdef DEBUG
	printf("        cons_args: len = %d\n", len);
#endif
	for (i = n; i < ac - 1; i++)
		end = strcatl(end, av[i], " ", (char *)NULL);
	strcatl(end, av[i], (char *)NULL);
#ifdef DEBUG
	printf("        cons_args = '%s'\n", cmdln);
#endif
	return (cmdln);
}

LOCAL char *
growline(s1, s2)
	char	*s1;
	char	*s2;
{
	char	*s;

	s = concat(s1, nl, s2, (char *)NULL);
	free(s1);
	free(s2);
	return (s);
}

LOCAL char *
gnextline()
{
	register	char	*lp = NULL;

	do {
		if (lp)
			lp = growline(lp, nextline());
		else
			lp = nextline();
	} while (lp[strlen(lp)-1] == '\\');
	return (lp);
}

LOCAL int
readloop(vp, stopc)
	Argvec	*vp;
	int	stopc;
{
	register int	ilev = 0;
	register MSTK	sp;
	register MSTK	lp = (MSTK) NULL;
		char	*linep = NULL;
	register Uchar	*p;
	register char	*cp;

#ifdef DEBUG
	printf("        readloop: ttyflg = %s\n", ttyflg?"TRUE":"FALSE");
#endif
	if (firstp)
		return (TRUE);
	push(stopc);
	for (;;) {
		if (stackp) {
			quote();
			linep = gnextline();
			unquote();
#ifdef	DEBUG
			printf("        readloop: linep = '%s'\n", linep);
#endif
			if (delim == EOF) {
				berror("EOF unexpected.");
				ex_status = 1;
				return (FALSE);
			}
		} else {
			linep = cons_args(vp, 0);
		}
		for (p = (Uchar *)linep; iswhite(*p); p++);
		if (wordeql((char *)p, "if")) {
			push(STOPFI);
			ilev++;
		} else if (wordeql((char *)p, "for") || wordeql((char *)p, "loop") ||
			    wordeql((char *)p, "switch")) {
			push(STOPEND);
			ilev++;
		}
		sp = (MSTK)malloc(sizeof (_MSTK));
		if (lp)
			lp->s_next = sp;
		else
			firstp = stackp = sp;
		cp = makestr((char *)p);
		sp->s_lp = cp;
		sp->s_level = ilev;
		sp->s_next = (MSTK) NULL;
#ifdef DEBUG
		printf("        stacked line: '%s', internal level %d, at %x\n", cp, ilev, cp);
#endif
		if (linep)
			free(linep);
		if (wordeql(cp, "fi") || wordeql(cp, "end")) {
			stopc = pop();
			if (wordeql(cp, "fi")) {
				if (stopc == STOPEND) {
					freestack();
					freeup();
					berror("'end' expected.");
					ex_status = 1;
					return (FALSE);
				}
			} else {
				if (stopc == STOPFI) {
					freestack();
					freeup();
					berror("'fi' expected.");
					ex_status = 1;
					return (FALSE);
				}
			}
			ilev--;
		}
		if (ilev == 0)
			return (TRUE);
		lp = sp;
	}
}

LOCAL void
freeup()
{
	register MSTK	np;
	register MSTK	cp = firstp;

#ifdef DEBUG
	printf("        freeup:\n");
#endif
	while (cp) {
		np = cp->s_next;
#ifdef DEBUG
		printf("        freeup: '%s' at %x, ",	cp->s_lp, cp->s_lp);
#endif
		free(cp->s_lp);
#ifdef DEBUG
		printf("struct at %x\n", cp);
#endif
		free((char *) cp);
		cp = np;
	}
	stackp = firstp = (MSTK) NULL;
}

LOCAL int
parseuntil(task, ilev, std, vpp)
	int		task;
	register int	ilev;
	FILE		*std[];
	register char	**vpp;
{
	register int	i;
	register MSTK	sp = stackp;

#ifdef DEBUG
	for (i = 0; vpp[i]; i++)
		printf("'%s' ", vpp[i]);
	printf("at level %d\n", ilev);
#endif
	if (!sp)
		return (-1);
	for (;;) {
#ifdef DEBUG
		if (ilev != sp->s_level)
			printf("        %s line '%s' (level=%d).\n",
				(task == SKIP) ? "skipping" : "executing",
				sp->s_lp, sp->s_level);
#endif
		if (ilev == sp->s_level) {
			for (i = 0; vpp[i]; i++) {
#ifdef DEBUG
				printf("        comparing line '%s' at level %d with '%s'\n", sp->s_lp, ilev, vpp[i]);
#endif
				if (streqln(vpp[i], sp->s_lp, strlen(vpp[i])))
					break;
			}
			if (vpp[i]) {
#ifdef DEBUG
				printf("        found at #%d.\n", i);
#endif
				foundline = sp;
				stackp = sp->s_next;
				return (i);
			}
		}
		if (task == EXEC) {
			stackp = sp;
			if (wordeql(sp->s_lp, "break") || ctlc || read_delim == EOF)
				return (ABORTED);
			pushline(sp->s_lp);
			freetree(cmdline(0, std, FALSE));
			if (stackp == sp)
				sp = sp->s_next;
			else
				sp = stackp;
		} else if (task == SKIP) {
			sp = sp->s_next;
		}
	}
}

#define	VEC_SIZE	4
LOCAL BOOL
makevec(vec, args)
	char	**vec;
	va_list	args;
{
	char	*p;
	int	n = 0;

	do {
		if (n == VEC_SIZE) {
			berror("Implementation error (VEC_SIZE too small).");
			return (FALSE);
		}
		p = va_arg(args, char *);
		vec[n++] = p;

	} while (p != NULL);
	return (TRUE);
}

/* VARARGS1 */

#ifdef	PROTOTYPES
LOCAL int
skipuntil(int ilev, ...)
#else
LOCAL int
skipuntil(ilev, va_alist)
	int	ilev;
	va_dcl
#endif
{
	va_list	args;
	char	*vec[VEC_SIZE];
	int	ret;

#ifdef DEBUG
	printf("        skipuntil: ");
#endif
#ifdef	PROTOTYPES
	va_start(args, ilev);
#else
	va_start(args);
#endif
	if (!makevec(vec, args)) {
		va_end(args);
		return (ABORTED);
	}
	va_end(args);
	ret = parseuntil(SKIP, ilev, gstd, vec);
	return (ret);
}

/* VARARGS2 */

#ifdef	PROTOTYPES
LOCAL int
execuntil(FILE *std[], int ilev, ...)
#else
LOCAL int
execuntil(std, ilev, va_alist)
	FILE	*std[];
	int	ilev;
	va_dcl
#endif
{
	va_list	args;
	char	*vec[VEC_SIZE];
	int	ret;

#ifdef DEBUG
	printf("        execuntil: ");
#endif
#ifdef	PROTOTYPES
	va_start(args, ilev);
#else
	va_start(args);
#endif
	if (!makevec(vec, args))
		return (ABORTED);
	va_end(args);
	ret = parseuntil(EXEC, ilev, std, vec);
	return (ret);
}

LOCAL void
not_found(fp, name)
	FILE	*fp;
	char	*name;
{
	fprintf(fp, "'%s' not found.\n", name);
	ex_status = 1;
}

/* if - command */
/* ARGSUSED */
EXPORT void
bif(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
		char	*cmdl;
	register int	ilev;

#ifdef	DEBUG
	int	i;
#endif

	level++;
#ifdef DEBUG
	printf("        bif: ");
	for (i = 0; i < vp->av_ac; i++)
		printf("%s ", vp->av_av[i]);
	printf(", files %d %d %d,\n", std[0], std[1], std[2]);
#endif
	if (vp->av_ac < 2) {
		wrong_args(vp, std);
		dec_level();
		return;
	}
	if (!readloop(vp, STOPFI))
		return;
	ilev = stackp->s_level;
	if (streql(vp->av_av[1], "(")) {
		if (!test(vp, std)) {
			dec_level();
			return;
		}
	} else {
		cmdl = cons_args(vp, 1);
#ifdef DEBUG
		printf("        bif: cmdl '%s'.\n", cmdl);
#endif
		pushline(cmdl);
		freetree(cmdline(0, std, FALSE));
		free(cmdl);
	}
#ifdef DEBUG
	printf("        bif: retval = %d\n", ex_status);
#endif
	if (ex_status) {
		if (skipuntil(ilev, "else", "fi", (char *)NULL) == 0) {
			if (execuntil(std, ilev, "fi", (char *)NULL))
				not_found(std[2], "fi");
		}
	} else {
		if (skipuntil(ilev, "then", "fi", (char *)NULL) != 0) {
			not_found(std[2], "then");
		} else {
			if (execuntil(std, ilev, "else", "fi", (char *)NULL) == 0) {
				if (skipuntil(ilev, "fi", (char *)NULL))
					not_found(std[2], "fi");
			}
		}
	}
	dec_level();
}

/* ARGSUSED */
EXPORT void
bfor(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	register int	i;
	register int	brktype;
		int	ilev;
		char	*name;
		MSTK	sp;

	level++;
	if (vp->av_ac < 3) {
		wrong_args(vp, std);
		dec_level();
		return;
	}
	name = vp->av_av[1];
	if (!streql(vp->av_av[2], "in")) {
		not_found(std[2], "in");
		ex_status = 1;
		dec_level();
		return;
	}
#ifdef DEBUG
	printf("        bfor: for %s in ", name);
	for (i = 3; i < vp->av_ac; i++)
		printf("%s ", vp->av_av[i]);
	printf(nl);
#endif
	if (!readloop(vp, STOPEND))
		return;
	ilev = stackp->s_level;
	sp = stackp = stackp->s_next;
	for (i = 3, brktype = 0; (i < vp->av_ac) && (brktype != ABORTED); i++) {
		ev_insert(concat(name, eql, vp->av_av[i], (char *)NULL));
#ifdef DEBUG
		printf("        executing loop with '%s'\n", vp->av_av[i]);
#endif
		brktype = execuntil(std, ilev, "end", (char *)NULL);
		stackp = sp;
	}
	if (skipuntil(ilev, "end", (char *)NULL))
		not_found(std[2], "end");
	dec_level();
}

/* ARGSUSED */
EXPORT void
bloop(vp, std, flag)
		Argvec	*vp;
	register FILE	*std[];
		int	flag;
{
	register int	ilev;
	register int	brktype;
	register MSTK	sp;

	level++;
	if (!readloop(vp, STOPEND))
		return;
	ilev = stackp->s_level;
	sp = stackp = stackp->s_next;
	do {
		brktype = execuntil(std, ilev, "end", (char *)NULL);
		stackp = sp;
	} while (brktype != ABORTED);
	if (skipuntil(ilev, "end", (char *)NULL))
		not_found(std[2], "end");
	dec_level();
}

/* ARGSUSED */
EXPORT void
bread(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
	extern int	prflg;
	extern int	ttyflg;
	FILE	*old;
	int	save = delim;
	char	*linep = NULL;
	int	prsave = prflg;
	int	ttysave = ttyflg;

	ttyflg = isatty(fdown(std[0]));
	prflg = ttyflg || iflg;

	old = setinput(std[0]);
	linep = nextline();
#ifdef	DEBUG
	printf("linep: %s\n", linep);
#endif
	if (old == std[0])
		read_delim = delim;
	prflg = prsave;
	ttyflg = ttysave;
	setinput(old);
	delim = save;
	ev_insert(concat(vp->av_av[1], eql, linep, (char *)NULL));
	if (linep)
		free(linep);
}

/* ARGSUSED */
EXPORT void
bswitch(vp, std, flag)
	Argvec	*vp;
	FILE	*std[];
	int	flag;
{
		int	ilev;
	register Uchar	*pattern;
	register char	*p;
		char	*name, found;
		int	plen, *aux, *state, alt;

	level++;
	if (!streql(vp->av_av[2], "of")) {
		not_found(std[2], "of");
		dec_level();
		return;
	}
	if (!readloop(vp, STOPEND))
		return;
	ilev = stackp->s_level;
	stackp = stackp->s_next;
	name = vp->av_av[1];
	found = FALSE;
	for (;;) {
		if (skipuntil(ilev, "case", "end", (char *)NULL) == 1)
			break;
		pattern = (Uchar *)foundline->s_lp + strlen("case");
		while (iswhite(*pattern))
			pattern++;
		plen = strlen((char *)pattern);
#ifdef DEBUG
		printf("matching with '%s'\n", pattern);
#endif
		aux = (int *)malloc((size_t) plen * sizeof (int));
		if ((alt = patcompile((unsigned char *)pattern, plen, aux)) == 0) {
			fprintf(std[2], "'%s': %s\n", pattern, ebadpattern);
			ex_status = 1;
			free((char *) aux);
			break;
		}
		state = (int *)malloc((size_t) (plen+1) * sizeof (int));
		p = (char *)patmatch((unsigned char *)pattern, aux,
				(unsigned char *)name, 0, strlen(name), alt, state);
		if (p && *p == '\0') {
#ifdef DEBUG
			printf("found.\n");
#endif
			found = TRUE;
			free((char *) aux);
			free((char *) state);
			break;
		}
		free((char *) aux);
		free((char *) state);
	}
	if (found) {
		if (execuntil(std, ilev, "end", (char *)NULL) == ABORTED)
			skipuntil(ilev, "end", (char *)NULL);
	} else if (ex_status == 1) {
		skipuntil(ilev, "end", (char *)NULL);
	}
	dec_level();
}
