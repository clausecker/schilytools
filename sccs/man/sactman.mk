#ident @(#)sactman.mk	1.2 11/10/12 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sact
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	sact.1

sact.1.html: sccs-sact.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
