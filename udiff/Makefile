#ident @(#)Makefile	1.12 20/08/27 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		udiff
SYMLINKS=	fdiff fsdiff
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		udiff.c
HFILES=
#LIBS=		-lunos
LIBS=		-lschily $(LIB_INTL)
XMK_FILE=	Makefile.man fdiff.mk1 fsdiff.mk1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:		$(CFILES) $(HFILES)
		count $r1 

