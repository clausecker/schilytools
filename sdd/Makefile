#ident @(#)Makefile	1.14 13/04/21 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		sdd
CPPOPTS +=	-DUSE_REMOTE
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		sdd.c
HFILES=		
#LIBS=		-lunos
#
# LIB_CAP is needed for Linux capability support in librmt.
#
LIBS=		-lmdigest -lrmt -lschily $(LIB_SOCKET) $(LIB_CAP)
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################

count:		$(CFILES) $(HFILES)
		count $r1

