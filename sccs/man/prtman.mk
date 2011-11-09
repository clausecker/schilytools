#ident @(#)prtman.mk	1.2 11/10/12 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	prt
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	prt.1

prt.1.html: sccs-prt.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
