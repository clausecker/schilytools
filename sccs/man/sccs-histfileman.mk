#ident @(#)sccs-histfileman.mk	1.1 20/06/29 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sccs-histfile
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	sccs-histfile.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
