#ident @(#)sccslogman.mk	1.1 15/01/19 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sccslog
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	sccslog.1

sccslog.1.html: sccs-log.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
