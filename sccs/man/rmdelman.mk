#ident @(#)rmdelman.mk	1.2 11/10/12 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	rmdel
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	rmdel.1

rmdel.1.html: sccs-rmdel.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
