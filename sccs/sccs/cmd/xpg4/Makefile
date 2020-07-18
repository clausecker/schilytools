#ident "@(#)Makefile	1.5 20/07/08 "
###########################################################################
# Sample makefile for sub-makes in one directory
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MK_FILES=	sccs.mk get.mk

XMK_FILE=	Makefile.man

sccs.c:
	@echo "	==> MAKING SYMLINKS in ."; sh ./MKLINKS
$(ALLTARGETS): sccs.c
$(SRCROOT)/$(RULESDIR)/rules.mks: sccs.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.mks
###########################################################################
