#ident "@(#)libstreamar_p.mk	1.1 17/02/14 "
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

include		Targets

LIBS=
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
