#ident "@(#)shlstreamar.mk	1.1 17/02/14 "
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

include		Targets

LIBS=		-lfind -lschily -lc
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.shl
###########################################################################
