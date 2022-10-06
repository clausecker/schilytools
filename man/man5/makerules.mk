#ident %W% %E% %Q%
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	makerules
MANSECT=	$(MANSECT_FILEFORM)
MANSUFFIX=	$(MANSUFF_FILEFORM)
MANFILE=	makerules.5

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
