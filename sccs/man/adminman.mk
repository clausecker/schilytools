#ident @(#)adminman.mk	1.2 11/10/12 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	admin
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	admin.1

admin.1.html: sccs-admin.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
