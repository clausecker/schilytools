#ident @(#)svmakeman.mk	1.1 17/04/17 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sysV-make
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	sysV-make.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
