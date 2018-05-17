#ident "@(#)libstreamar.mk	1.3 18/05/17 "
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
CPPOPTS +=	-DUSE_ICONV

include		Targets

LIBS=
XMK_FILE=	streamarman.mk

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

