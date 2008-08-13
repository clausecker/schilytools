#ident "@(#)Makefile	1.2 08/07/31 "
###########################################################################
# Sample makefile for sub-makes in one directory
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MK_FILES=	sccs.mk delta.mk get.mk

XMK_FILE=	Makefile.man

sccs.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
all: sccs.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.mks
###########################################################################
