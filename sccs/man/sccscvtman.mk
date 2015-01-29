#ident @(#)sccscvtman.mk	1.1 15/01/19 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sccscvt
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	sccscvt.1

sccscvt.1.html: sccs-cvt.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
