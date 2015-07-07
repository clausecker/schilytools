#ident "@(#)diffman.mk	1.1 11/05/25 "
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	diff
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	diff.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
