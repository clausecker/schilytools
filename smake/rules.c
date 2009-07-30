/* @(#)rules.c	1.18 09/07/08 Copyright 1987-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)rules.c	1.18 09/07/08 Copyright 1987-2009 J. Schilling";
#endif
/*
 *	Copyright (c) 1987-2009 J. Schilling
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

#include <schily/stdio.h>
#include <schily/standard.h>
#include "make.h"

/*
 * Default rules complied into make.
 *
 * Should rather read in a setup file...
 */
char implicit_rules[] =


#if	defined(unix) || defined(IS_UNIX) || defined(IS_GCC_WIN32) || \
	defined(__EMX__) || defined(__BEOS__) || defined(__HAIKU__) || \
	defined(__DJGPP__)
#	define _OS_

"\
FC=	f77\n\
RC=	f77\n\
PC=	pc\n\
AS=	as\n\
CC=	cc\n\
LD=	ld\n\
LEX= 	lex\n\
MAKE=	smake\n\
YACCR=	yacc -r\n\
YACC=	yacc\n\
ROFF=	nroff\n\
ASFLAGS=\n\
FFLAGS=\n\
LDFLAGS=\n\
RFLAGS=	-ms\n\
YFLAGS=\n\
.o: .c .s .l\n\
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<\n\
	$(AS) -o $*.o $(ASFLAGS) $<\n\
	$(LEX) $(LFLAGS) $<;$(CC) -c $(CFLAGS) lex.yy.c;rm lex.yy.c;mv lex.yy.o $@\n\
.c: .y\n\
	$(YACC) $(YFLAGS) $<;mv y.tab.c $@\n\
\"\": .o .sc\n\
	$(CC) -o $* $<\n\
	$(ROFF) $(RFLAGS) $< > $@\n";
#endif

#ifdef	tos
#	define _OS_

"\
FC=	f77\n\
RC=	f77\n\
PC=	pc\n\
AS=	as\n\
CC=	cc\n\
LD=	ld\n\
LEX= 	lex\n\
MAKE=	smake\n\
YACCR=	yacc -r\n\
YACC=	yacc\n\
ROFF=	nroff\n\
ASFLAGS=\n\
FFLAGS=\n\
LDFLAGS=\n\
RFLAGS=	-ms\n\
.o: .c .s .l\n\
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<\n\
	$(AS) -o $*.o $(ASFLAGS) $<\n\
	$(LEX) $(LFLAGS) $<;$(CC) -c $(CFLAGS) lexyy.c;rm lexyy.c;mv lexyy.o $@\n\
.c: .y\n\
	$(YACC) $(YFLAGS) $<;mv ytab.c $@\n\
\"\": .sc\n\
	$(ROFF) $(RFLAGS) $< > $@\n\
.prg .tos .ttp: .o\n\
	$(CC) -o $* $<\n";
#endif

#ifndef _OS_
"";
#endif

