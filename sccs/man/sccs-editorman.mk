#ident @(#)sccs-editorman.mk	1.1 20/06/29 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	sccs-editor
MANSECT=	$(MANSECT_CMD)
MANSUFFIX=	$(MANSUFF_CMD)
MANFILE=	sccs-editor.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
