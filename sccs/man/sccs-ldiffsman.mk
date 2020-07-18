#ident @(#)sccs-ldiffsman.mk	1.1 20/07/05 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sccs-ldiffs
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	sccs-ldiffs.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
