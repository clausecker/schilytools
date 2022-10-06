#ident @(#)starformatman.mk	1.1 05/08/28 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	star
MANSECT=	$(MANSECT_FILEFORM)
MANSUFFIX=	$(MANSUFF_FILEFORM)
MANFILE=	star.5

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
