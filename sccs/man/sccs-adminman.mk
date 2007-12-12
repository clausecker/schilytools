#ident @(#)sccs-adminman.mk	1.1 07/02/10 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sccs-admin
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	sccs-admin.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
