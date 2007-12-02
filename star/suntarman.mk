#ident @(#)suntarman.mk	1.1 04/09/26 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	suntar
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	suntar.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
