#ident "@(#)rcs2sccsman.mk	1.2 11/10/12 "
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sccs-rcs2sccs
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	sccs-rcs2sccs.1

rcs2sccs.1.html: sccs-rcs2sccs.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
