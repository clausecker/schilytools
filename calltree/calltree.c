/* @(#)calltree.c	1.50 21/08/20 Copyright 1985, 1999-2021 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)calltree.c	1.50 21/08/20 Copyright 1985, 1999-2021 J. Schilling";
#endif
/*
 *	A program to produce a static calltree for C-functions
 *
 *	Copyright (c) 1985, 1999-2021 J. Schilling
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

#include <schily/stdio.h>		/* Includes popen()/pclose()	*/
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#define	GT_COMERR		/* #define comerr gtcomerr */
#define	GT_ERROR		/* #define error gterror   */
#include <schily/schily.h>
#define	VMS_VFORK_OK
#include <schily/vfork.h>
#include <schily/nlsdefs.h>
#include "strsubs.h"
#include "sym.h"
#include "clex.h"
#include "version.h"

#ifdef	HAVE_FORK
#	include <schily/wait.h>
#	define	Pwait(f)	{ wait(0); fclose(f); }
#else
#	define	Pwait(f)	pclose(f)
#endif

LOCAL	char	ct_version[] = VERSION;

BOOL	bflag	= FALSE;		/* -b: print vertical bar for indent */
BOOL	rflag	= FALSE;		/* -r: inverted output print callers */
BOOL	fflag	= FALSE;		/* -f: flat output (summary per func)*/
BOOL	gflag	= FALSE;		/* -g: print filename of definition  */
BOOL	mflag	= FALSE;		/* -m: list 'main' only (like l=main)*/
BOOL	npflag	= FALSE;		/* -np: don't call preprocessor	    */
BOOL	pflag	= TRUE;			/* -p: call preprocessor (default)   */
BOOL	uflag	= FALSE;		/* -u: print funcs unused from 'main'*/
BOOL	eflag	= FALSE;		/* -e: print funcs unused completely */
BOOL	xflag	= FALSE;		/* -x: detect external functions    */
BOOL	vflag	= FALSE;		/* -v: be verbose		    */
BOOL	debug	= FALSE;		/* -debug: print debugging messages */
BOOL	xvcg	= FALSE;		/* -xvcg: format output for xvcg    */
BOOL	dot	= FALSE;		/* -dot: format output for graphviz */

#define	DEFDEPTH	32000

int	depth	= DEFDEPTH;		/* Stop at this call nesting depth  */
int	indents	= 4;			/* default indentation per nest depth*/
char	indent[256];			/* This is the indent string	    */

int	nestlevel;			/* for {} nesting, used by parser   */
sym_t	*funcs;				/* global function table	    */
sym_t	*fnames;			/* global filename table	    */
sym_t	*listfuncs;			/* table of functions to list	    */
sym_t	*ignorefuncs;			/* table of functions to ignore	    */
char	*curfname;

#define	MAXARGS		32

#ifdef	_MSC_VER
int	Argc = 2;
char	*Argv [MAXARGS] = { "cl.exe", "-E" };
#else
#ifndef	HAVE_FORK
int	Argc = 2;
char	*Argv [MAXARGS] = { "cc", "-E" };
#else
int	Argc = 2;
char	*Argv [MAXARGS] = { "cpp", "-I/usr/include" };
#endif	/* HAVE_FORK */
#endif	/* _MSC_VER */
#ifdef	__EMX__
char	*Env[2] = { "PATH=/lib;/usr/ccs/lib;/usr/bin;/bin", 0 };
#else
char	*Env[2] = { "PATH=/lib:/usr/ccs/lib:/usr/bin:/bin", 0 };
#endif

LOCAL	void	usage		__PR((int exitcode));
EXPORT	int	main		__PR((int ac, char **av));
LOCAL	FILE	*mkcpppipe	__PR((FILE *f, char *fname));
LOCAL	void	printfuncs	__PR((sym_t *table));
LOCAL	void	printafunc	__PR((sym_t *sym));
LOCAL	void	printusage	__PR((int indt, sym_t *tab));
LOCAL	void	printflatusage	__PR((sym_t *tab));
LOCAL	void	printxvcgusage	__PR((sym_t *caller, sym_t *tab));
LOCAL	void	printdotusage	__PR((sym_t *caller, sym_t *tab));
LOCAL	void	cleartree	__PR((sym_t *sym));
LOCAL	void	flattree	__PR((sym_t *tab));
LOCAL	BOOL	findclose	__PR((FILE *fp));
LOCAL	void	findfname	__PR((FILE *fp, char *fname));
LOCAL	int	checkfunc	__PR((FILE *fp, int *ftype));
LOCAL	int	findfunc	__PR((FILE *fp, char *name, char *fname));
LOCAL	void	nesterror	__PR((void));
LOCAL	void	parsefile	__PR((FILE *fp, char *filename));
LOCAL	int	readfuncs	__PR((char *filename, sym_t **tab));
LOCAL	int	got_cpp_arg	__PR((char *name, char *type));

LOCAL void
usage(exitcode)
	int	exitcode;
{
	error("Usage:	calltree [calltree_options] [cpp_options] file1..filen\n");
	error("Options:\n");
	error("\t-b\t\tPrint a vertial Bar at each tab stop.\n");
	error("\t-r\t\tInvert the structure of the tree.\n");
	error("\t-f\t\tFlattened (cumulative) tree.\n");
	error("\t-g\t\tPrint file names past procedure names.\n");
	error("\t-m\t\tCall structure for main only.\n");
	error("\t-p\t\tUse C Preprocessor (default).\n");
	error("\t-np\t\tDon't use C Preprocessor.\n");
	error("\t-u\t\tList all functions not called via 'main'.\n");
	error("\t-e\t\tList all functions not called.\n");
	error("\t-x\t\tList all external functions.\n");
	error("\t-xvcg\t\tFormat output for xvcg.\n");
	error("\t-dot\t\tFormat output for graphviz.\n");
	error("\t-v\t\tBe verbose.\n");
	error("\t-help\t\tPrint this help.\n");
	error("\t-version\tPrint version number.\n");
	error("\tigorefile=file\tDon't list functions found in 'file'.\n");
	error("\tlistfile=file\tList only functions found in 'file'.\n");
	error("\tlist=name\tProduce call graph only for function 'name'.\n");
	error("\tdepth=#\t\tSet the maximum printed nesting depth to #.\n");
	error("\ts=#\t\tSet indentation to #.\n");
	error("ignorefile=, listfile= and depth= may be abbreviated by first letter.\n");
	error("list= may be abbreviated by lf=.\n");
	exit(exitcode);
}

LOCAL	char	*opts = "b,r,f,g,m,p,np,u,e,x,dot,xvcg,v,ignorefile&,i&,listfile&,l&,list*,lf*,depth#,d#,s#,help,version,debug+,db+,I&,D&,U&";

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	FILE	*f;
	int	i;
	int	help = 0;
	int	version = 0;
	char	*thisname = 0;
	int	cac;
	char *const * cav;

	save_args(ac, av);

	(void) setlocale(LC_ALL, "");

#ifdef  USE_NLS
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "calltree"	/* Use this only if it weren't */
#endif
	{ char	*dir;
	dir = searchfileinpath("share/locale", F_OK,
					SIP_ANY_FILE|SIP_NO_PATH, NULL);
	if (dir)
		(void) bindtextdomain(TEXT_DOMAIN, dir);
	else
#if defined(PROTOTYPES) && defined(INS_BASE)
	(void) bindtextdomain(TEXT_DOMAIN, INS_BASE "/share/locale");
#else
	(void) bindtextdomain(TEXT_DOMAIN, "/usr/share/locale");
#endif
	(void) textdomain(TEXT_DOMAIN);
	}
#endif 	/* USE_NLS */

	cac = --ac;
	cav = ++av;

	/*
	 * the argument order it's important and it must be the same of opts
	 */
	if (getallargs(&cac, &cav, opts, &bflag, &rflag, &fflag, &gflag,
			&mflag, &pflag, &npflag, &uflag, &eflag,
			&xflag,
			&dot,
			&xvcg,
			&vflag,
			readfuncs, &ignorefuncs,
			readfuncs, &ignorefuncs,
			readfuncs, &listfuncs,
			readfuncs, &listfuncs,
			&thisname, &thisname,
			&depth, &depth,
			&indents, &help, &version,
			&debug, &debug,
			got_cpp_arg, "-I",
			got_cpp_arg, "-D",
			got_cpp_arg, "-U") < 0) {
		errmsgno(EX_BAD, "Bad Option: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (version) {
		gtprintf("Calltree release %s %s (%s-%s-%s) Copyright (C) 1985, 88-90, 95-99, 2000-2021 %s\n",
				ct_version, VERSION_DATE,
				HOST_CPU, HOST_VENDOR, HOST_OS,
				_("Jörg Schilling"));
		exit(0);
	}

	if (npflag)
		pflag = FALSE;
	if (depth <= 0)
		depth = DEFDEPTH;
	if (uflag)
		mflag = fflag = TRUE;
	if (eflag)
		uflag = rflag = TRUE;
	if (xflag) {
		eflag = TRUE;  rflag = FALSE;
	}
	if (thisname)
		mflag = TRUE;
	if (thisname == 0)
		thisname = "main";

	if (indents > sizeof (indent)-1) {
		indents = sizeof (indent)-1;
		errmsgno(EX_BAD, "Cannot indent more than %d spaces.\n", indents);
	}
	for (i = 0; i < indents; i++)
		indent[i] = ' ';
	if (bflag)
		indent[0] = '|';
	indent[indents] = '\0';

	initkeyw();

	cac = ac;
	cav = av;
	if (getfiles(&cac, &cav, opts) <= 0) {
		errmsgno(EX_BAD, "No input files.\n");
		usage(EX_BAD);
	}

	if (vflag)
		error("Generating names ...\n");

	for (; getfiles(&cac, &cav, opts); cac--, cav++) {
		if ((f = fileopen(cav[0], "r")) == (FILE *) NULL)
			comerr("Cannot open '%s'.\n", cav[0]);

		if (pflag)
			f = mkcpppipe(f, cav[0]);

		parsefile(f, cav[0]);

		if (pflag) {
			Pwait(f);
		} else {
			fclose(f);
		}
	}

	if (vflag)
		error("Generating tree ...\n");

	if (xvcg) {
		/*
		 * Print header for xvcg.
		 */
		mflag = FALSE;
		printf("graph: {\norientation: left_to_right\n");
	}
	if (dot) {
		/*
		 * Print header for graphviz.
		 */
		mflag = FALSE;
		printf("digraph function_map {\nrankdir=LR;\nratio=fill;\nnode [style=filled]\n");
	}
	if (mflag) {
		/*
		 * Either -m or -r name, do one function only.
		 */
		sym_t	*this;

		if ((this = lookup(thisname, &funcs, L_LOOK)) == NULL) {
			comerrno(EX_BAD, "Function '%s' not found.\n", thisname);
		} else {
			cleartree(funcs);
			if (uflag)
				this->s_flags |= S_USED;
			printafunc(this);
		}
	} else {
		/*
		 * Print all functions.
		 */
		printfuncs(funcs);
	}
	if (xvcg) {
		/*
		 * Print trailer for xvcg.
		 */
		printf("}\n");
	}
	if (dot) {
		/*
		 * Print trailer for graphviz.
		 */
		printf("}\n");
	}
	exit(0);
	/* NOTREACHED */
	return (0);	/* Keep lint happy */
}

LOCAL FILE *
mkcpppipe(f, fname)
	FILE	*f;
	char	*fname;
{
#ifdef	HAVE_FORK
	FILE	*fpp[2];
	int	i;

	if (debug) {
		error("CPP args: ");
		for (i = 0; i < Argc; i++)
			error("'%s' ", Argv[i]);
		error("\n");
	}
	if (fpipe(fpp) == 0)
		comerr("Can not make pipe to C-preprocessor.\n");

	if ((i = vfork()) == 0) {	/* child */
#ifdef	F_SETFD
		fcntl(fileno(pfd[0]), F_SETFD, FD_CLOEXEC);
#endif
		fexecve(Argv[0], f, fpp[1], stderr,
			Argv, Env);
		errmsg("Cannot execute '%s'.\n", Argv[0]);
#ifdef	HAVE_VFORK
		_exit(geterrno());
#else
		exit(geterrno());
#endif
	}
	if (i < 0)
		comerr("Fork failed.\n");

	/*
	 * parent
	 */
	fclose(f);			/* don't need it here*/
	fclose(fpp[1]);
	return (fpp[0]);
#else					/* No fork(), try popen() */
	FILE	*fp;
	int	i;
	char	cmd[8192];
	char	*p;

	if (debug) {
		error("CPP args: ");
		for (i = 0; i < Argc; i++)
			error("'%s' ", Argv[i]);
		error("\n");
	}
	cmd[0] = '\0';
	for (i = 0; i < Argc; i++) {
		p = cmd + strlen(cmd);
		js_snprintf(p, sizeof (cmd) + cmd - p,
			"%s ", Argv[i]);
	}	
	p = cmd + strlen(cmd);
	js_snprintf(p, sizeof (cmd) + cmd - p,
		"%s ", fname);
	fp = popen(cmd, "r");
	fclose(f);
	return (fp);
#endif
}

/*
 * Print the known information for all functions
 */
LOCAL void
printfuncs(tab)
	sym_t	*tab;
{
	if (tab) {
		printfuncs(tab->s_left);
		cleartree(funcs);
		printafunc(tab);
		printfuncs(tab->s_right);
	}
}

/*
 * Print the known information for one function
 */
LOCAL void
printafunc(sym)
	sym_t	*sym;
{
	if (listfuncs && !lookup(sym->s_name, &listfuncs, L_LOOK))
		return;

	if (xvcg) {
		if (!eflag && !sym->s_uses)
			return;
		printf("node: { title: \"%s\" ", sym->s_name);
		if (gflag && sym->s_filename)
			printf("label: \"%s [%s:%d]\" ",
				sym->s_name, sym->s_filename, sym->s_lineno);
		if ((sym->s_flags & S_DEF) == 0)
			printf("shape: ellipse textcolor: lightgrey bordercolor: lightgrey ");
		printf("}\n");
		printxvcgusage(sym, sym->s_uses);
	} else if (dot) {
		if (!eflag && !sym->s_uses)
			return;
		printf(" %s [ shape=box ", sym->s_name);
		if (gflag && sym->s_filename)
			printf("label = \"%s [%s:%d]\" ",
				sym->s_name, sym->s_filename, sym->s_lineno);
		if ((sym->s_flags & S_DEF) == 0)
			printf("shape = ellipse, fontcolor = lightgrey ");
		printf("];\n");
		printdotusage(sym, sym->s_uses);
	} else if ((sym->s_flags & S_DEF) == 0) {
		printf("%s", sym->s_name);
		if (gflag && sym->s_filename) {
			printf(" [%s:%d]",
				sym->s_filename, sym->s_lineno);
		}
		if (eflag) {
			printf("\n");
		} else if (rflag) {
			printf(":\n%sNOT CALLED\n", indent);
		} else {
			printf(":\n%sEXTERNAL ROUTINE\n", indent);
		}
	} else if (!eflag) {
		printf("%s", sym->s_name);
		if (gflag && sym->s_filename) {
			printf(" [%s:%d]:\n",
					sym->s_filename,
					sym->s_lineno);
		} else {
			printf(":\n");
		}
		if (fflag) {
			flattree(sym->s_uses);
			printflatusage(funcs);
		} else {
			sym->s_flags |= S_RECURSE;
			printusage(1, sym->s_uses);
			sym->s_flags &= ~S_RECURSE;
		}
	}
}

/*
 * Print the known caller/callee information for one function.
 */
LOCAL void
printusage(indt, tab)
	int	indt;
	sym_t	*tab;
{
	int	i;
	sym_t	*sym;
	BOOL	isrecurse = FALSE;

	if (tab == NULL)
		return;

	printusage(indt, tab->s_left);
	for (i = 0; i < indt; i++)
		printf("%s", indent);

	if (tab->s_sym == NULL) {	/* Darf eigentlich nicht vorkommen */
		printf("FAKED CALL\n");
		return;
	}
	sym = tab->s_sym;

	printf("%s", sym->s_name);
	if (gflag && sym->s_filename)
		printf(" [%s:%d]", sym->s_filename, sym->s_lineno);
	if ((sym->s_flags & S_RECURSE) != 0) {
		isrecurse = TRUE;
		printf(" ....\n");
	} else
		printf("\n");
	if ((--depth > 0) && sym != 0 && !isrecurse && !rflag) {
		sym->s_flags |= S_RECURSE;
		printusage(indt+1, sym->s_uses);
		sym->s_flags &= ~S_RECURSE;
	}
	depth++;

	printusage(indt, tab->s_right);
}

/*
 * Print the known caller/callee information for one function in flat form.
 */
LOCAL void
printflatusage(tab)
	sym_t	*tab;
{
	if (tab) {
		printflatusage(tab->s_left);
		if (((tab->s_flags & S_USED) != 0 && !uflag) ||
		    ((tab->s_flags & S_USED) == 0 && uflag)) {
			printf("%s%s", indent, tab->s_name);
			if (gflag && tab->s_filename) {
				printf(" [%s:%d]\n",
					tab->s_filename, tab->s_lineno);
			} else {
				printf("\n");
			}
		}
		printflatusage(tab->s_right);
	}
}

/*
 * Print the known caller/callee information formatted for xvcg.
 */
LOCAL void
printxvcgusage(caller, tab)
	sym_t	*caller;
	sym_t	*tab;
{
	if (tab) {
		sym_t	*called;

		printxvcgusage(caller, tab->s_left);
		if (eflag ||
		    ((called = lookup(tab->s_name, &funcs, L_LOOK)) != NULL &&
		    called->s_uses))
			printf("edge: { sourcename: \"%s\" targetname: \"%s\" }\n",
					caller->s_name, tab->s_name);
		printxvcgusage(caller, tab->s_right);
	}
}

LOCAL void
printdotusage(caller, tab)
	sym_t	*caller;
	sym_t	*tab;
{
	if (tab) {
		sym_t	*called;

		printdotusage(caller, tab->s_left);
		if (eflag ||
		    ((called = lookup(tab->s_name, &funcs, L_LOOK)) != NULL &&
		    called->s_uses))
			printf("%s  -> %s;\n",
				caller->s_name, tab->s_name);
		printdotusage(caller, tab->s_right);
	}
}
/*
 * Clear all markers in the symbol tree
 */
LOCAL void
cleartree(sym)
	sym_t	*sym;
{
	if (sym) {
		cleartree(sym->s_left);
		sym->s_flags &= ~(S_USED|S_RECURSE);
		cleartree(sym->s_right);
	}
}

/*
 * Prepare a tree for flat (cumulative) printing.
 */
LOCAL void
flattree(tab)
	sym_t	*tab;
{
	sym_t	*sym;

	if (tab) {
		flattree(tab->s_left);
		if (((sym = tab->s_sym)->s_flags & S_USED) == 0) {
			sym->s_flags |= S_USED;
			flattree(sym->s_uses);
		}
		flattree(tab->s_right);
	}
}

/*
 * Find a matching close bracket ')'.
 */
LOCAL BOOL
findclose(fp)
	FILE	*fp;
{
	register int	n = 1;
	register int	tktype;

	while ((tktype = clex(fp)) != T_EOF) {
		switch (tktype) {

		case T_LCURLY:
		case T_RCURLY:
			return (FALSE);

		case T_OPEN:
			n++; break;

		case T_CLOSE:
			n--; break;
		}
		if (n == 0)
			return (TRUE);
	}
	return (FALSE);
}

/*
 * Parse a lineno/filename directive from the C-preprocessor.
 */
LOCAL void
findfname(fp, fname)
	FILE	*fp;
	char	*fname;
{
	register int	tktype;
		char	*p;
		int	line;

	if ((tktype = clex(fp)) == T_EOF)
		return;

	switch (tktype) {

	case T_ALPHA:
		if (strcmp((char *)lexbuf, "line") != 0)
			return;
		if ((tktype = clex(fp)) == T_EOF)
			return;
		if (tktype != T_NUMBER)
			return;
		/* FALLTHROUGH */
	case T_NUMBER:
		astoi((char *)lexbuf, &line);
		lexline = line-1;
		if ((tktype = clex(fp)) == T_EOF)
			return;
		if (tktype == T_STRING) {
			p = (char *)lexbuf;
			p++;
			p[strlen(p)-1] = '\0';
			strcpy(fname, p);

			/*
			 * The new GNU cpp v3.2 prints <stdin> for filename.
			 * For example the hash line in GNU cpp v2.96 (worked):
			 *
			 *  # 40 ""
			 *
			 * With GNU cpp v3.2 (fails without the fix below):
			 *
			 *  # 40 "<stdin>"
			 *
			 * The "<stdin>" string should be treated as a blank
			 * string instead of a filename.
			 */
			if (strcmp(fname, "<stdin>") == 0)
				fname[0] = '\0';

			if (fname[0] != '\0')
				lexfile = fname;
			else
				lexfile = curfname;
		}
	}
}

#define	FUNC_DEF	1
#define	FUNC_CALL	0

/*
 * Check for function call/definition.
 */
LOCAL int
checkfunc(fp, ftypep)
	FILE	*fp;
	int	*ftypep;
{
	register int	tktype = T_NONE;

	/*
	 * We come here if we found a T_ALPHA followed by T_OPEN
	 * e.g. 'testfunc ('
	 */
	if (nestlevel == 0) {				/* not in function */

		if (!findclose(fp))
			return (-1);			/* maybe var def */
		tktype = clex(fp);

		if (tktype != T_SEMI &&
		    tktype != T_COMMA) {		/* no external def */

			if (tktype == T_LCURLY) {
				nestlevel++;
			} else if (tktype == T_RCURLY &&
				    --nestlevel < 0) {
				/*
				 * Bad syntax or rotten parser.
				 */
				nesterror();
			}
			*ftypep = FUNC_DEF;		/* func definition */
			return (tktype);
		}
		*ftypep = -1;				/* maybe a var def */
		return (tktype);

	} else {					/* in function */

#ifdef	nonono /* Hier wird Func call im Argument nicht erkannt */
		if (!findclose(fp))
			return (-1);			/* maybe var usage */
		if ((tktype = clex(fp)) == T_OPEN) {
			error("No func: '%s' ('%s') on line %d\n", name, lexbuf, lexline);
			return (-1);
		}
#endif
		*ftypep = FUNC_CALL;			/* func call */
		return (tktype);
	}
}

/*
 * Find functions in C-syntax.
 */
LOCAL int
findfunc(fp, name, fname)
	FILE	*fp;
	char	*name;
	char	*fname;
{
	register int	tktype;
		int	ftype = 0;		/* init to make GCC quiet */

	while ((tktype = clex(fp)) != T_EOF) {
recheck:
		if (debug) {
			error("%s:%d{%d} %s: %s\n",
				lexfile, lexline, nestlevel, lextnames[tktype], lexbuf);
		}
		switch (tktype) {

		case T_LCURLY:
			nestlevel++;
			break;

		case T_RCURLY:
			if (--nestlevel < 0)
				nesterror();
			break;

		case T_HASH:
			findfname(fp, fname);
			break;

		case T_ALPHA:
			strcpy(name, (char *)lexbuf);
			if ((tktype = clex(fp)) == T_OPEN) {
				tktype = checkfunc(fp, &ftype);
				if (ftype < 0)
					goto recheck;
				return (ftype);
			}
			goto recheck;

		default:
			break;
		}
	}
	return (-1);					/* EOF */
}

/*
 * Warn about a {} nesting error and reset nestlevel.
 */
LOCAL void
nesterror()
{
	error("Found '}' without open on line %d.\n", lexline);
	if (debug)
		error("lexbuf from line %d: %s\n", lexline, lexbuf);
	nestlevel = 0;
}

/*
 * Zuerst wird eine Funktionsdefinition gefunden und deren Wert in 'dfunc'
 * gemerkt. Alle Funktionsaufrufe innerhalb dieser Funktion bekommen
 * dann 'dfunc' als rufende Funktion vermerkt.
 *
 * Problem:
 * Es gibt Struct Definitionen, bei denen calltree 'denkt' dasz ein
 * Funktionsaufruf vorliegt, ohne dasz vorher eine Funktionsdefinition
 * gefunden wurde. Dann ist der Zeiger auf die Rufende Funktion ein
 * NULL Pointer.
 * Richtige Abhilfe:
 * Funktionsdecoder verbessern.
 *
 * Standard tree:
 *
 * <funcs (tree)> ... -> <sym_t 'name' caller> -> <list of callees (tree)>
 *    ^							|
 *    |_________________________________________________|
 *	Backpointer to adjacent callee pointer in main tree
 *
 *
 * Reversed tree:
 *
 * <funcs (tree)> ... -> <sym_t 'name' callee> -> <list of callers (tree)>
 *    ^							|
 *    |_________________________________________________|
 *	Backpointer to adjacent caller pointer in main tree
 *
 */
LOCAL void
parsefile(fp, filename)
	FILE	*fp;
	char	*filename;
{
	char	name[LEXBSIZE];
	char	fname[LEXBSIZE];
	int	ft;
	sym_t	*dsym = (sym_t *)0;	/* defined function */
	sym_t	*csym;			/* called function */
	sym_t	*usym;			/* this function from uses list */

	if (vflag)
		error("%s\n", filename);

	clexinit();
	fname[0] = '\0';
	lexfile = curfname = filename;

	for (;;) {
		if ((ft = findfunc(fp, name, fname)) == FUNC_DEF) {
			if (lookup(name, &ignorefuncs, L_LOOK))
				continue;
			if (debug)
				error("define func '%s'\n", name);

			dsym = lookup(name, &funcs, L_CREATE);
			if (fname[0] != '\0') {
				usym = lookup(fname, &fnames, L_CREATE);
				dsym->s_filename = usym->s_name;
			} else {
				dsym->s_filename = filename;
			}
			dsym->s_lineno = lexline;
			if ((dsym->s_flags & S_DEF) != 0) {
				/*
				 * Static functions of the same name
				 * are not yet handled.
				 */
				if ((dsym->s_flags & S_WARN) == 0)
					errmsgno(EX_BAD,
						"Warning: function: %s already defined\n", name);
				dsym->s_flags |= S_WARN;
			}
			dsym->s_flags |= S_DEF;
		} else if (ft == FUNC_CALL) {
			if (lookup(name, &ignorefuncs, L_LOOK))
				continue;
			if (debug)
				error("call func '%s'\n", name);
			if (dsym == (sym_t *)0) {	/* Not in Function */
				if (debug)
					error("Bad call: '%s'\n", name);
				continue;
			}

			csym = lookup(name, &funcs, L_CREATE);
			if (rflag) {
				/*
				 * Insert the calling function into the
				 * function list of (this) called function.
				 */
				usym = lookup(dsym->s_name, &(csym->s_uses), funcs);
				usym->s_sym = dsym;	/* Back ptr to caller */
							/* in main funcs tree */
			} else {
				/*
				 * Insert (this) called function into the
				 * function list of the caller.
				 */
				usym = lookup(name, &(dsym->s_uses), funcs);
				usym->s_sym = csym;	/* Back ptr to callee */
							/* in main funcs tree */
			}
		} else {
			if (debug)
				error("EOF '%s'\n", name);
			break;
		}
	}
}

/*
 * Read a file containung a list of function names.
 */
LOCAL int
readfuncs(filename, tab)
	char	*filename;
	sym_t	**tab;
{
	FILE	*fp;
	char	fname[LEXBSIZE];

	if ((fp = fileopen(filename, "r")) == NULL)
		comerr("Cannot open file %s\n", filename);

	while (fgetline(fp, fname, sizeof (fname)) >= 0)
		lookup(fname, tab, L_CREATE);

	return (1);
}

/*
 * Handle CPP command line options.
 */
LOCAL int
got_cpp_arg(name, type)
	char	*name;
	char	*type;
{
	if (debug)
		error("Got CPP arg: %s %s\n", type, name);

	if (Argc >= MAXARGS) {
		errmsgno(EX_BAD, "Too many Preprocessor options.\n");
		return (-2);
	}
	Argv[Argc++] = concat(type, name, (char *)NULL);
	return (1);
}
