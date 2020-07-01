#ident @(#)sccs-editman.mk	1.1 20/06/28 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sccs-edit
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	sccs-edit.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
