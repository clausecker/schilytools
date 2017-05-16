#ident @(#)dmakeman.mk	1.1 17/05/04 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	dmake
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	dmake.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
