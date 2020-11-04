/* @(#)ctags.c	1.13 20/03/17 Copyright 1985-2020 J. Schilling */
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static	char sccsid[] =
	"@(#)ctags.c	1.13 20/03/17 1985-2020 J. Schilling from UCB 5.1 5/31/85";
#endif /* not lint */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

/*
 * ctags: create a tags file
 */

#define	reg	register
#define	bool	char

#define	TRUE	(1)
#define	FALSE	(0)

#define	iswhite(arg)	(_wht[arg])	/* T if char is white		*/
#define	begtoken(arg)	(_btk[arg])	/* T if char can start token	*/
#define	intoken(arg)	(_itk[arg])	/* T if char can be in token	*/
#define	endtoken(arg)	(_etk[arg])	/* T if char ends tokens	*/
#define	isgood(arg)	(_gd[arg])	/* T if char can be after ')'	*/

#define	max(I1, I2)	(I1 > I2 ? I1 : I2)

struct	nd_st {			/* sorting structure			*/
	char	*entry;			/* function or type name	*/
	char	*file;			/* file name			*/
	bool	f;			/* use pattern or line no	*/
	int	lno;			/* for -x option		*/
	char	*pat;			/* search pattern		*/
	bool	been_warned;		/* set if noticed dup		*/
	struct	nd_st	*left, *right;	/* left and right sons		*/
};

long	ftell();
typedef	struct	nd_st	NODE;

static bool	number,			/* T if on line starting with #	*/
#if 0
	term	= FALSE,		/* T if print on terminal	*/
	makefile = TRUE,		/* T if to creat "tags" file	*/
#endif
	gotone,				/* found a func already on line	*/
					/* boolean "func" (see init)	*/
	_wht[0177], _etk[0177], _itk[0177], _btk[0177], _gd[0177];

	/* typedefs are recognized using a simple finite automata,
	 * tydef is its state variable.
	 *
	 * none:	Initial state
	 * ext:		"extern" found
	 * begin:	"typedef" found
	 * begin2:	"struct", "union" or "enum" found
	 * begin3:	"struct", "union" or "enum" found with -t in effect
	 * middle:	past '{' in a typedef
	 * end:		past '}' in a typedef
	 */
typedef enum {none, ext, begin, begin2, begin3, middle, end } TYST;

#ifdef	DEBUG
char *tdnm[] = { "none", "ext", "begin", "begin2", "begin3", "midle", "end" };
#endif

static TYST tydef = none;
static bool strdef = FALSE;

static char	searchar = '/';		/* use /.../ searches 		*/

static int	lineno;			/* line number of current line */
static char	line[4*BUFSIZ],	/* current input line			*/
	*curfile,		/* current input file name		*/
	*outfile = "tags",	/* output file				*/
	*white	= " \f\t\n",	/* white chars				*/
	*endtk	= " \t\n\"'#()[]{}=-+%*/&|^~!<>;,.:?",
				/* token ending chars			*/
	*begtk	= "ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz",
				/* token starting chars			*/
	*intk	= "ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz0123456789",
				/* valid in-token chars			*/
	*notgd	= ",;";		/* non-valid after-function chars	*/

static int	file_num;	/* current file number			*/
static int	aflag;		/* -a: append to tags */
static int	tflag;		/* -t: create tags for typedefs */
static int	uflag;		/* -u: update tags */
static int	wflag;		/* -w: suppress warnings */
static int	vflag;		/* -v: create vgrind style index output */
static int	xflag;		/* -x: create cxref style output */

static char	lbuf[BUFSIZ];

static FILE	*inf,		/* ioptr for current input file		*/
	*outf;			/* ioptr for tags file			*/

static long	lineftell;	/* ftell after getc(inf) == '\n' 	*/

static NODE	*head;		/* the head of the sorted binary tree	*/

static void	init(void);
static void	find_entries(char *file);
/*void	pfnote(char *name, int ln, bool f);*/
static void	pfnote(char *name, int ln, int f);
static void	C_entries(void);
static int	start_entry(char **lp, char *token, char *tp, int *f);
static void	Y_entries(void);
static char *	toss_comment(char *start);
static void	getaline(void);
static void	free_tree(NODE *node);
static void	add_node(NODE *node, NODE *cur_node);
static void	put_entries(NODE *node);
static int	PF_funcs(FILE *fi);
static int	tail(char *cp);
static void	takeprec(void);
static void	getit(void);
static char *	savestr(char *cp);

static void	L_funcs(FILE *fi);
static void	L_getit(int special);
static int	striccmp(char *str, char *pat);
static int	first_char(void);
static void	toss_yysec(void);

#ifdef	__STDC__
#define	index	strchr
#define	rindex	strrchr
#else
char	*rindex(), *index();
#endif

int
main(ac, av)
int	ac;
char	*av[];
{
	char cmd[100];
	int i;

	while (ac > 1 && av[1][0] == '-') {
		for (i = 1; av[1][i]; i++) {
			switch(av[1][i]) {
			case 'B':
				searchar = '?';
				break;
			case 'F':
				searchar = '/';
				break;
			case 'a':
				aflag++;
				break;
			case 't':
				tflag++;
				break;
			case 'u':
				uflag++;
				break;
			case 'w':
				wflag++;
				break;
			case 'v':
				vflag++;
				xflag++;
				break;
			case 'x':
				xflag++;
				break;
			case 'f':
				if (ac < 2)
					goto usage;
				ac--, av++;
				outfile = av[1];
				goto next;
			default:
				goto usage;
		}
		}
	next:
		ac--; av++;
	}

	if (ac <= 1) {
usage:
		(void) printf(
			"Usage: ctags [-BFatuwvx] [-f tagsfile] file ...\n");
		exit(1);
	}

	init();			/* set up boolean "functions"		*/
	/*
	 * loop through files finding functions
	 */
	for (file_num = 1; file_num < ac; file_num++)
		find_entries(av[file_num]);

	if (xflag) {
		put_entries(head);
		exit(0);
	}
	if (uflag) {
		for (i = 1; i < ac; i++) {
			(void) sprintf(cmd,
			    "mv %s OTAGS;fgrep -v '\t%s\t' OTAGS >%s;rm OTAGS",
				outfile, av[i], outfile);
			(void) system(cmd);
		}
		aflag++;
	}
	outf = fopen(outfile, aflag ? "a" : "w");
	if (outf == NULL) {
		perror(outfile);
		exit(1);
	}
	put_entries(head);
	(void) fclose(outf);
	if (uflag) {
		(void) sprintf(cmd, "sort %s -o %s", outfile, outfile);
		(void) system(cmd);
	}
	exit(0);
}

/*
 * This routine sets up the boolean psuedo-functions which work
 * by seting boolean flags dependent upon the corresponding character
 * Every char which is NOT in that string is not a white char.  Therefore,
 * all of the array "_wht" is set to FALSE, and then the elements
 * subscripted by the chars in "white" are set to TRUE.  Thus "_wht"
 * of a char is TRUE if it is the string "white", else FALSE.
 */
static void
init()
{

	reg	char	*sp;
	reg	int	i;

	for (i = 0; i < 0177; i++) {
		_wht[i] = _etk[i] = _itk[i] = _btk[i] = FALSE;
		_gd[i] = TRUE;
	}
	for (sp = white; *sp; sp++)
		_wht[*sp] = TRUE;
	for (sp = endtk; *sp; sp++)
		_etk[*sp] = TRUE;
	for (sp = intk; *sp; sp++)
		_itk[*sp] = TRUE;
	for (sp = begtk; *sp; sp++)
		_btk[*sp] = TRUE;
	for (sp = notgd; *sp; sp++)
		_gd[*sp] = FALSE;
}

/*
 * This routine opens the specified file and calls the function
 * which finds the function and type definitions.
 */
static void
find_entries(file)
char	*file;
{
	char *cp;

	if ((inf = fopen(file, "r")) == NULL) {
		perror(file);
		return;
	}
	curfile = savestr(file);
	lineno = 0;
	cp = rindex(file, '.');
	/* .l implies lisp or lex source code */
	if (cp && cp[1] == 'l' && cp[2] == '\0') {
		if (index(";([", first_char()) != NULL) {	/* lisp */
			L_funcs(inf);
			(void) fclose(inf);
			return;
		} else {					/* lex */
			/*
			 * throw away all the code before the second "%%"
			 */
			toss_yysec();
			getaline();
			pfnote("yylex", lineno, TRUE);
			toss_yysec();
			C_entries();
			(void) fclose(inf);
			return;
		}
	}
	/* .y implies a yacc file */
	if (cp && cp[1] == 'y' && cp[2] == '\0') {
		toss_yysec();
		Y_entries();
		C_entries();
		(void) fclose(inf);
		return;
	}
	/* if not a .c or .h file, try fortran */
	if (cp && (cp[1] != 'c' && cp[1] != 'h') && cp[2] == '\0') {
		if (PF_funcs(inf) != 0) {
			(void) fclose(inf);
			return;
		}
		rewind(inf);	/* no fortran tags found, try C */
	}
	C_entries();
	(void) fclose(inf);
}

static void
pfnote(name, ln, f)
char	*name;
int	ln;
bool	f;		/* f == TRUE when function */
{
	register char *fp;
	register NODE *np;
	char nbuf[BUFSIZ];

	if ((np = (NODE *) malloc(sizeof (NODE))) == NULL) {
		(void) fprintf(stderr, "ctags: too many entries to sort\n");
		put_entries(head);
		free_tree(head);
		head = np = (NODE *) malloc(sizeof (NODE));
	}
	if (xflag == 0 && !strcmp(name, "main")) {
		fp = rindex(curfile, '/');
		if (fp == 0)
			fp = curfile;
		else
			fp++;
		(void) sprintf(nbuf, "M%s", fp);
		fp = rindex(nbuf, '.');
		if (fp && fp[2] == 0)
			*fp = 0;
		name = nbuf;
	}
	np->entry = savestr(name);
	np->file = curfile;
	np->f = f;
	np->lno = ln;
	np->left = np->right = 0;
	if (xflag == 0) {
		lbuf[50] = 0;
		(void) strcat(lbuf, "$");
		lbuf[50] = 0;
	}
	np->pat = savestr(lbuf);
	if (head == NULL)
		head = np;
	else
		add_node(np, head);
}

/*
 * This routine finds functions and typedefs in C syntax and adds them
 * to the list.
 */
static void
C_entries()
{
	register int c;
	register char *token, *tp;
	bool incomm, inquote, inchar, midtoken;
	int level;
	char *sp;
	char tok[BUFSIZ];

	number = gotone = midtoken = inquote = inchar = incomm = FALSE;
	level = 0;
	sp = tp = token = line;
	lineno++;
	lineftell = ftell(inf);
	for (;;) {
		*sp = c = getc(inf);
		if (feof(inf))
			break;
		if (c == '\n')
			lineno++;
		else if (c == '\\') {
			c = *++sp = getc(inf);
			if (c == '\n') {
				c = ' ';
				lineno++;
			}
		} else if (incomm) {
			if (c == '*') {
				while ((*++sp = c = getc(inf)) == '*')
					continue;
				if (c == '\n')
					lineno++;
				if (c == '/')
					incomm = FALSE;
			}
		} else if (inquote) {
			/*
			 * Too dumb to know about \" not being magic, but
			 * they usually occur in pairs anyway.
			 */
			if (c == '"')
				inquote = FALSE;
			continue;
		} else if (inchar) {
			if (c == '\'')
				inchar = FALSE;
			continue;
		} else switch (c) {
		case '"':
			inquote = TRUE;
			continue;
		case '\'':
			inchar = TRUE;
			continue;
		case '/':
			if ((*++sp = c = getc(inf)) == '*')
				incomm = TRUE;
			else
				(void) ungetc(*sp, inf);
			continue;
		case '#':
			if (sp == line)
				number = TRUE;
			continue;
		case '{':
			if (tydef == ext) {
				tydef = none;
				continue;
			}
/*			if (tydef == begin) {*/
/*			if (tydef == begin2) {			/* JS */
			if (tydef == begin2 || tydef == begin3) { /* JS */
				tydef = middle;
			}
			level++;
			continue;
		case '}':
			if (sp == line)
				level = 0;	/* reset */
			else
				level--;
			if (level < 0) {
(void) fprintf(stderr, "level: %d lineno: %d line: %s\n",
				level, lineno, line);
				level = 0;
			}
			if (!level && tydef == middle) {
				tydef = end;
			}
			continue;
		}
		if (!level && !inquote && !incomm && gotone == FALSE) {
			if (midtoken) {
				if (endtoken(c)) {
					int f;
					int pfline = lineno;

					sp[1] = '\0';		/* terminate */
					if (start_entry(&sp, token, tp, &f)) {
						(void) strncpy(tok, token,
								tp-token+1);
						tok[tp-token+1] = 0;
						getaline();
						pfnote(tok, pfline, f);
						gotone = f;	/* function */
					}
					midtoken = FALSE;
					token = sp;
				} else if (intoken(c))
					tp++;
			} else if (begtoken(c)) {
				token = tp = sp;
				midtoken = TRUE;
			}
		}
/*		if (c == ';'  &&  tydef == end) {  /* clean with typedefs */
		if (c == ';' && tydef != middle) { /* clean with typedefs */
			tydef = none;
			strdef = FALSE;
		}
		sp++;
		if (c == '\n' || sp > &line[sizeof (line) - BUFSIZ]) {
			tp = token = sp = line;
			lineftell = ftell(inf);
			number = gotone = midtoken = inquote = inchar = FALSE;
		}
	}
}

/*
 * This routine  checks to see if the current token is
 * at the start of a function, or corresponds to a typedef
 * It updates the input line * so that the '(' will be
 * in it when it returns.
 */
/* ARGSUSED */
static int
start_entry(lp, token, tp, f)
char	**lp, *token, *tp;
int	*f;
{
	reg	char	c, *sp;
	static	bool	found;
	bool	firsttok;		/* T if have seen first token in ()'s */
	int	bad;

#ifdef	DEBUG
	(void) fprintf(stderr, "T '%s' %d %d lineno %d\n",
		token, strlen(token), tp - token, lineno);
#endif
	*f = 1;			/* a function */
	sp = *lp;
	c = *sp;
	bad = FALSE;
	if (!number) {		/* space is not allowed in macro defs	*/
		while (iswhite(c)) {
			*++sp = c = getc(inf);
			if (c == '\n') {
				lineno++;
				goto badone;	/*JS*/
				/* no real definition on this line */
/*JS				if (sp > &line[sizeof (line) - BUFSIZ])*/
/*JS					goto ret;*/
			}
		}
	/* the following tries to make it so that a #define a b(c)	*/
	/* doesn't count as a define of b.				*/
	} else {
		if (!strncmp(token, "define", 6))
			found = 0;
		else
			found++;
		if (found >= 2) {
			gotone = TRUE;
badone:			bad = TRUE;
			goto ret;
		}
	}
	/* check for the typedef cases		*/
#ifdef	DEBUG
	{	static int calls;

		(void) fprintf(stderr,
		"calls: %d lineno %d token '%s' state: %d %s *sp '%c'\n",
		calls++, lineno, token, tydef, tdnm[tydef], *sp);
	}
#endif
	if (tydef == none && !strncmp(token, "extern", 6)) {
		tydef = ext;
		if (*sp == '"')			/* extern "C" */
			goto badone;
	}
	if (tflag && !strncmp(token, "typedef", 7)) {
		tydef = begin;
		goto badone;
	}
	if (tflag && (tydef == none || tydef == ext) &&
	    (!strncmp(token, "struct", 6) ||				 /*JS*/
	    !strncmp(token, "union", 5) || !strncmp(token, "enum", 4))) {/*JS*/
		tydef = begin3;
		strdef = TRUE;
		goto badone;
	}
	if (tydef == begin && (!strncmp(token, "struct", 6) ||
	    !strncmp(token, "union", 5) || !strncmp(token, "enum", 4))) {
		tydef = begin2;	/* JS */
		goto badone;
	}
	if (tydef == begin) {
		tydef = end;
		goto badone;
	}
	if (tydef == begin2) {
		tydef = begin3;
		if (c != '{')			/* typedef struct s1{ ... } */
			goto badone;
	}
	if (tydef == begin3) {
		if (strdef && c != '{') {	/* struct s1 *f(); */
			tydef = none;
			strdef = FALSE;
			goto badone;
		}
		*f = 0;
		goto ret;
	}
	if (tydef == end) {
		if (strdef)
			goto badone;
		*f = 0;
		goto ret;
	}
	if (c != '(')
		goto badone;
	firsttok = FALSE;
	while ((*++sp = c = getc(inf)) != ')') {
		if (c == '\n') {
			lineno++;
			if (sp > &line[sizeof (line) - BUFSIZ])
				goto ret;
		}
		/*
		 * This line used to confuse ctags:
		 *	int	(*oldhup)();
		 * This fixes it. A nonwhite char before the first
		 * token, other than a / (in case of a comment in there)
		 * makes this not a declaration.
		 */
		if (begtoken(c) || c == '/')
			firsttok++;
		else if (!iswhite(c) && !firsttok)
			goto badone;
	}
	while (iswhite(*++sp = c = getc(inf)))
		if (c == '\n') {
			lineno++;
			if (sp > &line[sizeof (line) - BUFSIZ])
				break;
		}
ret:
	*lp = --sp;
	if (c == '\n')
		lineno--;
	(void) ungetc(c, inf);
#ifdef	DEBUG
	(void) fprintf(stderr, "LINE %d\n", lineno);
#endif
	return (!bad && (!*f || isgood(c)));
					/* hack for typedefs */
}

/*
 * Y_entries:
 *	Find the yacc tags and put them in.
 */
static void
Y_entries()
{
	register char	*sp, *orig_sp;
	register int	brace;
	register bool	in_rule, toklen;
	char		tok[BUFSIZ];

	brace = 0;
	in_rule = FALSE;
	getaline();
	pfnote("yyparse", lineno, TRUE);
	while (fgets(line, sizeof (line), inf) != NULL)
		for (sp = line; *sp; sp++)
			switch (*sp) {
			case '\n':
				lineno++;
				/* FALLTHROUGH */
			case ' ':
			case '\t':
			case '\f':
			case '\r':
				break;
			case '"':
				do {
					while (*++sp != '"')
						continue;
				} while (sp[-1] == '\\');
				break;
			case '\'':
				do {
					while (*++sp != '\'')
						continue;
				} while (sp[-1] == '\\');
				break;
			case '/':
				if (*++sp == '*')
					sp = toss_comment(sp);
				else
					--sp;
				break;
			case '{':
				brace++;
				break;
			case '}':
				brace--;
				break;
			case '%':
				if (sp[1] == '%' && sp == line)
					return;
				break;
			case '|':
			case ';':
				in_rule = FALSE;
				break;
			default:
				if (brace == 0 && !in_rule && (isalpha(*sp) ||
								*sp == '.' ||
								*sp == '_')) {
					orig_sp = sp;
					++sp;
					while (isalnum(*sp) || *sp == '_' ||
						*sp == '.')
						sp++;
					toklen = sp - orig_sp;
					while (isspace(*sp))
						sp++;
					if (*sp == ':' || (*sp == '\0' &&
					    first_char() == ':')) {
						(void) strncpy(tok, orig_sp,
								toklen);
						tok[toklen] = '\0';
						(void) strcpy(lbuf, line);
						lbuf[strlen(lbuf) - 1] = '\0';
						pfnote(tok, lineno, TRUE);
						in_rule = TRUE;
					} else
						sp--;
				}
				break;
			}
}

static char *
toss_comment(start)
char	*start;
{
	register char	*sp;

	/*
	 * first, see if the end-of-comment is on the same line
	 */
	do {
		while ((sp = index(start, '*')) != NULL)
			if (sp[1] == '/')
				return (++sp);
			else
				start = ++sp;
		start = line;
		lineno++;
	} while (fgets(line, sizeof (line), inf) != NULL);

	return ("");
}

static void
getaline()
{
	long saveftell = ftell(inf);
	register char *cp;

	(void) fseek(inf, lineftell, 0);
	(void) fgets(lbuf, sizeof (lbuf), inf);
	cp = rindex(lbuf, '\n');
	if (cp)
		*cp = 0;
	(void) fseek(inf, saveftell, 0);
}

static void
free_tree(node)
NODE	*node;
{

	while (node) {
		free_tree(node->right);
		free(node);
		node = node->left;
	}
}

static void
add_node(node, cur_node)
	NODE *node, *cur_node;
{
	register int dif;

	dif = strcmp(node->entry, cur_node->entry);
	if (dif == 0) {
		if (node->file == cur_node->file) {
			if (!wflag) {
(void) fprintf(stderr, "Duplicate entry in file %s, line %d: %s\n",
    node->file, lineno, node->entry);
(void) fprintf(stderr, "Second entry ignored\n");
			}
			return;
		}
		if (!cur_node->been_warned)
			if (!wflag)
(void) fprintf(stderr,
	"Duplicate entry in files %s and %s: %s (Warning only)\n",
	node->file, cur_node->file, node->entry);
		cur_node->been_warned = TRUE;
		return;
	}

	if (dif < 0) {
		if (cur_node->left != NULL)
			add_node(node, cur_node->left);
		else
			cur_node->left = node;
		return;
	}
	if (cur_node->right != NULL)
		add_node(node, cur_node->right);
	else
		cur_node->right = node;
}

static void
put_entries(node)
reg NODE	*node;
{
	reg char	*sp;

	if (node == NULL)
		return;
	put_entries(node->left);
	if (xflag == 0) {
		if (node->f) {		/* a function */
			(void) fprintf(outf, "%s\t%s\t%c^",
				node->entry, node->file, searchar);
			for (sp = node->pat; *sp; sp++)
				if (*sp == '\\')
					(void) fprintf(outf, "\\\\");
				else if (*sp == searchar)
					(void) fprintf(outf, "\\%c", searchar);
				else
					(void) putc(*sp, outf);
			(void) fprintf(outf, "%c\n", searchar);
		} else {		/* a typedef; text pattern inadequate */
			(void) fprintf(outf, "%s\t%s\t%d\n",
				node->entry, node->file, node->lno);
		}
	} else if (vflag)
		(void) fprintf(stdout, "%s %s %d\n",
				node->entry, node->file, (node->lno+63)/64);
	else
		(void) fprintf(stdout, "%-16s%4d %-16s %s\n",
			node->entry, node->lno, node->file, node->pat);
	put_entries(node->right);
}
static char	*dbp = lbuf;
static int	pfcnt;

static int
PF_funcs(fi)
	FILE *fi;
{

	pfcnt = 0;
	while (fgets(lbuf, sizeof (lbuf), fi)) {
		lineno++;
		dbp = lbuf;
		if (*dbp == '%') dbp++;		/* Ratfor escape to fortran */
		while (isspace(*dbp))
			dbp++;
		if (*dbp == 0)
			continue;
		switch (*dbp |' ') {

		case 'i':
			if (tail("integer"))
				takeprec();
			break;
		case 'r':
			if (tail("real"))
				takeprec();
			break;
		case 'l':
			if (tail("logical"))
				takeprec();
			break;
		case 'c':
			if (tail("complex") || tail("character"))
				takeprec();
			break;
		case 'd':
			if (tail("double")) {
				while (isspace(*dbp))
					dbp++;
				if (*dbp == 0)
					continue;
				if (tail("precision"))
					break;
				continue;
			}
			break;
		}
		while (isspace(*dbp))
			dbp++;
		if (*dbp == 0)
			continue;
		switch (*dbp|' ') {

		case 'f':
			if (tail("function"))
				getit();
			continue;
		case 's':
			if (tail("subroutine"))
				getit();
			continue;
		case 'p':
			if (tail("program")) {
				getit();
				continue;
			}
			if (tail("procedure"))
				getit();
			continue;
		}
	}
	return (pfcnt);
}

static int
tail(cp)
	char *cp;
{
	register int len = 0;

	while (*cp && (*cp&~' ') == ((*(dbp+len))&~' '))
		cp++, len++;
	if (*cp == 0) {
		dbp += len;
		return (1);
	}
	return (0);
}

static void
takeprec()
{

	while (isspace(*dbp))
		dbp++;
	if (*dbp != '*')
		return;
	dbp++;
	while (isspace(*dbp))
		dbp++;
	if (!isdigit(*dbp)) {
		--dbp;		/* force failure */
		return;
	}
	do
		dbp++;
	while (isdigit(*dbp));
}

static void
getit()
{
	register char *cp;
	char c;
	char nambuf[BUFSIZ];

	for (cp = lbuf; *cp; cp++)
		;
	*--cp = 0;	/* zap newline */
	while (isspace(*dbp))
		dbp++;
	if (*dbp == 0 || !isalpha(*dbp) || !isascii(*dbp))
		return;
	for (cp = dbp+1; *cp && (isalpha(*cp) || isdigit(*cp)); cp++)
		continue;
	c = cp[0];
	cp[0] = 0;
	(void) strcpy(nambuf, dbp);
	cp[0] = c;
	pfnote(nambuf, lineno, TRUE);
	pfcnt++;
}

static char *
savestr(cp)
	char *cp;
{
	register int len;
	register char *dp;

	len = strlen(cp);
	dp = (char *)malloc(len+1);
	(void) strcpy(dp, cp);
	return (dp);
}

/*
 * Return the ptr in sp at which the character c last
 * appears; NULL if not found
 *
 * Identical to v7 rindex, included for portability.
 */
#ifndef	__STDC__
char *
rindex(sp, c)
register char *sp;
int c;
{
	register char *r;

	r = NULL;
	do {
		if (*sp == c)
			r = sp;
	} while (*sp++);
	return (r);
}

char *
index(sp, c)
register char *sp;
int c;
{
	do {
		if (*sp == c)
			return (sp);
	} while (*sp++);
	return (NULL);
}
#endif

/*
 * lisp tag functions
 * just look for (def or (DEF
 */
static void
L_funcs(fi)
FILE *fi;
{
	register int	special;

	pfcnt = 0;
	while (fgets(lbuf, sizeof (lbuf), fi)) {
		lineno++;
		dbp = lbuf;
		if (dbp[0] == '(' &&
		    (dbp[1] == 'D' || dbp[1] == 'd') &&
		    (dbp[2] == 'E' || dbp[2] == 'e') &&
		    (dbp[3] == 'F' || dbp[3] == 'f')) {
			dbp += 4;
			if (striccmp(dbp, "method") == 0 ||
			    striccmp(dbp, "wrapper") == 0 ||
			    striccmp(dbp, "whopper") == 0)
				special = TRUE;
			else
				special = FALSE;
			while (!isspace(*dbp))
				dbp++;
			while (isspace(*dbp))
				dbp++;
			L_getit(special);
		}
	}
}

static void
L_getit(special)
int	special;
{
	register char	*cp;
	register char	c;
	char		nambuf[BUFSIZ];

	for (cp = lbuf; *cp; cp++)
		continue;
	*--cp = 0;		/* zap newline */
	if (*dbp == 0)
		return;
	if (special) {
		if ((cp = index(dbp, ')')) == NULL)
			return;
		while (cp >= dbp && *cp != ':')
			cp--;
		if (cp < dbp)
			return;
		dbp = cp;
		while (*cp && *cp != ')' && *cp != ' ')
			cp++;
	} else
		for (cp = dbp + 1; *cp && *cp != '(' && *cp != ' '; cp++)
			continue;
	c = cp[0];
	cp[0] = 0;
	(void) strcpy(nambuf, dbp);
	cp[0] = c;
	pfnote(nambuf, lineno, TRUE);
	pfcnt++;
}

/*
 * striccmp:
 *	Compare two strings over the length of the second, ignoring
 *	case distinctions.  If they are the same, return 0.  If they
 *	are different, return the difference of the first two different
 *	characters.  It is assumed that the pattern (second string) is
 *	completely lower case.
 */
static int
striccmp(str, pat)
register char	*str, *pat;
{
	register int	c1;

	while (*pat) {
		if (isupper(*str))
			c1 = tolower(*str);
		else
			c1 = *str;
		if (c1 != *pat)
			return (c1 - *pat);
		pat++;
		str++;
	}
	return (0);
}

/*
 * first_char:
 *	Return the first non-blank character in the file.  After
 *	finding it, rewind the input file so we start at the beginning
 *	again.
 */
static int
first_char()
{
	register int	c;
	register long	off;

	off = ftell(inf);
	while ((c = getc(inf)) != EOF)
		if (!isspace(c) && c != '\r') {
			(void) fseek(inf, off, 0);
			return (c);
		}
	(void) fseek(inf, off, 0);
	return (EOF);
}

/*
 * toss_yysec:
 *	Toss away code until the next "%%" line.
 */
static void
toss_yysec()
{
	char		buf[BUFSIZ];

	for (;;) {
		lineftell = ftell(inf);
		if (fgets(buf, BUFSIZ, inf) == NULL)
			return;
		lineno++;
		if (strncmp(buf, "%%", 2) == 0)
			return;
	}
}
