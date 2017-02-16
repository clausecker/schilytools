#ident "@(#)streamarman.mk	1.1 17/02/15 "
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	streamarchive
MANSECT=	$(MANSECT_FILEFORM)
MANSUFFIX=	$(MANSUFF_FILEFORM)
MANFILE=	streamarchive.4

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
