#ident @(#)tartestman.mk	1.1 04/09/27 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	tartest
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	tartest.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
