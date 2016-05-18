/* @(#)strexpr.c	1.5 16/05/17 Copyright 2016 J. Schilling */
#include <schily/mconfig.h>
static	UConst char sccsid[] =
	"@(#)strexpr.c	1.5 16/05/17 Copyright 2016 J. Schilling";
/*
 *	Arithmetic expansion
 *
 *	Copyright (c) 2016 J. Schilling
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

#include "defs.h"

struct expr {
	unsigned char	*expr;
	unsigned char	*subexpr;
	unsigned char	*tokenp;
	struct namnod	*var;
	int		token;
	int		unop;
	int		precedence;
	int		flags;
	Intmax_t	val;
};

#define	EX_OP		1	/* Looking for an operator */

#define	LOCAL	static

	Intmax_t	strexpr		__PR((unsigned char *arg));
LOCAL	int		exprtok		__PR((struct expr *ep));
LOCAL	Intmax_t	expreval	__PR((struct expr *ep,
							int precedence));
LOCAL	Intmax_t	unary		__PR((struct expr *ep, Intmax_t v,
							int op));
LOCAL	int		getop		__PR((struct expr *ep));
#ifdef	ARITH_DEBUG
LOCAL	char		*tokname	__PR((int t));
#endif

/*
 * Token definitions for the parser
 */
#define	TK_EOF		1	/* End of string seen */
#define	TK_ADD		2	/* +  */
#define	TK_SUB		3	/* -  */
#define	TK_MULT		4	/* *  */
#define	TK_DIV		5	/* /  */
#define	TK_MOD		6	/* %  */
#define	TK_LSHIFT	7	/* << */
#define	TK_RSHIFT	8	/* >> */
#define	TK_LT		9	/* <  */
#define	TK_GT		10	/* >  */
#define	TK_LE		11	/* <= */
#define	TK_GE		12	/* >= */
#define	TK_EQUAL	13	/* == */
#define	TK_NEQUAL	14	/* != */
#define	TK_BAND		15	/* &  */
#define	TK_BXOR		16	/* ^  */
#define	TK_BOR		17	/* |  */
#define	TK_LAND		18	/* && */
#define	TK_LOR		19	/* || */
#define	TK_QUEST	20	/* ?  */
#define	TK_COLON	21	/* :  */
#define	TK_ASSIGN	22	/* =  */
#define	TK_PLUSASN	23	/* += */
#define	TK_MINUSASN	24	/* -= */
#define	TK_MULTASN	25	/* *= */
#define	TK_DIVASN	26	/* /= */
#define	TK_MODASN	27	/* %= */
#define	TK_LSHIFTASN	28	/* <<= */
#define	TK_RSHIFTASN	29	/* >>= */
#define	TK_BANDASN	30	/* &= */
#define	TK_BXORASN	31	/* ^= */
#define	TK_BORASN	32	/* |= */
#define	TK_PLUSPLUS	33	/* ++ */
#define	TK_MINUSMINUS	34	/* -- */
#define	TK_UPLUS	35	/* +  */
#define	TK_UMINUS	36	/* -  */
#define	TK_BNOT		37	/* ~  */
#define	TK_LNOT		38	/* !  */
#define	TK_OPAREN	39	/* (  */
#define	TK_CLPAREN	40	/* )  */
#define	TK_COMMA	41	/*  , */
#define	TK_END		42	/* op-delimiter */
#define	TK_NUM		43	/* numerical value */
#define	TK_VAR		45	/* variable name */
#define	TK_ERROR	47	/* No token found */

#define	TK_ISBINARYOP	((t) >= TK_ADD && (t) <= TK_LOR)
#define	TK_ISASSIGN(t)	((t) >= TK_ASSIGN && (t) <= TK_BORASN)
#define	TK_ISUNARYOP(t)	((t) >= TK_PLUSPLUS && (t) <= TK_LNOT)

/*
 * Precedence values, lower numbers yield higher precedence.
 */
#define	PR_PRIMARY	0	/* var, (), ! ~ ++ -- */
#define	PR_MULT		1	/* * / % */
#define	PR_ADD		2	/* + - */
#define	PR_SHIFT	3	/* << >> */
#define	PR_COMPARE	4	/* < > <= >= */
#define	PR_EQUAL	5	/* == != */
#define	PR_BAND		6	/* & */
#define	PR_BXOR		7	/* ^ */
#define	PR_BOR		8	/* | */
#define	PR_LAND		9	/* && */
#define	PR_LOR		10	/* || */
#define	PR_QUEST	11	/* ? */
#define	PR_ASSIGN	12	/* = += -= *= /= %= <<= >>= &= ^= |= */
#define	PR_COMMA	13	/*  , */
#define	PR_MAXPREC	13

/*
 * Operators, longer strings need to be before shorter sub-strings.
 */
struct ops {
	char	name[4];
	char	nlen;
	char	val;
	char	precedence;
} ops[] = {
	{ "++",		2,	TK_PLUSPLUS,	PR_PRIMARY, },
	{ "--",		2,	TK_MINUSMINUS,	PR_PRIMARY, },
	{ "==",		2,	TK_EQUAL,	PR_EQUAL, },
	{ "!=",		2,	TK_NEQUAL,	PR_EQUAL, },

	{ "+=",		2,	TK_PLUSASN,	PR_ASSIGN, },
	{ "-=",		2,	TK_MINUSASN,	PR_ASSIGN, },
	{ "*=",		2,	TK_MULTASN,	PR_ASSIGN, },
	{ "/=",		2,	TK_DIVASN,	PR_ASSIGN, },
	{ "%=",		2,	TK_MODASN,	PR_ASSIGN, },
	{ "<<=",	3,	TK_LSHIFTASN,	PR_ASSIGN, },
	{ ">>=",	3,	TK_RSHIFTASN,	PR_ASSIGN, },
	{ "&=",		2,	TK_BANDASN,	PR_ASSIGN, },
	{ "^=",		2,	TK_BXORASN,	PR_ASSIGN, },
	{ "|=",		2,	TK_BORASN,	PR_ASSIGN, },
	{ "=",		1,	TK_ASSIGN,	PR_ASSIGN, },
	{ "&&",		2,	TK_LAND,	PR_LAND, },
	{ "||",		2,	TK_LOR,		PR_LOR, },
	{ "+",		1,	TK_ADD,		PR_ADD, },
	{ "-",		1,	TK_SUB,		PR_ADD, },
	{ "*",		1,	TK_MULT,	PR_MULT, },
	{ "/",		1,	TK_DIV,		PR_MULT, },
	{ "%",		1,	TK_MOD,		PR_MULT, },
	{ "<<",		2,	TK_LSHIFT,	PR_SHIFT, },
	{ ">>",		2,	TK_RSHIFT,	PR_SHIFT, },
	{ "&",		1,	TK_BAND,	PR_BAND, },
	{ "^",		1,	TK_BXOR,	PR_BXOR, },
	{ "|",		1,	TK_BOR,		PR_BOR, },
	{ "<=",		2,	TK_LE,		PR_COMPARE, },
	{ ">=",		2,	TK_GE,		PR_COMPARE, },
	{ "<",		1,	TK_LT,		PR_COMPARE, },
	{ ">",		1,	TK_GT,		PR_COMPARE, },
	{ "~",		1,	TK_BNOT,	PR_PRIMARY, },
	{ "!",		1,	TK_LNOT,	PR_PRIMARY, },
	{ "(",		1,	TK_OPAREN,	PR_PRIMARY, },
	{ ")",		1,	TK_CLPAREN,	PR_PRIMARY, },
	{ "?",		1,	TK_QUEST,	PR_QUEST, },
	{ ":",		1,	TK_COLON,	PR_QUEST, },
	{ ",",		1,	TK_COMMA,	PR_COMMA, },
	{ "",		1,	TK_EOF,		PR_PRIMARY, },
	{ "",		0,	TK_END,		PR_PRIMARY, }
};

/*
 * Returns the result.
 */
Intmax_t
strexpr(arg)
	unsigned char	*arg;
{
	struct expr	exp;
	Intmax_t	ret;

	memset(&exp, 0, sizeof (exp));
	exp.expr = exp.subexpr = exp.tokenp = arg;

	exprtok(&exp);
	ret = expreval(&exp, PR_MAXPREC);
	return (ret);
}

/*
 * Returns the next token.
 * If the next token is a variable or a constant, then the variable is put
 * ino ep->val.
 */
LOCAL int
exprtok(ep)
	struct expr	*ep;
{
	int	flag = ep->flags;
	int	tok = getop(ep);	/* get token */

	if ((flag & EX_OP) == 0 &&
	    (tok == TK_ADD || tok == TK_SUB ||
	    TK_ISUNARYOP(tok))) {
		exprtok(ep);
		if (ep->token == TK_OPAREN) {
			exprtok(ep);
			expreval(ep, ep->precedence);
		}
		ep->val = unary(ep, ep->val, tok);
		return (tok);
	}
	if (tok == TK_NUM) {
		UIntmax_t	i;
		unsigned char	*np;

#ifdef	HAVE_STRTOULL
		i = strtoull(C ep->tokenp, CP &np, 0);
#else
		np = UC astoull(C ep->tokenp, &i);
#endif

		ep->val = unary(ep, i, ep->unop);
		ep->tokenp = np;

	} else if (tok == TK_VAR) {
		UIntptr_t	b = relstak();
		unsigned char	*np = ep->tokenp;
		struct namnod	*n;
		int		c;

		while ((c = *np++) != '\0') {
			if (!alphanum(c))
				break;
			GROWSTAKTOP();
			pushstak(c);
		}
		zerostak();
		staktop = absstak(b);
		ep->tokenp = --np;
		n = lookup(staktop);
		if (n->namval == NULL || *n->namval == '\0') {
			ep->val = 0;
		} else {
			UIntmax_t	i;

#ifdef	HAVE_STRTOULL
			i = strtoull(C n->namval, CP &np, 0);
#else
			np = UC astoull(C n->namval, &i);
#endif
			ep->val = unary(ep, i, ep->unop);
		}
	}
	return (tok);
}

/*
 * Returns the result.
 *
 * We start with the first token already read.
 * The first call to exprtok() for this reason either returns the oparator
 * or TK_EOF.
 */
LOCAL Intmax_t
expreval(ep, precedence)
	struct expr	*ep;
	int		precedence;
{
	Intmax_t	v;
	int		tok;
	int		otok;
	int		ntok = TK_EOF;
	int		prec;

	if (ep->token == TK_OPAREN) {
		exprtok(ep);
		expreval(ep, ep->precedence);
	} else if (ep->token > TK_EOF &&
		    ep->token < TK_NUM &&
		    ep->token != TK_CLPAREN) {
		failed(ep->expr, synmsg);
	}
	v = ep->val;
	otok = ep->token;
	ep->flags |= EX_OP;
	tok = exprtok(ep);		/* get next token (oparator) */
	prec = ep->precedence;
	if (tok == TK_PLUSPLUS || tok == TK_MINUSMINUS) {
		failed(ep->expr, "lvalue required");
	}

	/*
	 * If there is no next token, then we return the previous value.
	 */
	if ((tok == TK_EOF || tok == TK_CLPAREN) &&
	    ep->precedence == PR_PRIMARY) {
		return (ep->val);
	}

	/*
	 * If we found a normal (non-assignment) oparator, fetch the
	 * second value. The "second value" may be "(".
	 */
	if (tok >= TK_ADD && tok <= TK_QUEST)
		ntok = exprtok(ep);


	if (ntok == TK_EOF)
		failed(ep->expr, "missing token");

	do {
		unsigned char	*otokenp;
		int		otoken;

		otok = tok;
		if (ntok == TK_OPAREN) {
			exprtok(ep);
			expreval(ep, ep->precedence);
		}
		otokenp = ep->tokenp;
		otoken = ep->token;
		ep->flags |= EX_OP;
		ntok = exprtok(ep);	/* Check for next oparator */
		/*
		 * XXX expreval() rekursiv wenn "ntok" höhere Prezedenz hat.
		 */
		if (ntok != TK_EOF && prec > ep->precedence) {
			ep->tokenp = otokenp;
			ep->token = otoken;
			expreval(ep, ep->precedence);
			ntok = ep->token;
			tok = otok;
		}

		switch (tok) {
		case TK_DIV:
		case TK_DIVASN:
		case TK_MOD:
		case TK_MODASN:
			if (ep->val == 0) {
				failed(ep->expr, "divide by zero");
				ep->val = 1;
			}
		}
		switch (tok) {

		case TK_ADD:	ep->val = v + ep->val;
				break;
		case TK_SUB:	ep->val = v - ep->val;
				break;
		case TK_MULT:	ep->val = v * ep->val;
				break;
		case TK_DIV:	ep->val = v / ep->val;
				break;
		case TK_MOD:	ep->val = v % ep->val;
				break;
		case TK_LSHIFT:	ep->val = v << ep->val;
				break;
		case TK_RSHIFT:	ep->val = v >> ep->val;
				break;
		case TK_LT:	ep->val = v < ep->val;
				break;
		case TK_GT:	ep->val = v > ep->val;
				break;
		case TK_LE:	ep->val = v <= ep->val;
				break;
		case TK_GE:	ep->val = v >= ep->val;
				break;
		case TK_EQUAL:	ep->val = v == ep->val;
				break;
		case TK_NEQUAL:	ep->val = v != ep->val;
				break;
		case TK_BAND:	ep->val = v & ep->val;
				break;
		case TK_BXOR:	ep->val = v ^ ep->val;
				break;
		case TK_BOR:	ep->val = v | ep->val;
				break;
		case TK_LAND:	ep->val = v && ep->val;
				break;
		case TK_LOR:	ep->val = v || ep->val;
				break;

		case TK_QUEST:
		case TK_COLON:
				ep->val = 999;
				break;

		}
		if (precedence <= ep->precedence) {
			return (ep->val);
		}

		tok = ntok;
		prec = ep->precedence;
		v = ep->val;
		if (tok != TK_EOF && tok != TK_CLPAREN) {
			ntok = exprtok(ep);
		}
	} while (tok != TK_EOF && tok != TK_CLPAREN);

	return (ep->val);
}

LOCAL Intmax_t
unary(ep, v, op)
	struct expr	*ep;
	Intmax_t	v;
	int		op;
{
	switch (op) {

	case 0:
	case TK_ADD:
	case TK_UPLUS:		return (v);
	case TK_SUB:
	case TK_UMINUS:		return (-v);
	case TK_PLUSPLUS:	return (++v);
	case TK_MINUSMINUS:	return (--v);
	case TK_BNOT:		return (~v);
	case TK_LNOT:		return (!v);

	default:		failed(ep->expr, "bad expression");
				return (0);
	}
}

/*
 * Returns next operator token.
 * If the next token is a variable or a constant, then a primary operator in
 * front of the variable/number is returned as a combined token.
 */
LOCAL int
getop(ep)
	struct expr	*ep;
{
	int		i;
	int		c;
	int		val = 0;
	unsigned char	*p = ep->tokenp;

	ep->unop = 0;
again:
	while (c = *p++, space(c))
		;
	--p;
	if (digit(c)) {
		if (val == TK_PLUSPLUS || val == TK_MINUSMINUS) {
			failed(ep->expr, "lvalue required");
			return (ep->token = val);
		}

		ep->tokenp = p;
		ep->precedence = PR_PRIMARY;
		ep->flags &= ~EX_OP;

		ep->unop = val;
		return (ep->token = TK_NUM);
	} else if (letter(c)) {
		ep->tokenp = p;
		ep->precedence = PR_PRIMARY;
		ep->flags &= ~EX_OP;

		ep->unop = val;
		return (ep->token = TK_VAR);
	} else {
		if (val)
			return (ep->token = val);
		for (i = 0; ops[i].val != TK_END; i++)  {
			if (strncmp(C p, ops[i].name, ops[i].nlen) == 0) {
				val = ops[i].val;

				if (val != TK_EOF)
					p += ops[i].nlen;

				if (val == TK_COMMA) {
					/*
					 * A comma without a space before only
					 * may appear inside international
					 * floating point numbers.
					 */
					if (ep->subexpr == ep->tokenp)
						break;
					ep->subexpr = ep->tokenp;
				} else if ((ep->flags & EX_OP) == 0 &&
					    (val == TK_ADD || val == TK_SUB ||
					    TK_ISUNARYOP(val))) {
					ep->tokenp = p;
					ep->precedence = ops[i].precedence;
					goto again;
				}
				ep->flags &= ~EX_OP;
				ep->tokenp = p;
				ep->precedence = ops[i].precedence;
				return (ep->token = val);
			}
		}
	}
	failed(ep->expr, "bad expression");
	ep->flags &= ~EX_OP;
	return (ep->token = TK_ERROR);
}

#ifdef	ARITH_DEBUG
LOCAL char *
tokname(t)
	int	t;
{
	int	i;

	/*
	 * First check for the values that are missing in our table:
	 */
	switch (t) {

	case 0:		return ("UNINTIALIZED");
	case TK_UPLUS:	return ("unary +");
	case TK_UMINUS:	return ("unary -");
	case TK_EOF:	return ("EOF");
	case TK_END:	return ("ENDE DER LISTE");
	case TK_NUM:	return ("NUMBER");
	case TK_VAR:	return ("VARIABLE");
	case TK_ERROR:	return ("ERROR");
	}

	/*
	 * Now search the table:
	 */
	for (i = 0; ops[i].val != TK_END; i++) {
		if (ops[i].val == t)
			return (ops[i].name);
	}

	/*
	 * Any other value:
	 */
	return ("???");
}
#endif
