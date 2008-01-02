#ident @(#)sccslog.mk	1.7 08/01/02 
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		ccs/bin
TARGET=		sccslog
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-I../../hdr
CFILES=		sccslog.c
HFILES=		
LIBS=		-lschily $(LIB_INTL)
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
