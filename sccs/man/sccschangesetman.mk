#ident "@(#)sccschangesetman.mk	1.2 15/01/19 "
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sccschangeset
MANSECT=	$(MANSECT_FILEFORM)
MANSUFFIX=	$(MANSUFF_FILEFORM)
MANFILE=	sccschangeset.5

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
