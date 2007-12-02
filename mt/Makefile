#ident @(#)Makefile	1.5 07/02/04 
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
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
