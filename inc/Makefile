#ident "@(#)Makefile	1.2 08/07/31 "
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MK_FILES= align_test.mk avoffset.mk

align_test.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
all: align_test.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.mks
###########################################################################
