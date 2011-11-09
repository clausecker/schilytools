#ident @(#)deltaman.mk	1.2 11/10/12 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	delta
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	delta.1

delta.1.html: sccs-delta.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
