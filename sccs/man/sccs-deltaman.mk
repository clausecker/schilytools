#ident @(#)sccs-deltaman.mk	1.1 07/02/10 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sccs-delta
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	sccs-delta.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
