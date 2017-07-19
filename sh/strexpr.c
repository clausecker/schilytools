/* @(#)strexpr.c	1.23 17/07/09 Copyright 2016-2017 J. Schilling */
#include <schily/mconfig.h>
static	UConst char sccsid[] =
	"@(#)strexpr.c	1.23 17/07/09 Copyright 2016-2017 J. Schilling";
#ifdef	DO_DOL_PAREN
/*
 *	Arithmetic expansion
 *
 *	Copyright (c) 2016-2017 J. Schilling
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

#ifdef	STREX_DEBUG
#define	ARITH_DEBUG
#else
#ifdef	PROTOTYPES
#define	fprintf(...)
#else
#define	fprintf()
#endif
#endif

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

#define	EX_OP		1	/* Looking for an operator	*/
#define	EX_NOEVAL	2	/* Parse but do not evaluate	*/

#define	LOCAL	static

LOCAL	const char	nolvalue[] = "lvalue required";
LOCAL	const char	badexpr[]  = "bad expression";


	Intmax_t	strexpr		__PR((unsigned char *arg));
#ifdef	NEED_PEEKTOK
LOCAL	int		peektok		__PR((struct expr *ep));
#endif
LOCAL	int		exprtok		__PR((struct expr *ep));
LOCAL	Intmax_t	expreval	__PR((struct expr *ep,
							int precedence));
LOCAL	Intmax_t	unary		__PR((struct expr *ep, Intmax_t v,
							int op));
LOCAL	int		getop		__PR((struct expr *ep));
LOCAL	UIntmax_t	number		__PR((struct expr *ep,
						unsigned char *str,
						unsigned char **endptr));
#ifdef	ARITH_DEBUG
LOCAL	char		*tokname	__PR((int t));
#endif

/*
 * Token definitions for the parser
 */
#define	TK_NONE		0
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
#define	TK_VAR		44	/* variable name */
#define	TK_ERROR	45	/* No token found */

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

#ifdef	ARITH_DEBUG
	fprintf(stderr, "strexpr('%s')\n", arg);
#endif
	memset(&exp, 0, sizeof (exp));
	exp.expr = exp.subexpr = exp.tokenp = arg;

	exprtok(&exp);
	ret = expreval(&exp, PR_MAXPREC);
	return (ret);
}

#ifdef	NEED_PEEKTOK
LOCAL int
peektok(ep)
	struct expr	*ep;
{
	struct expr	exp;

	exp = *ep;
	exp.flags = EX_OP | EX_NOEVAL;
	return (exprtok(&exp));
}
#endif

/*
 * Returns the next token.
 * If the next token is a variable or a constant, then the variable is put
 * into ep->val.
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
			expreval(ep, PR_PRIMARY);
		}
		ep->val = unary(ep, ep->val, tok);
		return (tok);
	}
	if (tok == TK_NUM) {
		UIntmax_t	i;
		unsigned char	*np;

		i = number(ep, ep->tokenp, &np);
		ep->val = unary(ep, i, ep->unop);
		ep->var = NULL;
		ep->tokenp = np;

	} else if (tok == TK_VAR) {
		UIntptr_t	b = relstak();
		unsigned char	*np = ep->tokenp;
		struct namnod	*n;
		int		c;
		UIntmax_t	i = 0;
		int		neg = 0;
		unsigned char	*otokenp;
		int		otoken;
		int		xtok;

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
		if (n->namval == NULL) {
			ep->val = i;
		} else if(*n->namval == '\0') {
			ep->val = i;
		} else {
			unsigned char	*nv = n->namval;

			if (*nv == '-') {
				neg++;
				nv++;
			}
			i = number(ep, nv, &np);
		}
		ep->var = n;
		ep->val = unary(ep, neg?-i:i, ep->unop);
		otokenp = ep->tokenp;
		otoken = ep->token;
		xtok = getop(ep);
		if ((flags & setflg) && xtok != TK_ASSIGN && n->namval == NULL)
			failed(n->namid, unset);
		if (xtok == TK_PLUSPLUS || xtok == TK_MINUSMINUS) {
			unary(ep, ep->val, xtok);
			ep->token = otoken;
		} else {
			ep->tokenp = otokenp;
			ep->token = otoken;
		}
	}
	return (tok);
}

/*
 * Returns the result.
 *
 * We start with the first token already read.
 * The first call to exprtok() for this reason either returns the operator
 * or TK_EOF.
 */
LOCAL Intmax_t
expreval(ep, precedence)
	struct expr	*ep;
	int		precedence;
{
	Intmax_t	v;
	struct namnod	*n;
	int		tok;
	int		ntok = TK_NONE;
	int		prec;
	int		nprec;
	unsigned char	*otokenp;
	int		otoken;

	if ((tok = ep->token) == TK_OPAREN) {
		exprtok(ep);
		expreval(ep, PR_PRIMARY);
		ep->var = NULL;
	} else if (tok > TK_EOF &&
		    tok < TK_NUM &&
		    tok != TK_CLPAREN) {
		failed(ep->expr, synmsg);
	}
	v = ep->val;
	n = ep->var;
	otokenp = ep->tokenp;
	otoken = ep->token;
	ep->flags |= EX_OP;
	tok = exprtok(ep);		/* get next token (operator) */
	prec = ep->precedence;

	/*
	 * If there is no next token, then we return the previous value.
	 */
	if (tok == TK_EOF ||
	    ((tok == TK_CLPAREN) && ep->precedence == PR_PRIMARY) ||
	    ((tok == TK_COLON) && ep->precedence == PR_QUEST)) {
		if (tok == TK_COLON) {
			ep->tokenp = otokenp;
			ep->token = otoken;
		}
		return (ep->val);
	}

	/*
	 * Check whether we need at least one additional argument.
	 * This is the case with normal binary operators, with
	 * the conditional operator and with the assignment operators.
	 * The "second value" may be "(".
	 */
	if ((tok >= TK_ADD && tok <= TK_BORASN) || tok == TK_COMMA) {
		int	oflags = ep->flags;

		if ((tok == TK_LAND || tok == TK_QUEST) && v == 0)
			ep->flags |= EX_NOEVAL;
		if ((tok == TK_LOR) && v != 0)
			ep->flags |= EX_NOEVAL;
		ntok = exprtok(ep);
		ep->flags = oflags;
	}

	if (ntok == TK_EOF)
		failed(ep->expr, "missing token");

	do {
		int	otok = tok;

		if (ntok == TK_OPAREN || tok == TK_QUEST) {
			int	oflags = ep->flags;

			if ((tok == TK_LAND || tok == TK_QUEST) && v == 0)
				ep->flags |= EX_NOEVAL;
			if ((tok == TK_LOR) && v != 0)
				ep->flags |= EX_NOEVAL;
			if (ntok == TK_OPAREN) {
				exprtok(ep);
				expreval(ep, PR_PRIMARY);
			} else {
				expreval(ep, PR_QUEST);
			}
			ep->flags = oflags;
		}
		otokenp = ep->tokenp;
		otoken = ep->token;
		ep->flags |= EX_OP;
		ntok = exprtok(ep);	/* Check for next operator */
		nprec = ep->precedence;

		/*
		 * XXX expreval() rekursiv wenn "ntok" höhere Prezedenz hat.
		 */
		if (ntok != TK_EOF &&
		    ((prec > ep->precedence) ||
		    TK_ISASSIGN(ntok))) {
			int	oflags = ep->flags;

			if ((tok == TK_LAND || tok == TK_QUEST) && v == 0)
				ep->flags |= EX_NOEVAL;
			if ((tok == TK_LOR) && v != 0)
				ep->flags |= EX_NOEVAL;
			if (otoken != TK_COLON) {
				ep->tokenp = otokenp;
				ep->token = otoken;
			}
			expreval(ep, ep->precedence);
			ep->flags = oflags;
			ntok = ep->token;
			nprec = ep->precedence;
			tok = otok;
		}

		if (n == NULL && TK_ISASSIGN(tok)) {
			failed(ep->expr, nolvalue);
		}
		if (ep->flags & EX_NOEVAL) {
			if (tok == TK_QUEST)
				goto quest;
			goto noeval;
		}
		switch (tok) {
		case TK_DIV:
		case TK_DIVASN:
		case TK_MOD:
		case TK_MODASN:
			if (ep->val == 0) {
				failed(ep->expr, divzero);
				ep->val = 1;
			}
		}
		switch (tok) {

		case TK_PLUSASN:
		case TK_ADD:	ep->val = v + ep->val;
				break;
		case TK_MINUSASN:
		case TK_SUB:	ep->val = v - ep->val;
				break;
		case TK_MULTASN:
		case TK_MULT:	ep->val = v * ep->val;
				break;
		case TK_DIVASN:
		case TK_DIV:	ep->val = v / ep->val;
				break;
		case TK_MODASN:
		case TK_MOD:	ep->val = v % ep->val;
				break;
		case TK_LSHIFTASN:
		case TK_LSHIFT:	ep->val = v << ep->val;
				break;
		case TK_RSHIFTASN:
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
		case TK_BANDASN:
		case TK_BAND:	ep->val = v & ep->val;
				break;
		case TK_BXORASN:
		case TK_BXOR:	ep->val = v ^ ep->val;
				break;
		case TK_BORASN:
		case TK_BOR:	ep->val = v | ep->val;
				break;
		case TK_LAND:	ep->val = v && ep->val;
				break;
		case TK_LOR:	ep->val = v || ep->val;
				break;

		quest:
		case TK_QUEST: {
					int		oflags = ep->flags;
					Intmax_t	oval = ep->val;

					if (v)
						ep->flags |= EX_NOEVAL;
					exprtok(ep);
					expreval(ep, PR_QUEST);
					ep->flags = oflags;
					if (ntok == TK_COLON) {
						nprec = PR_MAXPREC +1;
					}
					ntok = ep->token;
					if (v)
						ep->val = oval;
				}
				if (ep->flags & EX_NOEVAL)
					goto noeval;
				break;

		case TK_COLON:
				failed(ep->expr, synmsg);
				break;

		}
		if (TK_ISASSIGN(tok)) {
			assign(n, &numbuf[slltos(ep->val)]);
		}
	noeval:
		/*
		 * "precedence" is the precedence we have been called with.
		 * Comparing only works with non-primary expressions, so
		 * if this call was because of an expression in parenthesis
		 * we may not leave here.
		 */
		if (precedence > PR_PRIMARY && precedence < nprec) {
			return (ep->val);
		}

		tok = ntok;
		prec = ep->precedence;
		v = ep->val;
		if (tok != TK_EOF && tok != TK_CLPAREN && tok != TK_COLON) {
			ntok = exprtok(ep);
		} else if (tok == TK_COLON) {
			ep->tokenp = otokenp;
			ep->token = otoken;
		}
	} while (tok != TK_EOF && tok != TK_CLPAREN && tok != TK_COLON);

	return (ep->val);
}

LOCAL Intmax_t
unary(ep, v, op)
	struct expr	*ep;
	Intmax_t	v;
	int		op;
{
	if (op == TK_PLUSPLUS || op == TK_MINUSMINUS) {
		if (ep->var == NULL) {
			failed(ep->expr, nolvalue);
			return (0);
		}
		if ((ep->flags & EX_NOEVAL) == 0) {
			assign(ep->var,
				&numbuf[slltos(op == TK_PLUSPLUS ? ++v:--v)]);
		}
		ep->var = NULL;
		return (v);
	}
	switch (op) {

	case 0:
	case TK_ADD:
	case TK_UPLUS:		return (v);
	case TK_SUB:
	case TK_UMINUS:		return (-v);
	case TK_BNOT:		return (~v);
	case TK_LNOT:		return (!v);

	default:		failed(ep->expr, badexpr);
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
	while (c = *p++, white(c))
		;
	--p;
	if (digit(c)) {
		if (val == TK_PLUSPLUS || val == TK_MINUSMINUS) {
			failed(ep->expr, nolvalue);
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
#ifdef	__needed__
					/*
					 * A comma without a space before only
					 * may appear inside international
					 * floating point numbers.
					 */
					if (ep->subexpr == ep->tokenp)
						break;
#endif
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
	failed(ep->expr, badexpr);
	ep->flags &= ~EX_OP;
	return (ep->token = TK_ERROR);
}

LOCAL UIntmax_t
number(ep, str, endptr)
	struct expr	*ep;
	unsigned char	*str;
	unsigned char	**endptr;
{
	UIntmax_t	i;
	int		c;

#ifdef	HAVE_STRTOULL
	i = strtoull(C str, CP endptr, 0);
#else
	*endptr = UC astoull(C str, &i);
#endif
	c = **endptr;
	if (alphanum(c))
		failed(ep->expr, badnum);
	return (i);
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
#endif	/* DO_DOL_PAREN */
