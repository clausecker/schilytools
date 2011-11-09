#ident @(#)prsman.mk	1.2 11/10/12 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	prs
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	prs.1

prs.1.html: sccs-prs.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
