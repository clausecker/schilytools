#ident @(#)sccs-initman.mk	1.1 20/05/18 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sccs-init
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	sccs-init.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
