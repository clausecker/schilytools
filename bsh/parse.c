/* @(#)parse.c	1.38 18/06/27 Copyright 1985-2018 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)parse.c	1.38 18/06/27 Copyright 1985-2018 J. Schilling";
#endif
/*
 *	bsh command interpreter - Command Line Parser
 *
 *	Copyright (c) 1985-2018 J. Schilling
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
#include "str.h"
#include "strsubs.h"
#include "node.h"
#include "ctype.h"

#define	MAXARG	8192	/* old 512 Joerg fuer Ulrich 1024!!!*/

LOCAL int need_par	= FALSE;

int	nerrors = 0;

extern int	ctlc;
extern int	delim;

LOCAL	void	Printtree	__PR((FILE * f, Tnode * cmd));
LOCAL	void	Printtype	__PR((FILE * f, Tnode * cmd));
EXPORT	Tnode	*cmdline	__PR((int flag, FILE ** std, int buildcmd));
#ifdef	INTERACTIVE
LOCAL	BOOL	phist		__PR((int flag, FILE ** std));
#endif
LOCAL	Tnode	*pcmdlist	__PR((void));
LOCAL	Tnode	*pcond		__PR((void));
LOCAL	Tnode	*pfilter	__PR((void));
LOCAL	Tnode	*pcmd		__PR((void));
EXPORT	Tnode	*pword		__PR((void));
EXPORT	char	*pstring	__PR((char *terms, int quotec));
EXPORT	BOOL	argend		__PR((char *term, int quotec));
LOCAL	Tnode	*piolist	__PR((Tnode *list));
LOCAL	Tnode	*pio		__PR((void));
LOCAL	int	iotype		__PR((long m));
LOCAL	int	iocheck		__PR((int ty, Tnode *np));
LOCAL	void	need		__PR((int c));
EXPORT	int	skipwhite	__PR((void));
EXPORT	void	eatline		__PR((void));
EXPORT	void	syntax		__PR((char *s, ...));

LOCAL void
Printtree(f, cmd)
	register FILE	*f;
	register Tnode	*cmd;
{
	static	int	par	= 0;

	if (cmd == (Tnode *)NULL)
		Printtype(f, cmd);

	while (cmd != (Tnode *) NULL) {

		Printtype(f, cmd);
		fprintf(f, "#%d[ ", ++par);

		switch ((int)ntype(cmd)) {

		case '&':
		case '|':
		case ';':
			Printtree(f, cmd->tn_left.tn_node);
			fprintf(f, "%c ", (int)ntype(cmd));
			break;
		case ERRPIPE:	/* |% */
			Printtree(f, cmd->tn_left.tn_node);
			fprintf(f, "|%% ");
			break;
		case ANDP:	/* && */
			Printtree(f, cmd->tn_left.tn_node);
			fprintf(f, "&& ");
			break;
		case ORP:	/* || */
			Printtree(f, cmd->tn_left.tn_node);
			fprintf(f, "|| ");
			break;
		case '(':
			putc('(', f);
			Printtree(f, cmd->tn_left.tn_node);
			putc(')', f);
			break;
		case CMD:
			Printtree(f, cmd->tn_left.tn_node);
			break;
		case '<':
		case DOCIN:	/* << */
		case '>':
		case OUTAPP:	/* >> */
		case '%':
		case ERRAPP:	/* %% */
		case IDUP:	/* <& */
		case ODUP:	/* >& */
			_printio(f, cmd->tn_type);
			printstring(f, cmd);
			break;
		case STRING:
		case LSTRING:
			printstring(f, cmd);
			break;
		}
		cmd = cmd->tn_right.tn_node;
		fprintf(f, "#%d] ", par--);
	}
}

LOCAL void
Printtype(f, cmd)
	register FILE	*f;
	register Tnode	*cmd;
{
	if (cmd == (Tnode *)NULL)
		fprintf(f, "#NULL ");
	else
		switch ((int)ntype(cmd)) {

		case '&':
		case '|':
		case ';':
			fprintf(f, "#%c ", (int)ntype(cmd));
			break;
		case ERRPIPE:	/* |% */
			fprintf(f, "#|%% ");
			break;
		case ANDP:	/* && */
			fprintf(f, "#&& ");
			break;
		case ORP:	/* || */
			fprintf(f, "#|| ");
			break;
		case '(':
			fprintf(f, "#( ");
			break;
		case CMD:
			fprintf(f, "#CMD ");
			break;
		case '<':
		case DOCIN:	/* << */
		case '>':
		case OUTAPP:	/* >> */
		case '%':
		case ERRAPP:	/* %% */
		case IDUP:	/* <& */
		case ODUP:	/* >& */
			fprintf(f, "#");
			printio(f, cmd);
			fprintf(f, " ");
			break;
		case STRING:
		case LSTRING:
			fprintf(f, "#STRING ");
			break;
		}
}

/*
 * The top level funtion of the command line parser.
 * This function is not recursively called from lower levels of the parser.
 *
 * Note that skipwhite() stops after reading the first non-white character
 * and thus triggers alias expansions. The begin alias expansion state needs
 * to have the right value for the following word.
 */
Tnode *
cmdline(flag, std, buildcmd)
	int	flag;
	FILE	*std[];
	char	buildcmd;
{
	Tnode	*cmd;

#ifdef DEBUG
	printf("cmdline...\n");
#endif
	nerrors = 0;		/* No errors yet */
	parseflg = TRUE;	/* Set logical start of parsing */
	begina(TRUE);		/* Set logical start of new cmd */
	nextch();
	skipwhite();		/* Skip all initial whitespace */
	cmd = (Tnode *) NULL;
#ifdef INTERACTIVE		/* Using command line history editor */
	if (delim == '#')
		hashcmd(std);
	else if (delim != '!' || !phist(flag, std))
		cmd = pcmdlist();
#else
	if (delim == '^' || delim == '~')
		phistory(delim == '~');
	else
		if (delim == '#')
			hashcmd(std);
		else
			cmd = pcmdlist();
#endif
	if (nerrors)
		eatline();	/* XXX quote() ?? Don't want rest of line */
	parseflg = FALSE;	/* parsing done */
	if (!ctlc && nerrors == 0 && cmd != (Tnode *) NULL) {
/*		Printtree(std[1], cmd);*/
/*		fprintf(std[1], nl);*/
		if (!buildcmd)
			execute(cmd, flag, std); /* Execute parsed cmd tree */
		return (cmd);
	}
	freetree(cmd);
	return ((Tnode *) NULL);
}


#ifdef	INTERACTIVE
/*
 * Parse a history command
 */
LOCAL BOOL
phist(flag, std)
	int	flag;
	FILE	*std[];
{
	char	*pattern;
	char	*line;

	if (get_histlen() == 0)
		return (FALSE);
	quote();
	nextch();					/* skip '!' */
	pattern = pstring(spaces, 0);			/* get a word */
	unquote();
	if (pattern == NULL) {
		ungetch(delim);
		delim = '!';
		return (FALSE);
	}
	if ((line = match_hist(pattern)) != NULL) {
		pushline(line);
		freetree(cmdline(flag, std, FALSE));
	}
	eatline();
	return (TRUE);
}
#endif


/*
 * Parse a list of commands separated by '&' or ';'
 * Example: sleep 10; ls
 */
LOCAL Tnode *
pcmdlist()
{
	register Tnode	*np;
	register long	type;

	begina(TRUE);				/* New cmd gets begin aliases */
	np = pcond();
	type = delim;
	if (type == '&' || type == ';') {
		nextch();
		np = allocnode(type, np, pcmdlist());
		type = delim;
	}
	return (np);
}


/*
 * Parse a conditional expression separated by '&&' or '||'
 * Example: cd somedir && ls
 */
LOCAL Tnode *
pcond()
{
	register Tnode	*lp;
	register Tnode	*rp;
	register long	type;

	lp = pfilter();
	type = delim;
	if (type == '&' || type == '|') {
		if (type != nextch()) {
			ungetch(delim);
			delim = type;
		} else {
			type = (type << 8) | delim;
			begina(TRUE);
			nextch();
			rp = pcond();
			if (lp == (Tnode *) NULL || rp == (Tnode *) NULL)
				syntax("%s", emisscondcmd);
			lp = allocnode(type, lp, rp);
		}
	}
	return (lp);
}


/*
 * Parse a pipeline expression separated by '|' or '|%'
 * Example: ls | more
 */
LOCAL Tnode *
pfilter()
{
	register Tnode	*lp;
	register Tnode	*rp;
	register long	type;

	lp = pcmd();				/* Parse next command */
	type = delim;
	if (type == '|') {
		if (type == nextch()) {
			ungetch(delim); 	/* put back || conditional */
		} else {
			if (delim == '%') {	/* errpipe */
				type = ERRPIPE;	/* |% */
						/* Input module believes */
						/* that % was first token */
				begina(TRUE);	/* so allow begin aliases */
				nextch();
			}
			rp = pfilter();
			if (lp == (Tnode *) NULL|| rp == (Tnode *) NULL)
				syntax("%s", emisspipecmd);
			if (iocheck(type == ERRPIPE ? 2 : 1, lp) || iocheck(0, rp))
				syntax("%s", epiperedefio);
			lp = allocnode(type, lp, rp);
		}
	}
	return (lp);
}


/*
 * Parse a simple command including I/O redirection via '<' / '>'
 */
LOCAL Tnode *
pcmd()
{
	register Tnode	*ap;
	register Tnode	*np;
	register Tnode	*ip;
		Tnode	atmp;

	skipwhite();
	if (argend(ops, 0)) {		/* Found an empty command */
		begina(TRUE);
		return ((Tnode *) NULL);
	}
	ip = piolist((Tnode *) NULL);	/* Parse leading I/O directives */
	if (ip == NULL) {
		/*
		 * If no leading I/O directives have been found, alias
		 * expansion was already triggered from within piolist().
		 * We thus need to disable begin alias expansion here.
		 */
		begina(FALSE);
	}
	if (delim == '(') {
		need_par++;
		begina(TRUE);
		nextch();
		np = pcmdlist();	/* Parse a complex parenthesised cmd */
		need(')');
		need_par--;
		np = allocnode((long) '(', np, piolist(ip));
	} else {
		BOOL	isav0 = TRUE;
		/*
		 * Create argument list for command
		 */
		ap = &atmp;
		while ((ap->tn_right.tn_node = pword()) != NULL ||
			delim == '(' || (delim == ')' && !need_par)) {
			if (ap->tn_right.tn_node == (Tnode *) NULL) {
				ap->tn_right.tn_node = allocnode(STRING,
				    (Tnode *) makestr(delim == '(' ? "(" :")"),
				    (Tnode *) NULL);
				nextch();
			}
			/*
			 * "name=value ..." -> "env name=value ..." + NENV flag
			 */
			if (isav0 &&
			    strchr((char *)ap->tn_right.tn_node->tn_left.tn_node,
								'=') != NULL) {
				Tnode *ep = ap->tn_right.tn_node;

				ap->tn_right.tn_node = allocnode(STRING | NENV,
						    (Tnode *) makestr("env"),
						    (Tnode *) NULL);
				ap = ap->tn_right.tn_node;
				ap->tn_right.tn_node = ep;
			}
			isav0 = FALSE;
			setbegina();		/* Begin alias on next word? */
			ap = ap->tn_right.tn_node;
			ip = piolist(ip);
		}
		np = allocnode(CMD, atmp.tn_right.tn_node, ip);
		begina(TRUE);			/* Begin alias for next cmd */
	}
	return (np);
}


#ifdef	OLD_PWORD

/*
 * Parse one word
 */
EXPORT Tnode *
pword()
{
	register long	type = STRING;
	register int	q = 0;
	register char	*s;

again:
	skipwhite();
	if (isdigit(delim)) {
		if (peekch() == '>')
			return (NULL);
	}
	if (delim == '\'' || delim == '"' || delim == '`') {
		q = delim;
		if (q == '\'') {
			quote();
		} else if (q == '"') {
			dquote();
			xquote();
		}
		nextch();
		unxquote();
	}
	/*
	 * Check if it was "$@" with empty list.
	 */
	if (delim < -1) {
		int	pc = peekch();

		if (pc == q) {
			nextch();	/* Eat quotechar */
			if (pc == '\'')
				unquote();
			if (pc == '"')
				undquote();
		}
		nextch();		/* Get next char for skipwhite() */
		q = 0;
		goto again;
	}
	s = pstring(special, q);
	if (s == NULL)
		return ((Tnode *) NULL);
	if (q) {
		if (q == '\'') {
			unquote();
			type = SQUOTE;
		} else if (q == '"') {
			undquote();
			type = DQUOTE;
		} else {
			type = BQUOTE;
		}
		need(q);
	}
	begina(FALSE);
	return (allocnode(type, (Tnode *) s, (Tnode *) NULL));
}

/*
 * Parse a string
 * Stop at special chars and whitespace
 */
EXPORT char *
pstring(terms, quotec)
		char	*terms;
		int	quotec;
{
		char	buf[MAXARG];
	register char	*t;
	register int	flg = 0;

	/*
	 * Check if it ends the right away
	 */
	if (!quotec && delim != '\\' && argend(terms, quotec))
		return (NULL);

	t = buf;
	while (delim == '\\' || !argend(terms, quotec)) {
		if (t >= &buf[MAXARG-1]) {
			if (!flg) {
				syntax("%s", eargtoolong);
				flg++;
			}
			quote();
			eatline();
			return (NULL);
		} else {
			if (delim == '\\') {
				if (nextch() != '\n' && delim != '\205' && delim != '\'' && quoting()) {
					ungetch(delim);
					delim = '\\';
				}
			}
			*t++ = delim;
		}
		nextch();
	}
	*t = 0;
	return (makestr(buf));
}

#else /* OLD_PWORD */

/*
 * Parse one word
 */
EXPORT Tnode *
pword()
{
	register long	type = STRING;
	register char	*s;

	skipwhite();
	s = pstring(special, 0);
	if (s == NULL)
		return ((Tnode *) NULL);
	begina(FALSE);
	return (allocnode(type, (Tnode *) s, (Tnode *) NULL));
}

char *
xpstring(terms, quotec)
		char	*terms;
		int	quotec;
{
		char	buf[MAXARG];
	register char	*t;
	register int	flg = 0;

	/*
	 * Check if it ends the right away
	 */
	if (!quotec && delim != '\\' && argend(terms, quotec))
		return (NULL);

	t = buf;
	for (; delim != EOF; ) {
/*		error("  quotec: %x delim: %c", quotec, delim);*/
		if (t >= &buf[MAXARG-1]) {
			if (!flg) {
				syntax("%s", eargtoolong);
				flg++;
			}
		} else {
			if (quotec == '\0') {
				if (delim == '\'' || delim == '"') {
					quote();
					quotec = delim;
				} else if (delim == '\\') {
					quotec = '\\';
				}
			} else if (quotec == '\\') {
				quotec = '\0';
			} else if (delim == quotec) {
				unquote();
				quotec = '\0';
			}
			*t++ = delim;
		}
/*		error("3 quotec: %c %x delim: %c", quotec, quotec, delim);*/
		if (quotec && delim != quotec) {
			nextch();
			continue;
		}
		nextch();
		if (delim != '\\' && argend(terms, quotec))
			break;
	}
	*t = 0;
	return (makestr(buf));
}

/*
 * Parse a string
 * Stop at special chars and whitespace
 */
EXPORT char *
pstring(terms, quotec)
		char	*terms;
		int	quotec;
{
		char	buf[MAXARG];
	register char	*t;
	register int	flg = 0;

	/*
	 * Check if it ends correctly
	 */
	if (!quotec && delim != '\\' && argend(terms, quotec))
		return (NULL);

	t = buf;
	for (; delim != EOF; ) {
/*		error("  quotec: %x delim: %c", quotec, delim);*/
		if (t >= &buf[MAXARG-1]) {
			if (!flg) {
				syntax("%s", eargtoolong);
				flg++;
			}
		} else {
			*t++ = delim;
			if (delim == '\\') {
				if ((nextch() == '\n' || delim == '\205') && quotec != '\'') {
					t--;
					nextch();
				}
				if (t >= &buf[MAXARG-1]) {
					if (!flg) {
						syntax("%s", eargtoolong);
						flg++;
					}
				} else {
					*t++ = delim;
				}
			} else if (delim == '\'' || delim == '"') {
/*				error("0 quotec: %x delim: %c", quotec, delim);*/
				if (quotec == '\0' && quotec != delim) {
/*					error("1 quotec: %x delim: %c", quotec, delim);*/
					quotec = delim;
					quote();
					nextch();
					continue;
				} else if (quotec && quotec == delim) {
/*					error("2 unquote: %c", delim);*/
					unquote();
					quotec = '\0';
				}
			}
		}
/*		error("3 quotec: %c %x delim: %c", quotec, quotec, delim);*/
		if (quotec && delim != quotec) {
			nextch();
			continue;
		}
		nextch();
		if (delim != '\\' && argend(terms, quotec))
			break;
	}
	*t = 0;
	return (makestr(buf));
}

/*
 * Parse a string
 */
EXPORT char *
opstring(terms, quotec)
		char	*terms;
		int	quotec;
{
		char	buf[MAXARG];
	register char	*t;
	register int	flg = 0;

	/*
	 * Check if it ends correctly
	 */
	if (!quotec && delim != '\\' && argend(terms, quotec))
		return (NULL);

	t = buf;
	while (delim == '\\' || !argend(terms, quotec)) {
		if (t >= &buf[MAXARG-1]) {
			if (!flg) {
				syntax("%s", eargtoolong);
				flg++;
			}
		} else {
			if (delim == '\\') {
				if (nextch() != '\n' && delim != '\205' && delim != '\'' && quoting()) {
					ungetch(delim);
					delim = '\\';
				}
			} else if (delim == '\'' || delim == '"') {
				if (!quotec) {
					*t++ = quotec = delim;
					quote();
					nextch();
					while (!argend(terms, quotec)) {
						fprintf(stderr, "%c\n", delim);
						*t++ = delim;
						nextch();
					}
					unquote();
					quotec = '\0';
				}
			}
			*t++ = delim;
		}
		nextch();
	}
	*t = 0;
	return (makestr(buf));
}

nextchar(quote)
		int	quote;
{
	static	int	peekc;

	while (nextch() == /* ESCAPE */ '\\') {
		if (nextch() == /* NL */ '\n' || delim == '\205')
			continue;
/* 		else if (quote && delim != quote && !escchar(delim)) {*/
		else if (quote && delim != quote &&
					strchr("\"`", delim) == NULL) {
			peekc = delim | /* MARK */ 0400;
			return (/* ESCAPE */ '\\');
		} else
			break;
	}
	return (delim);
}

#endif /* OLD_PWORD */

EXPORT BOOL
argend(term, quotec)
	char	*term;
	int	quotec;
{
	if (delim == '\n' || delim == '\205' || delim == EOF)
		return (TRUE);
	if (quotec != 0)
		return (delim == quotec);
	else
		return ((BOOL) (strchr(term, delim) != 0));
}


/*
 * Parse a list of I/O redirection commands
 */
LOCAL Tnode *
piolist(list)
	register Tnode	*list;
{
	register Tnode	*ip;
	register Tnode	*lp = list;

	/*
	 * In order to parse the iolist in a POSIX compliant way,
	 * we need to append new entries to the end of the list.
	 */
	if (lp) {
		while (lp->tn_right.tn_node != NULL)
			lp = lp->tn_right.tn_node;
	}

	while ((ip = pio()) != NULL) {
		if (iocheck(iotype(ip->tn_type), list))
			syntax("%s", eioredef);

		if (lp)
			lp->tn_right.tn_node = ip;
		else
			list = ip;
		lp = ip;
	}
	return (list);
}


/*
 * Parse a single I/O redirection command
 */
LOCAL Tnode *
pio()
{
		int obegin = getbegina();	/* Remember alias state */
	register long	mode = (long) skipwhite();
	register Tnode	*ip  = (Tnode *) NULL;
		int	fd = -1;
		int	fd2 = -1;

	if (isdigit(mode) && peekch() == '>') {
		fd = mode - '0';
		mode = nextch();
		if (fd != 1 && fd != 2)
			syntax("unsupported redirection fd %d", fd);
		if (peekch() == '&') {
			nextch();
			mode = nextch();
			if (!isdigit(mode)) {
				syntax("unsupported source fd %c", (char)mode);
				return ((Tnode *)NULL);
			}
			fd2 = mode - '0';
			if (fd2 != 1 && fd2 != 2) {
				syntax("unsupported source fd %d", fd2);
				return ((Tnode *)NULL);
			}
			nextch();
			mode = ((fd << 8) | fd2) << 16;
			ip = allocnode(ODUP|mode, (Tnode *)NULL, (Tnode *)NULL);
			return (ip);
		}
	}
	if (strchr("<>%", (char) mode) != NULL) {
		if (nextch() == mode) {
			mode = (mode << 8)|mode;
			nextch();
		}
		if (fd == 2) {
			if (mode == '>')
				mode = '%';
			else if (mode == OUTAPP)
				mode = ERRAPP;
		}
		begina(FALSE);		/* No begin aliases for file names */
		ip = pword();
		begina(obegin);		/* Restore old alias expansion state */
		if (ip == (Tnode *) NULL) {
			if (mode == DOCIN) {	/* << */
				syntax("%s", emissiodelim);
			} else {
				syntax("%s", emissnameinio);
			}
		} else {
			ip->tn_type |= mode;
		}
	}
	return (ip);
}


LOCAL int
iotype(m)
	long	m;
{
	switch ((int)xntype(m)) {

	case '<':
	case DOCIN:	/* << */
		return (0);
	case '>':
	case OUTAPP:	/* >> */
		return (1);
	case '%':
	case ERRAPP:	/* %% */
		return (2);
	case IDUP:	/* <& */
	case ODUP:	/* >& */
		m >>= 16;
		return (m >> 8);
	default:
		raisecond("!iotype", (long)NULL);
		return (0);
	}
}


LOCAL int
iocheck(ty, np)
	register int	ty;
	register Tnode	*np;
{
	register long	t;

	if (np == (Tnode *) NULL)
		return (0);
	t = ntype(np);
	switch ((int)t) {

	case '(':
		return (iocheck(ty, np->tn_left.tn_node) ||
			iocheck(ty, np->tn_right.tn_node));
	case '|':
	case ERRPIPE:	/* |% */
	case '&':
	case ';':
		return (((t == '|' && ty == 1) || (t == ERRPIPE && ty == 2) ||
			    iocheck(ty, np->tn_left.tn_node)) &&
			    (((t == '|' || t == ERRPIPE) && ty == 0) ||
			    iocheck(ty, np->tn_right.tn_node)));
	case '<':
	case DOCIN:	/* << */
	case '>':
	case OUTAPP:	/* >> */
	case '%':
	case ERRAPP:	/* %% */
	case IDUP:	/* <& */
	case ODUP:	/* >& */
		return (iotype(np->tn_type) == ty || iocheck(ty, np->tn_right.tn_node));
	case CMD:
		return (iocheck(ty, np->tn_right.tn_node));
	default:
		raisecond("!iocheck", (long)NULL);
		return (0);
	}
}

LOCAL void
need(c)
	int c;
{
	if (delim != c)
		syntax("%s %c", emissing, c);
	else
		nextch();
}

EXPORT int
skipwhite()
{
	while (iswhite(delim))
		nextch();
	return (delim);
}

EXPORT void
eatline()
{
	while (!argend(nullstr, 0))
		nextch();	/* eat rest of line */
	while (quoting())
		unquote();
}

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT void
syntax(char *s, ...)
#else
EXPORT void
syntax(s, va_alist)
	char	*s;
	va_dcl
#endif
{
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, s);
#else
	va_start(args);
#endif
	fprintf(stderr, "%r\n", s, args);
	va_end(args);
	fflush(stderr);
	nerrors++;
}
