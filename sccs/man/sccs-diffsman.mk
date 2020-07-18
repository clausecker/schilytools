#ident @(#)sccs-diffsman.mk	1.1 20/07/05 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sccs-diffs
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	sccs-diffs.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
