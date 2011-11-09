#ident @(#)cdcman.mk	1.2 11/10/12 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	cdc
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	cdc.1

cdc.1.html: sccs-cdc.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
