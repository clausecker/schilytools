#ident @(#)Makefile	1.15 21/08/18 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		sdd
CPPOPTS +=	-DUSE_REMOTE
CPPOPTS +=	-DUSE_SSH
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-DUSE_NLS		# Include locale support
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DINS_RBASE=\"${INS_RBASE}\"
CPPOPTS +=	-DTEXT_DOMAIN=\"SCHILY_utils\"

CFILES=		sdd.c
HFILES=		
#LIBS=		-lunos
#
# LIB_CAP is needed for Linux capability support in librmt.
#
LIBS=		-lmdigest -lrmt -lschily $(LIB_SOCKET) $(LIB_CAP) $(LIB_INTL)
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################

count:		$(CFILES) $(HFILES)
		count $r1

