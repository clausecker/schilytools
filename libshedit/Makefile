#ident "@(#)Makefile	1.21 20/07/08 "
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MK_FILES= libshedit.mk libshedit_p.mk $(NO_SHL)shlshedit.mk

bsh.h:
	@echo "	==> MAKING SYMLINKS in ."; sh ./MKLINKS
$(ALLTARGETS): bsh.h
$(SRCROOT)/$(RULESDIR)/rules.mks: bsh.h

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.mks
###########################################################################
