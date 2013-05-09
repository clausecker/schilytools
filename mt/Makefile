#ident @(#)Makefile	1.9 13/04/21 
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
#
# LIB_CAP is needed for Linux capability support in librmt.
#
LIBS=		-lrmt -lschily $(LIB_SOCKET) $(LIB_CAP)
XMK_FILE=	Makefile.man mt.mk1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

