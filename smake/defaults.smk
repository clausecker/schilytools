# @(#)defaults.smk	1.4 05/06/11 Copyright 1987-2005 J. Schilling
#

#
# Special SCCS target
# SCCS is not yet supported
#
#.SCCS_GET:
#	sccs $(SCCSFLAGS) get $(SCCSGETFLAGS) $@

#
# Special Suffix target
# We start without SCCS support
#
#SUFFIXES= .o .c .y .l .a .sh .f .c~ .y~ .l~ .sh~ .f~

SUFFIXES= .o .c .y .l .s .a .sh .f
.SUFFIXES: $(SUFFIXES)


AR=		ar
ARFLAGS=	-rv
AS=		as
ASFLAGS=
CC=		cc
CFLAGS=		-O
CPPFLAGS=
FC=		f77
FFLAGS=
RC=		f77
RCFLAGS=
GET=		get
GFLAGS=
LD=		ld
LDFLAGS=
LDLIBS=
LEX=		lex
LFLAGS=
MAKE=		smake
PC=		pc
PFLAGS=
RM=		rm -f
ROFF=		nroff
RFLAGS=		-ms
SCCSFLAGS=
SCCSGETFLAGS=	-s
YACC=		yacc
YFLAGS=

#
# Simple Suffix Rules
# We do not use them in case there are POSIX sufix rules
#
#.o: .c .s .l
#	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<
#	$(AS) -o $*.o $(ASFLAGS) $<
#	$(LEX) $(LFLAGS) $<;$(CC) -c $(CFLAGS) lex.yy.c;rm lex.yy.c;mv lex.yy.o $@
#.c: .y
#	$(YACC) $(YFLAGS) $<;mv y.tab.c $@
#"": .o .sc
#	$(CC) -o $* $<
#	$(ROFF) $(RFLAGS) $< > $@

#
# POSIX Single Suffix Rules
#

#	C language
.c:
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(LDLIBS) -o $@ $<

#	Fortran language
.f:
	$(FC) $(FFLAGS) $(LDFLAGS) $(LDLIBS) -o $@ $<

#	Shell language
.sh:
	$(RM) $@
	cp $< $@
	chmod a+x $@

#	Yacc language
.y:
	$(YACC) $(YFLAGS) $<
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o $@ y.tab.c
	$(RM) y.tab.c

#	Lex language
.l:
	$(LEX) $(LFLAGS) $<
	$(CC) $(CFLAGS) $(LDFLAGS) -ll $(LDLIBS) -o $@ lex.yy.c
	$(RM) lex.yy.c

#
# POSIX Double Suffix Rules
#

#	C language
.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<

#	Fortran language
.f.o:
	$(FC) $(FFLAGS) $(CPPFLAGS) -c $<

#	Yacc language
.y.o:
	$(YACC) $(YFLAGS) $<
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c y.tab.c
	$(RM) y.tab.c

#	Lex language
.l.o:
	$(LEX) $(LFLAGS) $<
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c lex.yy.c
	$(RM) lex.yy.c

#	Yacc language to C
.y.c:
	$(YACC) $(YFLAGS) $<
	mv y.tab.c $@

#	Lex language to C
.l.c:
	$(LEX) $(LFLAGS) $<
	mv lex.yy.c $@

#	C language to archive
.c.a:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $<
	$(AR) $(ARFLAGS) $@ $*.o
	$(RM) $*.o

#	Fortran language to archive
.f.a:
	$(FC) $(FFLAGS) $(CPPFLAGS) -c $<
	$(AR) $(ARFLAGS) $@ $*.o
	$(RM) $*.o

#	Assembly language
.s.o:
	$(AS) $(ASFLAGS) -o $@ $<
