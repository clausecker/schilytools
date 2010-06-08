#ident "@(#)ustarman.mk	1.1 10/05/13 "
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	ustar
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	ustar.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
