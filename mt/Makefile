#ident @(#)Makefile	1.10 21/08/19 
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
CPPOPTS +=	-DUSE_NLS		# Include locale support
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DINS_RBASE=\"${INS_RBASE}\"
CPPOPTS +=	-DTEXT_DOMAIN=\"SCHILY_utils\"

CFILES=		mt.c
#HFILES=
#
# LIB_CAP is needed for Linux capability support in librmt.
#
LIBS=		-lrmt -lschily $(LIB_SOCKET) $(LIB_CAP) $(LIB_INTL)
XMK_FILE=	Makefile.man mt.mk1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

