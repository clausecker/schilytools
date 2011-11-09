#ident @(#)combman.mk	1.2 11/10/12 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	comb
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	comb.1

comb.1.html: sccs-comb.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
