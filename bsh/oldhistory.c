/* @(#)oldhistory.c	1.16 08/12/20 Copyright 1985-2008 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char sccsid[] =
	"@(#)oldhistory.c	1.16 08/12/20 Copyright 1985-2008 J. Schilling";
#endif
/*
 *	old bsh history section
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
#include "node.h"
#include "str.h"
#include "strsubs.h"
#include <schily/stdlib.h>
#include <schily/string.h>

#ifndef	INTERACTIVE
extern Tnode *lastcmd;
extern Tnode **cur_base;
extern int history;

EXPORT	int	high_hist	__PR((void));
EXPORT	BOOL	treeequal	__PR((Tnode *cmd, Tnode *ocmd));
EXPORT	void	h_append	__PR((Tnode *cmd));
EXPORT	void	lr_used		__PR((int nr));
LOCAL	char	*treestring	__PR((Tnode *cmd, char *string));
LOCAL	char	*sprintstring	__PR((char *string, Tnode *cmd));
EXPORT	void	printtree	__PR((FILE *f, Tnode *cmd));
LOCAL	Tnode	*stringtree	__PR((char *string));
LOCAL	char	*nextwd		__PR((char *line, char *word));
LOCAL	int	strmatch	__PR((char *pline, char *string));
LOCAL	void	get_str		__PR((char *));
LOCAL	char 	*getsub		__PR((int del, char *line, char *ret));
EXPORT	void	phistory	__PR((BOOL));
LOCAL	BOOL	ehist		__PR((Tnode **));
EXPORT	void	hi_list		__PR((FILE *));
EXPORT	BOOL	sethistory	__PR((char *));
EXPORT	void	inithistory	__PR((void));
LOCAL	void	clearhistory	__PR((void));

EXPORT int
high_hist()
{
	int i, ret;

	ret = -1;
	for (i = history-1; i >= 0 && ret == -1; i--)
		if (cur_base[i] != (Tnode *) NULL)
			ret = i;
	return (ret);
}

EXPORT BOOL
treeequal(cmd, ocmd)
	Tnode	*cmd;
	Tnode	*ocmd;
{
	char str1[512], str2[512];

	treestring(cmd, str1);
	treestring(ocmd, str2);
	return (streql(str1, str2));
}

EXPORT void
h_append(cmd)
	Tnode	*cmd;
{
	int i, max;

	max = high_hist();
	if (max == history-1) {
		freetree(cur_base[0]);
		for (i = 0; i < history-1; i++)
			cur_base[i] = cur_base[i+1];
	} else {
		max++;
	}
	cur_base[max] = cmd;
}

EXPORT void
lr_used(nr)
	int	nr;
{
	int i, max;
	Tnode *cbase;

	cbase = cur_base[nr];
	max = high_hist();
	for (i = nr; i < max; i++)
		cur_base[i] = cur_base[i+1];
	cur_base[max] = cbase;
}

LOCAL char *
treestring(cmd, string)
	Tnode	*cmd;
	char	*string;
{
	while (cmd != (Tnode *) NULL) {
		switch ((int)ntype(cmd)) {

		case '&':
		case '|':
		case ';':
			string = treestring(cmd->tn_left.tn_node, string);
			sprintf(string, "%c ", (int)ntype(cmd));
			string += 2;
			break;
		case ERRPIPE:	/* |% */
			string = treestring(cmd->tn_left.tn_node, string);
			sprintf(string, "|%% ");
			string += 3;
			break;
		case ANDP:	/* && */
			string = treestring(cmd->tn_left.tn_node, string);
			sprintf(string, "&& ");
			string += 3;
			break;
		case ORP:	/* || */
			string = treestring(cmd->tn_left.tn_node, string);
			sprintf(string, "|| ");
			string += 3;
			break;
		case '(':
			*string++ = '(';
			string = treestring(cmd->tn_left.tn_node, string);
			*string++ = ')';
			break;
		case CMD:
			string = treestring(cmd->tn_left.tn_node, string);
			break;

		case '<':
		case DOCIN:	/* << */
		case '>':
		case OUTAPP:	/* >> */
		case '%':
		case ERRAPP:	/* %% */
			sprintf(string, "%c", (int)ntype(cmd));
			string++;
			if ((ntype(cmd) >> 8) != 0) {
				sprintf(string, "%c", ((int)ntype(cmd) >> 8));
				string++;
			}
			string = sprintstring(string, cmd);
			break;
		case STRING:
		case LSTRING:
			string = sprintstring(string, cmd);
			break;
		}
		cmd = cmd->tn_right.tn_node;
	}
	return (string);
}


LOCAL char *
sprintstring(string, cmd)
	register char	*string;
	register Tnode	*cmd;
{
	register char	*s;

	switch ((int)quotetype(cmd)) {

	case NOQUOTE:
		s = quote_string(cmd->tn_left.tn_str, special);
		sprintf(string, "%s ", s);
		string += strlen(s);
		string++;
		break;

	case SQUOTE:
		sprintf(string, "'%s' ", cmd->tn_left.tn_str);
		string += 3 + strlen(cmd->tn_left.tn_str);
		break;

	case DQUOTE:
		sprintf(string, "\"%s\" ", cmd->tn_left.tn_str);
		string += 3 + strlen(cmd->tn_left.tn_str);
		break;
	case BQUOTE:
		sprintf(string, "`%s` ", cmd->tn_left.tn_str);
		string += 3 + strlen(cmd->tn_left.tn_str);
		break;
	}
	return (string);
}


EXPORT void
printtree(f, cmd)
	FILE	*f;
	Tnode	*cmd;
{
	char str[512];

	treestring(cmd, str);
	fprintf(f, "%s", str);
}

LOCAL Tnode *
stringtree(string)
	char *string;
{
	Tnode *ptr;

	pushline(string);
	ptr = cmdline(0, gstd, TRUE);
	return (ptr);
}

LOCAL char *
nextwd(line, word)
	char	*line;
	char	*word;
{
/*
 * gets next word from line, puts line into word, terminated by nullbyte
 * and returns with pointer to next word.
 */
	char *p, *d, *i;

	word[0] = 0;
	if (line == NULL)
		return (NULL);
	p = strchr(line, ' ');
	if (p == NULL) {
		sprintf(word, "%s", line);
	} else {
		d = word;
		for (i = line; i < p; i++)
			*d++ = *i;
		*d = 0;
		while (*p == ' ')
			p++;
		if (*p == 0)
			p = NULL;
	}
	return (p);
}

LOCAL int
strmatch(pline, string)
	char	*pline;
	char	*string;
{
	char *blankp, *blankw, *flg, pattern[100], word[100];
	BOOL found;
	int lenp, lenw, alt, aux[100], state[100];

	blankp = nextwd(pline, pattern);
	blankw = nextwd(string, word);
	do {
		lenp = strlen(pattern);
		lenw = strlen(word);
		if (lenp > 100) {
			fprintf(stderr, "pattern too long: %s\n", string);
			return (-1);
		}
		alt = patcompile((unsigned char *)pattern, lenp, aux);
		if (alt == 0) {
			error("error in pattern: %s", pattern);
			return (-1);
		}
		if (lenw == 0)
			flg = NULL;
		else
			flg = (char *)patmatch((unsigned char *)pattern, aux,
					(unsigned char *)word, 0, lenw, alt, state);
		found = (flg == NULL) ? FALSE : TRUE;
		blankp = nextwd(blankp, pattern);
		blankw = nextwd(blankw, word);
	} while (found && pattern[0] != 0);
	return (found ? 1 : 0);
}

LOCAL void
get_str(str)
	char	*str;
{
	register char	c;

	while ((c = getchar()) != '\n')
		*str++ = c;
	*str = 0;
}

LOCAL char *
getsub(del, line, ret)
	char	del;
	char	*line;
	char	*ret;
{
	while (*line != del && *line != 0)
		*ret++ = *line++;
	if (*line == 0)
		return (NULL);
	*ret = 0;
	return (line);
}

EXPORT void
phistory(edit)
	BOOL	edit;
{
	char	*name, string[512], exflg;
	Tnode	*cbase;
	int	i, max, found;

	if (history == 0) {
		fprintf(stderr, "history not set.\n");
		while (!argend(nullstr, 0))
			nextch();	/* eat rest of line */
		return;
	}
	if ((name = pstring(nl, 0)) != NULL) {
		if ((max = high_hist()) == -1)
			return;
		if ((streql(name, "^^") && !edit) ||
			(streql(name, "~~") && edit)) {
			i = max;
		} else {
			found = 0;
			for (i = max; i >= 0 && found == 0; i--)
			{
				treestring(cur_base[i], string);
				found = strmatch(&name[1], string);
			}
			if (found < 0)
				return;		/* error in pattern */
			if (found == 0) {
				fprintf(stderr, "no match.\n");
				return;
			}
			i++;
		}
		cbase = cur_base[i];
		if (edit) {
			exflg = ehist(&cbase);
			cur_base[i] = cbase;
		} else {
			exflg = TRUE;
		}
		lr_used(i);
		if (exflg)
			execute(cbase, 0, gstd);
		lastcmd = cbase;
	}
}

LOCAL BOOL
ehist(base)
	Tnode	**base;
{
	char *cmdlin, edline[80], pattern[80], rep[80];
	char *ptr, *ptr3, *tmp, *edptr, del;

#ifdef DEBUG
	printf("here is ehist !!!!\n");
#endif /* DEBUG */
	if ((cmdlin = malloc(512)) == NULL) {
		error(sn_no_mem);
		return (FALSE);
	}
	if ((tmp = malloc(512)) == NULL) {
		free(cmdlin);
		error(sn_no_mem);
		return (FALSE);
	}
#ifdef DEBUG
	printf("allocated memory...\n");
#endif /* DEBUG */
	treestring(*base, cmdlin);
#ifdef DEBUG
	printf("editing <%s> with cmdlin at %p,", cmdlin, cmdlin);
	printf(" tmp at %p\n", tmp);
#endif /* DEBUG */
	do {
		edptr = edline;
		fprintf(stderr, "{ %s}\n%s", cmdlin, prompts[1]);
		get_str(edptr);
		switch (*edptr) {

		case 's':
			edptr++;
			del = *edptr++;
			if ((ptr = getsub(del, edptr, pattern)) == NULL) {
				fprintf(stderr, "bad pattern.\n");
				*edptr = 1;
				break;
			}
			if (pattern[0] == 0) {
				fprintf(stderr, "bad pattern.\n");
				*edptr = 1;
				break;
			}
			ptr++;
			if (getsub(del, ptr, rep) == NULL) {
				fprintf(stderr, "bad pattern.\n");
				*edptr = 1;
				break;
			}
			ptr3 = strindex(pattern, cmdlin);
			if (ptr3 == NULL) {
				fprintf(stderr, "not found.\n");
				*edptr = 1;
				break;
			}
#ifdef	DEBUG
			printf("ptr=<%s> ", ptr);
			printf("ptr3=<%s> ", ptr3);
			printf("pattern=<%s> ", pattern);
			printf("rep=<%s>\n", rep);
#endif /* DEBUG */
			*ptr3 = 0;
			ptr3 += strlen(pattern);
			if (*ptr3)
				sprintf(tmp, "%s%s%s", cmdlin, rep, ptr3);
			else
				sprintf(tmp, "%s%s", cmdlin, rep);
			sprintf(cmdlin, "%s", tmp);
#ifdef DEBUG
			printf("cmdlin = <%s>\n", cmdlin);
#endif /* DEBUG */
			break;
		case 0:
		case 'x':
			break;
		default:
			fprintf(stderr, "illegal command.\n");
		}
	} while (*edptr != 0 && *edptr != 'x');
#ifdef DEBUG
	printf("break from loop with %02x\n", *edptr);
	printf("releasing tree <");
	printtree(stdout, *base);
	printf(">\n");
#endif /* DEBUG */
	freetree(*base);
	*base = stringtree(cmdlin);
	free(cmdlin);
	free(tmp);
#ifdef DEBUG
	printf("returning with %s\n", *edptr == 'x' ? "TRUE" : "FALSE");
#endif /* DEBUG */
	return (*edptr == 'x');
}

EXPORT void
hi_list(f)
	FILE	*f;
{
/* print history */

	int i;

	if (history == 0)
		fprintf(f, "history not set.\n");
	else
		for (i = 0; i < history; i++) {
			if (cur_base[i] != (Tnode *) NULL) {
				fprintf(f, "{ ");
				printtree(f, cur_base[i]);
				fprintf(f, "}\n");
			}
		}
}

EXPORT BOOL
sethistory(p)
	char	*p;
{
/* set history, if not already set (except for value zero) */

	int nval, i;
	char *ptr;

	ptr = astoi(p, &nval);
	if (*ptr != 0) {
		fprintf(stderr, "bad number\n");
		return (0);
	}
	if (history == 0) {
		if (nval == 0)
			return (1);
		if ((cur_base =
		    (Tnode **) malloc(nval*sizeof (Tnode *))) == (Tnode **) NULL)
			error(sn_no_mem);

		for (i = 0; i < nval; i++)
			cur_base[i] = (Tnode *) NULL;
		history = nval;
		return (1);
	} else {
		if (nval > 0) {
			fprintf(stderr, "history already set\n");
			return (0);
		} else {
			free(cur_base);
			history = 0;
			return (1);
		}
	}
}

EXPORT void
inithistory()
{
	char	*p;
	int	n;

	if ((p = getcurenv(histname)) == NULL) {
		clearhistory();
	} else {
		if (*astoi(p, &n) != '\0') {
			error("bad number '%s' in HISTORY.", p);
			clearhistory();
		} else {
			sethistory("0");
			sethistory(p);
		}
	}
}

LOCAL void
clearhistory()
{
/* initialize history with 0 */

	ev_insert(makestr("HISTORY=0"));
	history = 0;
}

EXPORT void
readhistory(f)
	FILE	*f;
{
}

#define	LINEQUANT	64
#define	new_line(lp, old, new)	realloc(lp, new)

extern	int	delim;

/* VARARGS1 */
EXPORT char *
make_line(f, arg)
	register int	(*f) __PR((FILE *));
	register FILE	*arg;
{
	register unsigned	maxl;
	register unsigned	llen;
			char	*lp;
	register	char	*p;
	register 	int	c;

/*#define	DEBUG*/
#ifdef DEBUG
	printf("        make_line\r\n");
#endif
	lp = p = malloc(maxl = LINEQUANT);
	llen = 0;
	for (;;) {
		if ((c = (*f)(arg)) == EOF || c == '\n' || c == '\205') {
			if (c == EOF)
				delim = EOF;
			else
				delim = '\n'; /*XXX ??? \205 ???*/
			*p = 0;
			return (lp);
		}
		*p++ = c;
		if (++llen == maxl) {
			maxl += LINEQUANT;
			lp = new_line(lp, llen, maxl);
			p = lp + llen;
		}
	}
}

#endif /* INTERACTIVE */
