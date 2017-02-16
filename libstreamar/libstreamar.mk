#ident "@(#)libstreamar.mk	1.2 17/02/15 "
###########################################################################
# Sample makefile for non-shared libraries
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		lib
TARGETLIB=	streamar
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DUSE_NLS

include		Targets

LIBS=
XMK_FILE=	streamarman.mk

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

