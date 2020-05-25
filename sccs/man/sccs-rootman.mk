#ident @(#)sccs-rootman.mk	1.1 20/05/21 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sccs-root
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	sccs-root.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
