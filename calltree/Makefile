#ident @(#)Makefile   1.10 09/07/11 Copyr 1990 J. Schilling
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		calltree
#CPPOPTS +=	-Ispecincl
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		calltree.c clex.c keyw.c string.c sym.c
HFILES=		clex.h strsubs.h sym.h
LIBS=		-lschily
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:		$(CFILES) $(HFILES)
		count $r1 

bintree:	$(CFILES)
		calltree -bg $(CPPOPTS) $r1 

