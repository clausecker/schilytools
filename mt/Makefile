#ident @(#)Makefile	1.8 11/04/16 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		smt
SYMLINKS=	mt
CPPOPTS +=	-DUSE_REMOTE
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		mt.c
#HFILES=
LIBS=		-lrmt -lschily $(LIB_SOCKET)
XMK_FILE=	Makefile.man mt.mk1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

