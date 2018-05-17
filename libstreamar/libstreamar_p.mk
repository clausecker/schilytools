#ident "@(#)libstreamar_p.mk	1.2 18/05/17 "
###########################################################################
# Sample makefile for non-shared libraries
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/profiled
SUBINSDIR=	/profiled
INSDIR=		lib
TARGETLIB=	streamar
COPTS +=	$(COPTGPROF)
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	-DUSE_ICONV

include		Targets

LIBS=
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
