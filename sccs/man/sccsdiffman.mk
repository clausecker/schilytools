#ident @(#)sccsdiffman.mk	1.2 11/10/12 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sccsdiff
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	sccsdiff.1

sccsdiff.1.html: sccs-sccsdiff.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
