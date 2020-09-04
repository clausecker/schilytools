#ident "@(#)Makefile	1.8 20/08/30 "
###########################################################################
# Sample makefile for sub-makes in one directory
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MK_FILES=	sccs.mk admin.mk comb.mk delta.mk get.mk help.mk \
		prs.mk prt.mk rmchg.mk sccsdiff.mk unget.mk val.mk vc.mk what.mk \
		sccscvt.mk sccslog.mk rcs2sccs.mk \
		bdiff.mk diff.mk diffh.mk fsdiff.mk

XMK_FILE=	Makefile.man

fsdiff.c:
	@echo "	==> MAKING SYMLINKS in ."; sh ./MKLINKS
$(ALLTARGETS): fsdiff.c
$(SRCROOT)/$(RULESDIR)/rules.mks: fsdiff.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.mks
###########################################################################
