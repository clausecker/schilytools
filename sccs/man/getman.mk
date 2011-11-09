#ident @(#)getman.mk	1.2 11/10/12 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	get
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	get.1

get.1.html: sccs-get.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
