/* @(#)node.c	1.23 08/12/20 Copyright 1985-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)node.c	1.23 08/12/20 Copyright 1985-2008 J. Schilling";
#endif
/*
 *	Node handling routines
 *
 *	Copyright (c) 1985-2008 J. Schilling
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

#include <schily/mconfig.h>
#include <stdio.h>
#include "bsh.h"
#include "str.h"
#include "node.h"
#include "strsubs.h"
#include <schily/stdlib.h>

EXPORT	Tnode*	allocnode	__PR((long type, Tnode * lp, Tnode * rp));
EXPORT	void	freetree	__PR((Tnode * np));
EXPORT	Argvec*	allocvec	__PR((int len));
EXPORT	void	freevec		__PR((Argvec * vp));
EXPORT	void	printtree	__PR((FILE * f, Tnode * cmd));
EXPORT	void	printio		__PR((FILE * f, Tnode * cmd));
EXPORT	void	_printio	__PR((FILE * f, long type));
EXPORT	void	printstring	__PR((FILE * f, Tnode * cmd));

EXPORT Tnode *
allocnode(type, lp, rp)
	long	type;
	Tnode	*lp;
	Tnode	*rp;
{
	register Tnode *np;

	if ((np = (Tnode *)malloc(sizeof (Tnode))) != NULL) {
		np->tn_type = type;
		np->tn_left.tn_node = lp;
		np->tn_right.tn_node = rp;
	} else {
		berror(sn_no_mem);
#ifdef	needed
		if (0) {	/* XXX ist das ueberhaupt ein Tnode* ??*/
			if (xntype(type) == STRING || xntype(type) == LSTRING)
				free(lp);
			else
				freetree(lp); /* XXX auch dasz ist falsch */
				/*
				 * XXX lp ist leider teilweise
				 * nicht allocziert! sonst waere freetree oder
				 * free korrekt. Free, dann wenn es ein
				 * String ist, sonst freetre...oder??
				 * Es scheint generell besser zu sein lp
				 * unangetastet zu lassen.
				 */
		}
#endif
		freetree(rp); 	/* XXX ist das ueberhaupt ein Tnode* ??*/
				/*
				 * XXX rp sollte eigentlich immer
				 * ein Tnode sein.
				 */
	}
	return (np);
}

EXPORT void
freetree(np)
	register Tnode *np;
{
	if (np) {
		switch ((int)ntype(np)) {

		case '&':
		case ';':
		case '|':
		case ERRPIPE:	/* |% */
		case '(':
		case ANDP:	/* && */
		case ORP:	/* || */
		case CMD:
			freetree(np->tn_left.tn_node);
			break;
		case STRING:
		case LSTRING:
		case '<':
		case DOCIN:	/* << */
		case '>':
		case OUTAPP:	/* >> */
		case '%':
		case ERRAPP:	/* %% */
			free(np->tn_left.tn_str);
		case IDUP:	/* <& */
		case ODUP:	/* >& */
			break;
		case VECTOR:
			freevec(np->tn_left.tn_avec);
			break;
		default:
			berror("!freetree(type: 0x%04lX)'%s'",
						ntype(np), np->tn_left.tn_str);
		}
		freetree(np->tn_right.tn_node);
		free((char *) np);
	}
}

EXPORT Argvec *
allocvec(len)
	int	len;
{
	register Argvec	*vp;

	/*
	 * The struct Argvec contains av_av[1] which is used as the last NULL
	 * pointer for size computations.
	 */
	vp = (Argvec *)malloc(sizeof (Argvec)+ len*sizeof (vp->av_av[0]));
	if (vp == (Argvec *)NULL)
		return ((Argvec *)NULL);

	vp->av_ac = len;
	vp->av_av[len] = NULL;
	{
		register int	i;
		register char	**p;

		for (i = len, p = vp->av_av; --i >= 0; )
			*p++ = NULL;
	}
	return (vp);
}

EXPORT void
freevec(vp)
	register Argvec	*vp;
{
	register char	**t;

	for (t = vp->av_av; *t != NULL; t++)
		free(*t);
	free((char *) vp);
}

#ifdef	INTERACTIVE

EXPORT void
printtree(f, cmd)
	register FILE	*f;
	register Tnode	*cmd;
{
	while (cmd != (Tnode *) NULL) {
		switch ((int)ntype(cmd)) {

		case '&':
		case '|':
		case ';':
			printtree(f, cmd->tn_left.tn_node);
			fprintf(f, "%c ", (int)ntype(cmd));
			break;
		case ERRPIPE:	/* |% */
			printtree(f, cmd->tn_left.tn_node);
			fprintf(f, "|%% ");
			break;
		case ANDP:	/* && */
			printtree(f, cmd->tn_left.tn_node);
			fprintf(f, "&& ");
			break;
		case ORP:	/* || */
			printtree(f, cmd->tn_left.tn_node);
			fprintf(f, "|| ");
			break;
		case '(':
			putc('(', f);
			printtree(f, cmd->tn_left.tn_node);
			putc(')', f);
			break;
		case CMD:
			printtree(f, cmd->tn_left.tn_node);
			break;
		case '<':
		case DOCIN:	/* << */
		case '>':
		case OUTAPP:	/* >> */
		case '%':
		case ERRAPP:	/* %% */
		case IDUP:	/* <& */
		case ODUP:	/* >& */
			printio(f, cmd);
			fprintf(f, " ");
/*			printstring(f, cmd);*/
			break;
		case STRING:
		case LSTRING:
/*			fprintf(f, "%s ", cmd->tn_left.tn_str);*/
			printstring(f, cmd);
			break;
		}
		cmd = cmd->tn_right.tn_node;
	}
}
#endif	/* INTERACTIVE */


EXPORT void
printio(f, cmd)
	FILE	*f;
	Tnode	*cmd;
{
	_printio(f, cmd->tn_type);
	if (ntype(cmd) != IDUP && ntype(cmd) != ODUP)
		fprintf(f, "%s", cmd->tn_left.tn_str);
}

EXPORT void
_printio(f, type)
	FILE	*f;
	long	type;
{
	int	fd = (type >> 24) & 0xFF;
	int	fd2 = (type >> 16) & 0xFF;
	int	t = xntype(type);

	if (t == IDUP || t == ODUP)
		fprintf(f, "%d", fd);
	if ((type >> 8) != 0)
		fprintf(f, "%c", (int)(type >> 8));
	fprintf(f, "%c", (int)(type & 0xFF));
	if (t == IDUP || t == ODUP)
		fprintf(f, "%d", fd2);
}


EXPORT void
printstring(f, cmd)
		FILE	*f;
	register Tnode	*cmd;
{
/*error("qtype %lX '%s\n", quotetype(cmd), cmd->tn_left.tn_str);*/
	switch ((int)quotetype(cmd)) {

	case NOQUOTE:
		fprintf(f, "%s ", quote_string(cmd->tn_left.tn_str, special));
		break;
	case SQUOTE:
		fprintf(f, "'%s' ", cmd->tn_left.tn_str);
		break;
	case DQUOTE:
		fprintf(f, "\"%s\" ", cmd->tn_left.tn_str);
		break;
	case BQUOTE:
		fprintf(f, "`%s` ", cmd->tn_left.tn_str);
		break;
	}
}
