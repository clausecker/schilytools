#ident @(#)sccs-tellman.mk	1.1 20/06/29 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sccs-tell
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	sccs-tell.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
