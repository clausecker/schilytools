#ident @(#)ungetman.mk	1.2 11/10/12 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	unget
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	unget.1

unget.1.html: sccs-unget.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
