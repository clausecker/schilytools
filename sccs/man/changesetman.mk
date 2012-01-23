#ident "@(#)changesetman.mk	1.1 11/12/09 "
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	changeset
MANSECT=	$(MANSECT_FILEFORM)
MANSUFFIX=	$(MANSUFF_FILEFORM)
MANFILE=	changeset.4

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
