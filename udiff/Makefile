#ident @(#)Makefile	1.9 11/07/12 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		udiff
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		udiff.c
HFILES=
#LIBS=		-lunos
LIBS=		-lschily $(LIB_INTL)
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:		$(CFILES) $(HFILES)
		count $r1 

