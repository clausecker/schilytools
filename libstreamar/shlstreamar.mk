#ident "@(#)shlstreamar.mk	1.2 18/05/17 "
###########################################################################
# Sample makefile for non-shared libraries
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/pic
INSDIR=		lib
TARGETLIB=	streamar
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	-DUSE_ICONV

include		Targets

LIBS=		-lfind -lschily -lc
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.shl
###########################################################################
