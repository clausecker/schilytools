
# line 2 "cpy.y"
/* @(#)cpy.y	1.6 18/06/17 2010-2018 J. Schilling */
#ifndef lint
static	char y_sccsid[] =
	"@(#)cpy.y	1.6 18/06/17 2010-2018 J. Schilling";
#endif
/*
 * This implementation is based on the UNIX 32V release from 1978
 * with permission from Caldera Inc.
 *
 * Copyright (c) 2010-2018 J. Schilling
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTOR(S) ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTOR(S) BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code and documentation must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:  This product includes
 *    software developed or owned by Caldera International, Inc.
 *
 * 4. Neither the name of Caldera International, Inc. nor the names of other
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC.  AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE FOR
 * ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <schily/mconfig.h>

#include "cpp.h"
#include <schily/limits.h>
# define number 257
# define stop 258
# define DEFINED 259
# define EQ 260
# define NE 261
# define LE 262
# define GE 263
# define LS 264
# define RS 265
# define ANDAND 266
# define OROR 267
# define UMINUS 268

#include <schily/inttypes.h>

#ifdef	IS_SCHILY
#include <schily/stdlib.h>
#include <schily/string.h>
#define	YYCONST	const
#else
#ifdef __STDC__
#include <stdlib.h>
#include <string.h>
#define	YYCONST	const
#else
#include <malloc.h>
#include <memory.h>
#define	YYCONST
#endif
#endif

#include <schily/values.h>

#if defined(__cplusplus) || defined(__STDC__)

#if defined(__cplusplus) && defined(__EXTERN_C__)
extern "C" {
#endif
#ifndef yyerror
#if defined(__cplusplus)
	void yyerror(YYCONST char *);
#endif
#endif
#ifndef yylex
	int yylex(void);
#endif
	int yyparse(void);
#if defined(__cplusplus) && defined(__EXTERN_C__)
}
#endif

#endif

#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern int yyerrflag;
#ifndef YYSTYPE
#define YYSTYPE int
#endif
YYSTYPE yylval;
YYSTYPE yyval;
typedef int yytabelem;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
#if YYMAXDEPTH > 0
int yy_yys[YYMAXDEPTH], *yys = yy_yys;
YYSTYPE yy_yyv[YYMAXDEPTH], *yyv = yy_yyv;
#else	/* user does initial allocation */
int *yys;
YYSTYPE *yyv;
#endif
static int yymaxdepth = YYMAXDEPTH;
# define YYERRCODE 256

# line 176 "cpy.y"

# include "yylex.c"
static YYCONST yytabelem yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 44,
	262, 0,
	263, 0,
	60, 0,
	62, 0,
	-2, 9,
-1, 45,
	262, 0,
	263, 0,
	60, 0,
	62, 0,
	-2, 10,
-1, 46,
	262, 0,
	263, 0,
	60, 0,
	62, 0,
	-2, 11,
-1, 47,
	262, 0,
	263, 0,
	60, 0,
	62, 0,
	-2, 12,
-1, 48,
	260, 0,
	261, 0,
	-2, 13,
-1, 49,
	260, 0,
	261, 0,
	-2, 14,
	};
# define YYNPROD 30
# define YYLAST 363
static YYCONST yytabelem yyact[]={

    13,    24,    35,    58,    13,    11,    14,    30,    15,    11,
    12,    60,    13,    24,    12,     1,    57,    11,    14,    30,
    15,    59,    12,    18,    13,    19,    29,     0,     0,    11,
    14,     0,    15,     0,    12,    18,     3,    19,    29,    13,
    24,    31,    32,    33,    11,    14,    30,    15,     0,    12,
    13,    24,     0,     0,     0,    11,    14,    25,    15,     5,
    12,     0,    18,     0,    19,    29,     7,     0,     0,    25,
     0,     4,     0,    18,     0,    19,    29,     0,    13,    24,
     0,     0,     0,    11,    14,     0,    15,    26,    12,     0,
     0,     0,    13,    24,     0,     0,    25,    11,    14,    26,
    15,    18,    12,    19,    13,    24,     0,    25,     0,    11,
    14,     0,    15,    13,    12,    18,     0,    19,    11,    14,
     0,    15,     0,    12,     0,     0,    26,    18,     0,    19,
     0,     0,     0,     0,    13,    25,    18,    26,    19,    11,
    14,     0,    15,     0,    12,    13,     0,     0,     0,    25,
    11,    14,     6,    15,     0,    12,     0,    18,     0,    19,
     0,     0,     0,     0,     0,    26,     0,     0,     0,     0,
     0,     0,     2,     0,     0,     0,     0,     0,     0,    26,
    34,     0,     0,     0,    37,    38,    39,    40,    41,    42,
    43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
    53,    54,    55,    56,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,    36,
     0,     0,     0,    22,    23,    20,    21,    16,    17,    27,
    28,     0,    61,     0,     0,    22,    23,    20,    21,    16,
    17,    27,    28,     0,     0,     0,     0,     0,     0,     0,
     0,    16,    17,     0,     0,     0,     0,     0,     0,     0,
    10,     0,    22,    23,    20,    21,    16,    17,    27,    28,
     0,     0,     0,    22,    23,    20,    21,    16,    17,    27,
    28,     0,     0,     9,     0,     8,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    22,    23,    20,    21,    16,    17,    27,     0,     0,
     0,     0,     0,     0,     0,    22,    23,    20,    21,    16,
    17,     0,     0,     0,     0,     0,     0,    22,    23,    20,
    21,    16,    17,     0,     0,     0,    22,    23,    20,    21,
    16,    17,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,    20,
    21,    16,    17 };
static YYCONST yytabelem yypact[]={

    26,-10000000,     2,-10000000,    26,    26,    26,    26,   -38,-10000000,
-10000000,    26,    26,    26,    26,    26,    26,    26,    26,    26,
    26,    26,    26,    26,    26,    26,    26,    26,    26,    26,
    26,-10000000,-10000000,-10000000,   -25,  -254,-10000000,-10000000,-10000000,-10000000,
   -33,   -33,   108,   108,   -13,   -13,   -13,   -13,    97,    97,
    76,    67,    67,    55,    41,   -37,    13,-10000000,   -30,    26,
-10000000,    13 };
static YYCONST yytabelem yypgo[]={

     0,    15,   172,    36 };
static YYCONST yytabelem yyr1[]={

     0,     1,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     3,     3,     3,     3,     3,     3,     3 };
static YYCONST yytabelem yyr2[]={

     0,     5,     7,     7,     7,     7,     7,     7,     7,     7,
     7,     7,     7,     7,     7,     7,     7,     7,     7,     7,
    11,     7,     3,     5,     5,     5,     7,     9,     5,     3 };
static YYCONST yytabelem yychk[]={

-10000000,    -1,    -2,    -3,    45,    33,   126,    40,   259,   257,
   258,    42,    47,    37,    43,    45,   264,   265,    60,    62,
   262,   263,   260,   261,    38,    94,   124,   266,   267,    63,
    44,    -3,    -3,    -3,    -2,    40,   257,    -2,    -2,    -2,
    -2,    -2,    -2,    -2,    -2,    -2,    -2,    -2,    -2,    -2,
    -2,    -2,    -2,    -2,    -2,    -2,    -2,    41,   257,    58,
    41,    -2 };
static YYCONST yytabelem yydef[]={

     0,    -2,     0,    22,     0,     0,     0,     0,     0,    29,
     1,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,    23,    24,    25,     0,     0,    28,     2,     3,     4,
     5,     6,     7,     8,    -2,    -2,    -2,    -2,    -2,    -2,
    15,    16,    17,    18,    19,     0,    21,    26,     0,     0,
    27,    20 };
typedef struct
#ifdef __cplusplus
	yytoktype
#endif
{
#ifdef __cplusplus
const
#endif
char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

yytoktype yytoks[] =
{
	"number",	257,
	"stop",	258,
	"DEFINED",	259,
	"EQ",	260,
	"NE",	261,
	"LE",	262,
	"GE",	263,
	"LS",	264,
	"RS",	265,
	"ANDAND",	266,
	"OROR",	267,
	",",	44,
	"=",	61,
	"?",	63,
	":",	58,
	"|",	124,
	"^",	94,
	"&",	38,
	"<",	60,
	">",	62,
	"+",	43,
	"-",	45,
	"*",	42,
	"/",	47,
	"%",	37,
	"!",	33,
	"~",	126,
	"UMINUS",	268,
	"(",	40,
	".",	46,
	"-unknown-",	-1	/* ends search */
};

#ifdef __cplusplus
const
#endif
char * yyreds[] =
{
	"-no such reduction-",
	"S : e stop",
	"e : e '*' e",
	"e : e '/' e",
	"e : e '%' e",
	"e : e '+' e",
	"e : e '-' e",
	"e : e LS e",
	"e : e RS e",
	"e : e '<' e",
	"e : e '>' e",
	"e : e LE e",
	"e : e GE e",
	"e : e EQ e",
	"e : e NE e",
	"e : e '&' e",
	"e : e '^' e",
	"e : e '|' e",
	"e : e ANDAND e",
	"e : e OROR e",
	"e : e '?' e ':' e",
	"e : e ',' e",
	"e : term",
	"term : '-' term",
	"term : '!' term",
	"term : '~' term",
	"term : '(' e ')'",
	"term : DEFINED '(' number ')'",
	"term : DEFINED number",
	"term : number",
};
#endif /* YYDEBUG */
# line	1 "/usr/share/lib/ccs/yaccpar"
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
/*
 * Copyright 1993 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/* Copyright (c) 1988 AT&T */
/* All Rights Reserved */

#pragma ident	"@(#)cpypre.c	1.4	19/08/29 SMI"

/*
** Skeleton parser driver for yacc output
*/

/*
** yacc user known macros and defines
*/
#define YYERROR		goto yyerrlab
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#define YYBACKUP( newtoken, newvalue )\
{\
	if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
	{\
		yyerror( "syntax error - cannot backup" );\
		goto yyerrlab;\
	}\
	yychar = newtoken;\
	yystate = *yyps;\
	yylval = newvalue;\
	goto yynewstate;\
}
#define YYRECOVERING()	(!!yyerrflag)
#define YYNEW(type)	malloc(sizeof(type) * yynewmax)
#define YYCOPY(to, from, type) \
	(type *) memcpy(to, (char *) from, yymaxdepth * sizeof (type))
#define YYENLARGE( from, type) \
	(type *) realloc((char *) from, yynewmax * sizeof(type))
#ifndef YYDEBUG
#	define YYDEBUG	1	/* make debugging available */
#endif

/*
** user known globals
*/
int yydebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
#define YYFLAG		(-10000000)

/*
** global variables used by the parser
*/
YYSTYPE *yypv;			/* top of value stack */
int *yyps;			/* top of state stack */

int yystate;			/* current state */
int yytmp;			/* extra var (lasts between blocks) */

int yynerrs;			/* number of errors */
int yyerrflag;			/* error recovery flag */
int yychar;			/* current input token number */



#ifdef YYNMBCHARS
#define YYLEX()		yycvtok(yylex())
/*
** yycvtok - return a token if i is a wchar_t value that exceeds 255.
**	If i<255, i itself is the token.  If i>255 but the neither 
**	of the 30th or 31st bit is on, i is already a token.
*/
#if defined(__STDC__) || defined(__cplusplus)
int yycvtok(int i)
#else
int yycvtok(i) int i;
#endif
{
	int first = 0;
	int last = YYNMBCHARS - 1;
	int mid;
	wchar_t j;

	if(i&0x60000000){/*Must convert to a token. */
		if( yymbchars[last].character < i ){
			return i;/*Giving up*/
		}
		while ((last>=first)&&(first>=0)) {/*Binary search loop*/
			mid = (first+last)/2;
			j = yymbchars[mid].character;
			if( j==i ){/*Found*/ 
				return yymbchars[mid].tvalue;
			}else if( j<i ){
				first = mid + 1;
			}else{
				last = mid -1;
			}
		}
		/*No entry in the table.*/
		return i;/* Giving up.*/
	}else{/* i is already a token. */
		return i;
	}
}
#else/*!YYNMBCHARS*/
#define YYLEX()		yylex()
#endif/*!YYNMBCHARS*/

/*
** yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
#if defined(__STDC__) || defined(__cplusplus)
int yyparse(void)
#else
int yyparse()
#endif
{
	register YYSTYPE *yypvt = 0;	/* top of value stack for $vars */

#if defined(__cplusplus) || defined(lint)
/*
	hacks to please C++ and lint - goto's inside
	switch should never be executed
*/
	static int __yaccpar_lint_hack__ = 0;
	switch (__yaccpar_lint_hack__)
	{
		case 1: goto yyerrlab;
		case 2: goto yynewstate;
	}
#endif

	/*
	** Initialize externals - yyparse may be called more than once
	*/
	yypv = &yyv[-1];
	yyps = &yys[-1];
	yystate = 0;
	yytmp = 0;
	yynerrs = 0;
	yyerrflag = 0;
	yychar = -1;

#if YYMAXDEPTH <= 0
	if (yymaxdepth <= 0)
	{
		if ((yymaxdepth = YYEXPAND(0)) <= 0)
		{
			yyerror("yacc initialization error");
			YYABORT;
		}
	}
#endif

	{
		register YYSTYPE *yy_pv;	/* top of value stack */
		register int *yy_ps;		/* top of state stack */
		register int yy_state;		/* current state */
		register int  yy_n;		/* internal state number info */
	goto yystack;	/* moved from 6 lines above to here to please C++ */

		/*
		** get globals into registers.
		** branch to here only if YYBACKUP was called.
		*/
	yynewstate:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;
		goto yy_newstate;

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
	yystack:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;

		/*
		** top of for (;;) loop while no reductions done
		*/
	yy_stack:
		/*
		** put a state and value onto the stacks
		*/
#if YYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( yydebug )
		{
			register int yy_i;

			printf( "State %d, token ", yy_state );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ++yy_ps >= &yys[ yymaxdepth ] )	/* room on stack? */
		{
			/*
			** reallocate and recover.  Note that pointers
			** have to be reset, or bad things will happen
			*/
			long yyps_index = (yy_ps - yys);
			long yypv_index = (yy_pv - yyv);
			long yypvt_index = (yypvt - yyv);
			int yynewmax;
#ifdef YYEXPAND
			yynewmax = YYEXPAND(yymaxdepth);
#else
			yynewmax = 2 * yymaxdepth;	/* double table size */
			if (yymaxdepth == YYMAXDEPTH)	/* first time growth */
			{
				char *newyys = (char *)YYNEW(int);
				char *newyyv = (char *)YYNEW(YYSTYPE);
				if (newyys != 0 && newyyv != 0)
				{
					yys = YYCOPY(newyys, yys, int);
					yyv = YYCOPY(newyyv, yyv, YYSTYPE);
				}
				else
					yynewmax = 0;	/* failed */
			}
			else				/* not first time */
			{
				yys = YYENLARGE(yys, int);
				yyv = YYENLARGE(yyv, YYSTYPE);
				if (yys == 0 || yyv == 0)
					yynewmax = 0;	/* failed */
			}
#endif
			if (yynewmax <= yymaxdepth)	/* tables not expanded */
			{
				yyerror( "yacc stack overflow" );
				YYABORT;
			}
			yymaxdepth = yynewmax;

			yy_ps = yys + yyps_index;
			yy_pv = yyv + yypv_index;
			yypvt = yyv + yypvt_index;
		}
		*yy_ps = yy_state;
		*++yy_pv = yyval;

		/*
		** we have a new state - find out what to do
		*/
	yy_newstate:
		if ( ( yy_n = yypact[ yy_state ] ) <= YYFLAG )
			goto yydefault;		/* simple state */
#if YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		yytmp = yychar < 0;
#endif
		if ( ( yychar < 0 ) && ( ( yychar = YYLEX() ) < 0 ) )
			yychar = 0;		/* reached EOF */
#if YYDEBUG
		if ( yydebug && yytmp )
		{
			register int yy_i;

			printf( "Received token " );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ( ( yy_n += yychar ) < 0 ) || ( yy_n >= YYLAST ) )
			goto yydefault;
		if ( yychk[ yy_n = yyact[ yy_n ] ] == yychar )	/*valid shift*/
		{
			yychar = -1;
			yyval = yylval;
			yy_state = yy_n;
			if ( yyerrflag > 0 )
				yyerrflag--;
			goto yy_stack;
		}

	yydefault:
		if ( ( yy_n = yydef[ yy_state ] ) == -2 )
		{
#if YYDEBUG
			yytmp = yychar < 0;
#endif
			if ( ( yychar < 0 ) && ( ( yychar = YYLEX() ) < 0 ) )
				yychar = 0;		/* reached EOF */
#if YYDEBUG
			if ( yydebug && yytmp )
			{
				register int yy_i;

				printf( "Received token " );
				if ( yychar == 0 )
					printf( "end-of-file\n" );
				else if ( yychar < 0 )
					printf( "-none-\n" );
				else
				{
					for ( yy_i = 0;
						yytoks[yy_i].t_val >= 0;
						yy_i++ )
					{
						if ( yytoks[yy_i].t_val
							== yychar )
						{
							break;
						}
					}
					printf( "%s\n", yytoks[yy_i].t_name );
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register YYCONST int *yyxi = yyexca;

				while ( ( *yyxi != -1 ) ||
					( yyxi[1] != yy_state ) )
				{
					yyxi += 2;
				}
				while ( ( *(yyxi += 2) >= 0 ) &&
					( *yyxi != yychar ) )
					;
				if ( ( yy_n = yyxi[1] ) < 0 )
					YYACCEPT;
			}
		}

		/*
		** check for syntax error
		*/
		if ( yy_n == 0 )	/* have an error */
		{
			/* no worry about speed here! */
			switch ( yyerrflag )
			{
			case 0:		/* new error */
				yyerror( "syntax error" );
				goto skip_init;
			yyerrlab:
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				yy_pv = yypv;
				yy_ps = yyps;
				yy_state = yystate;
			skip_init:
				yynerrs++;
				/* FALLTHRU */
			case 1:
			case 2:		/* incompletely recovered error */
					/* try again... */
				yyerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( yy_ps >= yys )
				{
					yy_n = yypact[ *yy_ps ] + YYERRCODE;
					if ( yy_n >= 0 && yy_n < YYLAST &&
						yychk[yyact[yy_n]] == YYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						yy_state = yyact[ yy_n ];
						goto yy_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( yydebug )
						printf( _POP_, *yy_ps,
							yy_ps[-1] );
#	undef _POP_
#endif
					yy_ps--;
					yy_pv--;
				}
				/*
				** there is no state on stack with "error" as
				** a valid shift.  give up.
				*/
				YYABORT;
			case 3:		/* no shift yet; eat a token */
#if YYDEBUG
				/*
				** if debugging, look up token in list of
				** pairs.  0 and negative shouldn't occur,
				** but since timing doesn't matter when
				** debugging, it doesn't hurt to leave the
				** tests here.
				*/
				if ( yydebug )
				{
					register int yy_i;

					printf( "Error recovery discards " );
					if ( yychar == 0 )
						printf( "token end-of-file\n" );
					else if ( yychar < 0 )
						printf( "token -none-\n" );
					else
					{
						for ( yy_i = 0;
							yytoks[yy_i].t_val >= 0;
							yy_i++ )
						{
							if ( yytoks[yy_i].t_val
								== yychar )
							{
								break;
							}
						}
						printf( "token %s\n",
							yytoks[yy_i].t_name );
					}
				}
#endif /* YYDEBUG */
				if ( yychar == 0 )	/* reached EOF. quit */
					YYABORT;
				yychar = -1;
				goto yy_newstate;
			}
		}/* end if ( yy_n == 0 ) */
		/*
		** reduction by production yy_n
		** put stack tops, etc. so things right after switch
		*/
#if YYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( yydebug )
			printf( "Reduce by (%d) \"%s\"\n",
				yy_n, yyreds[ yy_n ] );
#endif
		yytmp = yy_n;			/* value to switch over */
		yypvt = yy_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using yy_state here as temporary
		** register variable, but why not, if it works...
		** If yyr2[ yy_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto yy_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int yy_len = yyr2[ yy_n ];

			if ( !( yy_len & 01 ) )
			{
				yy_len >>= 1;
				yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
				yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
					*( yy_ps -= yy_len ) + 1;
				if ( yy_state >= YYLAST ||
					yychk[ yy_state =
					yyact[ yy_state ] ] != -yy_n )
				{
					yy_state = yyact[ yypgo[ yy_n ] ];
				}
				goto yy_stack;
			}
			yy_len >>= 1;
			yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
			yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
				*( yy_ps -= yy_len ) + 1;
			if ( yy_state >= YYLAST ||
				yychk[ yy_state = yyact[ yy_state ] ] != -yy_n )
			{
				yy_state = yyact[ yypgo[ yy_n ] ];
			}
		}
					/* save until reenter driver code */
		yystate = yy_state;
		yyps = yy_ps;
		yypv = yy_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/
	switch( yytmp )
	{
		
case 1:
# line 96 "cpy.y"
{return(yypvt[-1]);} break;
case 2:
# line 100 "cpy.y"
{yyval = yypvt[-2] * yypvt[-0];} break;
case 3:
# line 102 "cpy.y"
{
			if (yypvt[-0] == 0) {
				yyerror("division by zero");
				yyval = 0;
			} else if (yypvt[-2] == INT_MIN && yypvt[-0] == -1) {
				yyerror("division overflow");
				yyval = INT_MIN;
			} else {
				yyval = yypvt[-2] / yypvt[-0];
			}
		} break;
case 4:
# line 114 "cpy.y"
{
			if (yypvt[-0] == 0) {
				yyerror("division by zero");
				yyval = 0;
			} else if (yypvt[-2] == INT_MIN && yypvt[-0] == -1) {
				yyerror("division overflow");
				yyval = INT_MIN;
			} else {
				yyval = yypvt[-2] % yypvt[-0];
			}
		} break;
case 5:
# line 126 "cpy.y"
{yyval = yypvt[-2] + yypvt[-0];} break;
case 6:
# line 128 "cpy.y"
{yyval = yypvt[-2] - yypvt[-0];} break;
case 7:
# line 130 "cpy.y"
{yyval = yypvt[-2] << yypvt[-0];} break;
case 8:
# line 132 "cpy.y"
{yyval = yypvt[-2] >> yypvt[-0];} break;
case 9:
# line 134 "cpy.y"
{yyval = yypvt[-2] < yypvt[-0];} break;
case 10:
# line 136 "cpy.y"
{yyval = yypvt[-2] > yypvt[-0];} break;
case 11:
# line 138 "cpy.y"
{yyval = yypvt[-2] <= yypvt[-0];} break;
case 12:
# line 140 "cpy.y"
{yyval = yypvt[-2] >= yypvt[-0];} break;
case 13:
# line 142 "cpy.y"
{yyval = yypvt[-2] == yypvt[-0];} break;
case 14:
# line 144 "cpy.y"
{yyval = yypvt[-2] != yypvt[-0];} break;
case 15:
# line 146 "cpy.y"
{yyval = yypvt[-2] & yypvt[-0];} break;
case 16:
# line 148 "cpy.y"
{yyval = yypvt[-2] ^ yypvt[-0];} break;
case 17:
# line 150 "cpy.y"
{yyval = yypvt[-2] | yypvt[-0];} break;
case 18:
# line 152 "cpy.y"
{yyval = yypvt[-2] && yypvt[-0];} break;
case 19:
# line 154 "cpy.y"
{yyval = yypvt[-2] || yypvt[-0];} break;
case 20:
# line 156 "cpy.y"
{yyval = yypvt[-4] ? yypvt[-2] : yypvt[-0];} break;
case 21:
# line 158 "cpy.y"
{yyval = yypvt[-0];} break;
case 22:
# line 160 "cpy.y"
{yyval = yypvt[-0];} break;
case 23:
# line 163 "cpy.y"
{yyval = -yypvt[-0];} break;
case 24:
# line 165 "cpy.y"
{yyval = !yypvt[-0];} break;
case 25:
# line 167 "cpy.y"
{yyval = ~yypvt[-0];} break;
case 26:
# line 169 "cpy.y"
{yyval = yypvt[-1];} break;
case 27:
# line 171 "cpy.y"
{yyval = yypvt[-1];} break;
case 28:
# line 173 "cpy.y"
{yyval = yypvt[-0];} break;
case 29:
# line 175 "cpy.y"
{yyval = yypvt[-0];} break;
# line	556 "/usr/share/lib/ccs/yaccpar"
	}
	goto yystack;		/* reset registers in driver code */
}
