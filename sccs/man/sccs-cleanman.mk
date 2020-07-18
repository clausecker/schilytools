#ident @(#)sccs-cleanman.mk	1.1 20/07/05 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sccs-clean
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	sccs-clean.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
