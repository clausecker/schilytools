#ident @(#)Makefile	1.5 11/04/15 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		sbin
TARGET=		rmt
CPPOPTS +=	-DUSE_REMOTE
CPPOPTS +=	-DUSE_LARGEFILES
CFILES=		rmt.c
#HFILES=
LIBS=		-ldeflt -lschily $(LIB_SOCKET)
XMK_FILE=	Makefile.man Makefile.dfl Makefile.doc

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

