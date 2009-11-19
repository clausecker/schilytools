/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
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
/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */
/*
 * Copyright 2002 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * This file contains modifications Copyright 2006-2007 J. Schilling
 *
 * @(#)vc.c	1.6 09/11/08 J. Schilling
 */
#if defined(sun)
#pragma ident "@(#)vc.c 1.6 09/11/08 J. Schilling"
#endif
/*
 * @(#)vc.c 1.6 06/12/12
 */

#if defined(sun)
#pragma ident	"@(#)vc.c"
#pragma ident	"@(#)sccs:cmd/vc.c"
#endif
# include	<defines.h>


/*
 * The symbol table size is set to a limit of forty keywords per input
 * file.  Should this limit be changed it should also be changed in the
 * Help file.
 */

# define SYMSIZE 40
# define PARMSIZE 10
# define NSLOTS 32

# define USD  1
# define DCL 2
# define ASG 4

# define EQ '='
# define NEQ '!'
# define GT '>'
# define LT '<'
# define DELIM " \t"
# define TRUE 1
# define FALSE 0

char SccsError[MAXERRORLEN];

static char	Ctlchar = ':';

struct	symtab	{
	int	usage;
	char	name[PARMSIZE];
	char	*value;
	int	lenval;
};
static struct	symtab	Sym[SYMSIZE];


static char 	*sptr;
static int	Skiptabs;
static int	Repall;

/*
 * Delflag is used to indicate when text is to be skipped.  It is decre-
 * mented whenever an if condition is false, or when an if occurs
 * within a false if/end statement.  It is decremented whenever an end is
 * encountered and the Delflag is greater than zero.  Whenever Delflag
 * is greater than zero text is skipped.
 */

static int	Delflag;

/*
 * Ifcount keeps track of the number of ifs and ends.  Each time
 * an if is encountered Ifcount is incremented and each time an end is
 * encountered it is decremented.
 */

static int	Ifcount;
static int	Lineno;

static char	*Repflag;
static char	*Linend;
static int	Silent;

	int	main __PR((int argc, char **argv));
static void	asgfunc __PR((char *aptr));
static void	dclfunc __PR((char *dptr));
static void	errfunc __PR((char *eptr));
static void	endfunc __PR((void));
static void	msgfunc __PR((char *mptr));
static void	repfunc __PR((char *s));
static void	iffunc __PR((char *iptr));
static int	door __PR((void));
static int	doand __PR((void));
#undef	exp
#define	exp	vc_exp
static int	exp __PR((void));
static char *	getid __PR((char *gptr));
static int	numcomp __PR((char *id1, char *id2));
static void	numck __PR((char *nptr));
static char *	replace __PR((char *ptr));
static int	lookup __PR((char *lname));
static int	putin __PR((char *pname, char *pvalue));
static void	chksize __PR((char *s));
static char *	findch __PR((char *astr, int match));
static char *	ecopy __PR((char *s1, char *s2));
static char *	findstr __PR((char *astr, char *pat));
/*
 * The main program reads a line of text and sends it to be processed
 * if it is a version control statement. If it is a line of text and
 * the Delflag is equal to zero, it is written to the standard output.
 */

int
main(argc, argv)
int argc;
char *argv[];
{
	register  char *lineptr, *p;
	register int i;
	char line[1024];
	extern int Fflags;

	Fflags = FTLCLN | FTLMSG | FTLEXIT;
	setsig();
	for(i = 1; i< argc; i++) {
		p = argv[i];
		if (p[0] == '-')
			switch (p[1]) {
			case 's':
				Silent = 1;
				break;
			case 't':
				Skiptabs = 1;
				break;
			case 'a':
				Repall = 1;
				break;
			case 'c':
				Ctlchar = p[2];
				break;
			}
		else {
			p[size(p) - 1] = '\n';
			asgfunc(p);
		}
	}
	while (fgets(line,sizeof(line),stdin) != NULL) {
		lineptr = line;
		Lineno++;

		if (Repflag != 0) {
			ffree(Repflag);
			Repflag = 0;
		}

		if (Skiptabs) {
			for (p = lineptr; *p; p++)
				if (*p == '\t')
					break;
			if (*p++ == '\t')
				lineptr = p;
		}

		if (lineptr[0] != Ctlchar) {
			if (lineptr[0] == '\\' && lineptr[1] == Ctlchar)
				for (p = &lineptr[1];
				    (*lineptr++ = *p++) != '\0'; )
					;
			if(Delflag == 0) {
				if (Repall)
					repfunc(line);
				else
					if (fputs(line,stdout) == EOF) {
						FAILPUT;
					}
			}
			continue;
		}

		lineptr++;

		if (imatch("if ", lineptr))
			iffunc(&lineptr[3]);
		else if (imatch("end", lineptr))
			endfunc();
		else if (Delflag == 0) {
			if (imatch("asg ", lineptr))
				asgfunc(&lineptr[4]);
			else if (imatch("dcl ", lineptr))
				dclfunc(&lineptr[4]);
			else if (imatch("err", lineptr))
				errfunc(&lineptr[3]);
			else if (imatch("msg", lineptr))
				msgfunc(&lineptr[3]);
			else if (lineptr[0] == Ctlchar)
				repfunc(&lineptr[1]);
			else if (imatch("on", lineptr))
				Repall = 1;
			else if (imatch("off", lineptr))
				Repall = 0;
			else if (imatch("ctl ", lineptr))
				Ctlchar = lineptr[4];
			else {
				sprintf(SccsError,"unknown command on line %d (vc1)",
					Lineno);
				fatal(SccsError);
			}
		}
	}
	for(i = 0; Sym[i].usage != 0 && i<SYMSIZE; i++) {
		if ((Sym[i].usage&USD) == 0 && !Silent)
			fprintf(stderr,"`%s' never used (vc2)\n",Sym[i].name);
		if ((Sym[i].usage&DCL) == 0 && !Silent)
			fprintf(stderr,"`%s' never declared (vc3)\n",
				Sym[i].name);
		if ((Sym[i].usage&ASG) == 0 && !Silent)
			fprintf(stderr,"`%s' never assigned a value (vc20)\n",
				Sym[i].name);
	}
	if (Ifcount > 0)
		fatal("`if' with no matching `end' (vc4)");

	return (0);
}


/*
 * Asgfunc accepts a pointer to a line picks up a keyword name, an
 * equal sign and a value and calls putin to place it in the symbol table.
 */

static void
asgfunc(aptr)
register char *aptr;
{
	register char *end, *aname;
	char *avalue;

	aptr = replace(aptr);
	NONBLANK(aptr);
	aname = aptr;
	end = Linend;
	aptr = findstr(aptr,"= \t");
	if (*aptr == ' ' || *aptr == '\t') {
		*aptr++ = '\0';
		aptr = findch(aptr,'=');
	}
	if (aptr == end) {
		sprintf(SccsError,"syntax on line %d (vc17)",Lineno);
		fatal(SccsError);
	}
	*aptr++ = '\0';
	avalue = getid(aptr);
	chksize(aname);
	putin(aname, avalue);
}


/*
 * Dclfunc accepts a pointer to a line and picks up keywords
 * separated by commas.  It calls putin to put each keyword in the
 * symbol table.  It returns when it sees a newline.
 */

static void
dclfunc(dptr)
register char *dptr;
{
	register char *end, *name;
	int i;

	dptr = replace(dptr);
	end = Linend;
	NONBLANK(dptr);
	while (dptr < end) {
		name = dptr;
		dptr = findch(dptr,',');
		*dptr++ = '\0';
		chksize(name);
		if (Sym[i = lookup(name)].usage&DCL) {
			sprintf(SccsError,"`%s' declared twice on line %d (vc5)", 
				name, Lineno);
			fatal(SccsError);
		}
		else
			Sym[i].usage |= DCL;
		NONBLANK(dptr);
	}
}


/*
 * Errfunc calls fatal which stops the process.
 */

static void
errfunc(eptr)
char *eptr;
{
	if (!Silent)
		fprintf(stderr,"ERROR:%s\n",replace(eptr));
	sprintf(SccsError,"err statement on line %d (vc15)", Lineno);
	fatal(SccsError);
}


/*
 * Endfunc indicates an end has been found by decrementing the if count
 * flag.  If because of a previous if statement, text was being skipped,
 * Delflag is also decremented.
 */

static void
endfunc()
{
	if (--Ifcount < 0) {
		sprintf(SccsError,"`end' without matching `if' on line %d (vc10)",
			Lineno);
		fatal(SccsError);
	}
	if (Delflag > 0)
		Delflag--;
	return;
}


/*
 * Msgfunc accepts a pointer to a line and prints that line on the 
 * diagnostic output.
 */

static void
msgfunc(mptr)
char *mptr;
{
	if (!Silent)
		fprintf(stderr,"Message(%d):%s\n", Lineno, replace(mptr));
}


static void
repfunc(s)
char *s;
{
	fprintf(stdout,"%s\n",replace(s));
}


/*
 * Iffunc and the three functions following it, door, doand, and exp
 * are responsible for parsing and interperting the condition in the
 * if statement.  The BNF used is as follows:
 *	<iffunc> ::=   [ "not" ] <door> EOL
 *	<door> ::=     <doand> | <doand> "|" <door>
 *	<doand>::=     <exp> | <exp> "&" <doand>
 *	<exp>::=       "(" <door> ")" | <value> <operator> <value>
 *	<operator>::=  "=" | "!=" | "<" | ">"
 * And has precedence over or.  If the condition is false the Delflag
 * is bumped to indicate that lines are to be skipped.
 * An external variable, sptr is used for processing the line in
 * iffunc, door, doand, exp, getid.
 * Iffunc accepts a pointer to a line and sets sptr to that line.  The
 * rest of iffunc, door, and doand follow the BNF exactly.
 */

static void
iffunc(iptr)
char *iptr;
{
	register int value, not;

	Ifcount++;
	if (Delflag > 0)
		Delflag++;

	else {
		sptr = replace(iptr);
		NONBLANK(sptr);
		if (imatch("not ", sptr)) {
			not = FALSE;
			sptr += 4;
		}
		else not = TRUE;

		value = door();
		if( *sptr != 0) {
			sprintf(SccsError,"syntax on line %d (vc18)",Lineno);
			fatal(SccsError);
		}

		if (value != not)
			Delflag++;
	}

	return;
}

static int
door()
{
	int value;
	value = doand();
	NONBLANK(sptr);
	while (*sptr=='|') {
		sptr++;
		value |= doand();
		NONBLANK(sptr);
	}
	return(value);
}

static int
doand()
{
	int value;
	value = exp();
	NONBLANK(sptr);
	while (*sptr=='&') {
		sptr++;
		value &= exp();
		NONBLANK(sptr);
	}
	return(value);
}


/*
 * After exp checks for parentheses, it picks up a value by calling getid,
 * picks up an operator and calls getid to pick up the second value.
 * Then based on the operator it calls either numcomp or equal to see
 * if the exp is true or false and returns the correct value.
 */

static int
exp()
{
	register char op, save;
	register int value = 0;
	char *id1, *id2, next = 0;

	NONBLANK(sptr);
	if(*sptr == '(') {
		sptr++;
		value = door();
		NONBLANK(sptr);
		if (*sptr == ')') {
			sptr++;
			return(value);
		}
		else {
			sprintf(SccsError,"parenthesis error on line %d (vc11)",
				Lineno);
		}
	}

	id1 = getid(sptr);
	if ((op = *sptr) != '\0')
		*sptr++ = '\0';
	if (op == NEQ && (next = *sptr++) == '\0')
		--sptr;
	id2 = getid(sptr);
	save = *sptr;
	*sptr = '\0';

	if(op ==LT || op == GT) {
		value = numcomp(id1, id2);
		if ((op == GT && value == 1) || (op == LT && value == -1))
			value = TRUE;
		else value = FALSE;
	}

	else if (op==EQ || (op==NEQ && next==EQ)) {
		value = equal(id1, id2);
		if ( op == NEQ)
			value = !value;
	}

	else {
		sprintf(SccsError,"invalid operator on line %d (vc12)", Lineno);
		fatal(SccsError);
	}
	*sptr = save;
	return(value);
}


/*
 * Getid picks up a value off a line and returns a pointer to the value.
 */

static char *
getid(gptr)
register char *gptr;
{
	register char *id;

	NONBLANK(gptr);
	id = gptr;
	gptr = findstr(gptr,DELIM);
	if (*gptr)
		*gptr++ = '\0';
	NONBLANK(gptr);
	sptr = gptr;
	return(id);
}


/*
 * Numcomp accepts two pointers to strings of digits and calls numck
 * to see if the strings contain only digits.  It returns -1 if
 * the first is less than the second, 1 if the first is greater than the
 * second and 0 if the two are equal.
 */

static int
numcomp(id1, id2)
register char *id1, *id2;
{
	int k1, k2;

	numck(id1);
	numck(id2);
	while (*id1 == '0')
		id1++;
	while (*id2 == '0')
		id2++;
	if ((k1 = size(id1)) > (k2 = size(id2)))
		return(1);
	else if (k1 < k2)
		return(-1);
	else while(*id1 != '\0') {
		if(*id1 > *id2)
			return(1);
		else if(*id1 < *id2)
			return(-1);
		id1++;
		id2++;
	}
	return(0);
}


/*
 * Numck accepts a pointer to a string and checks to see if they are
 * all digits.  If they're not it calls fatal, otherwise it returns.
 */

static void
numck(nptr)
register char *nptr;
{
	for (; *nptr != '\0'; nptr++)
		if (!numeric(*nptr)) {
			sprintf(SccsError,"non-numerical value on line %d (vc14)",
				Lineno);
			fatal(SccsError);
		}
	return;
}


/*
 * Replace accepts a pointer to a line and scans the line for a keyword
 * enclosed in control characters.  If it doesn't find one it returns
 * a pointer to the begining of the line.  Otherwise, it calls
 * lookup to find the keyword.
 * It rewrites the line substituting the value for the
 * keyword enclosed in control characters.  It then continues scanning
 * the line until no control characters are found and returns a pointer to
 * the begining of the new line.
 */

# define INCR(int) if (++int==NSLOTS) { \
		sprintf(SccsError,"out of space [line %d] (vc16)",Lineno); \
		fatal(SccsError); }

static char *
replace(ptr)
char *ptr;
{
	char *slots[NSLOTS];
	int i,j,newlen;
	register char *s, *t, *p;

	for (s=ptr; *s++!='\n';);
	*(--s) = '\0';
	Linend = s;
	i = -1;
	for (p=ptr; *(s=findch(p,Ctlchar)); p=t) {
		*s++ = '\0';
		INCR(i);
		slots[i] = p;
		if (*(t=findch(s,Ctlchar))==0) {
			sprintf(SccsError,"unmatched `%c' on line %d (vc7)",
				Ctlchar,Lineno);
			fatal(SccsError);
		}
		*t++ = '\0';
		INCR(i);
		slots[i] = Sym[j = lookup(s)].value;
		Sym[j].usage |= USD;
	}
	INCR(i);
	slots[i] = p;
	if (i==0) return(ptr);
	newlen = 0;
	for (j=0; j<=i; j++)
		newlen += (size(slots[j])-1);
	t = Repflag = fmalloc((unsigned int) ++newlen);
	for (j=0; j<=i; j++)
		t = ecopy(slots[j],t);
	Linend = t;
	return(Repflag);
}


/*
 * Lookup accepts a pointer to a keyword name and searches the symbol
 * table for the keyword.  It returns its index in the table if its there,
 * otherwise it puts the keyword in the table.
 */

static int
lookup(lname)
char *lname;
{
	register int i;
	register char *t;
	register struct symtab *s;

	t = lname;
	while (((i = *t++) != 0) &&
		((i>='A' && i<='Z') || (i>='a' && i<='z') ||
			(i!= *lname && i>='0' && i<='9')));
	if (i) {
		sprintf(SccsError,"invalid keyword name on line %d (vc9)",Lineno);
		fatal(SccsError);
	}

	for(i =0; Sym[i].usage != 0 && i<SYMSIZE; i++)
		if (equal(lname, Sym[i].name)) return(i);
	s = &Sym[i];
	if (s->usage == 0) {
		copy(lname,s->name);
		copy("",(s->value = fmalloc((unsigned int) (s->lenval = 1))));
		return(i);
	}
	fatal("out of space (vc6)");
	/*NOTREACHED*/
	return (0);	/* fake for gcc */
}


/*
 * Putin accepts a pointer to a keyword name, and a pointer to a value.
 * It puts this information in the symbol table by calling lookup.
 * It returns the index of the name in the table.
 */

static int
putin(pname, pvalue)
char *pname;
char *pvalue;
{
	register int i;
	register struct symtab *s;

	s = &Sym[i = lookup(pname)];
	ffree(s->value);
	s->lenval = size(pvalue);
	copy(pvalue, (s->value = fmalloc((unsigned int) (s->lenval))));
	s->usage |= ASG;
	return(i);
}

static void
chksize(s)
char *s;
{
	if (size(s) > PARMSIZE) {
		sprintf(SccsError,"keyword name too long on line %d (vc8)",Lineno);
		fatal(SccsError);
	}
}


static char *
findch(astr,match)
char *astr, match;
{
	register char *s, *t, c;
	char *temp;

	for (s=astr; ((c = *s) != '\0') && c!=match; s++)
		if (c=='\\') {
			if (s[1]==0) {
				sprintf(SccsError,"syntax on line %d (vc19)",Lineno);
				fatal(SccsError);
			}
			else {
				for (t = (temp=s) + 1; (*s++ = *t++) != '\0';);
				s = temp;
			}
		}
	return(s);
}


static char *
ecopy(s1,s2)
char *s1, *s2;
{
	register char *r1, *r2;

	r1 = s1;
	r2 = s2;
	while ((*r2++ = *r1++) != '\0');
	return(--r2);
}


static char *
findstr(astr,pat)
char *astr, *pat;
{
	register char *s, *t, c;
	char *temp;

	for (s=astr; ((c = *s) != '\0') && any(c,pat)==0; s++)
		if (c=='\\') {
			if (s[1]==0) {
				sprintf(SccsError,"syntax on line %d (vc19)",Lineno);
				fatal(SccsError);
			}
			else {
				for (t = (temp=s) + 1; (*s++ = *t++) != '\0';);
				s = temp;
			}
		}
	return(s);
}

/* for fatal() */
void
clean_up()
{
}
