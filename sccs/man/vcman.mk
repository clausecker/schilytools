#ident "@(#)vcman.mk	1.1 11/07/06 "
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	vc
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	vc.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
