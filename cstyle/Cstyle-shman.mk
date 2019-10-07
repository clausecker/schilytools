#ident "@(#)Cstyle-shman.mk	1.1 09/11/19 "
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	Cstyle
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	Cstyle.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
