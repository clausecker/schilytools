#ident "@(#)sccs-rcs2sccsman.mk	1.1 11/09/30 "
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	rcs2sccs
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	rcs2sccs.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
