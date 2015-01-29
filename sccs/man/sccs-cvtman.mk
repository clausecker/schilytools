#ident "@(#)sccs-cvtman.mk	1.2 15/01/19 "
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sccs-cvt
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	sccs-cvt.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
