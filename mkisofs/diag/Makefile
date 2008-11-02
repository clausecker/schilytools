#ident "@(#)Makefile	1.9 08/10/26 "
###########################################################################
#@@C@@
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MK_FILES=	dump.mk isodump.mk isoinfo.mk isovfy.mk isodebug.mk
#XMK_FILE=	Makefile.man

scsi.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
$(ALLTARGETS): scsi.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.mks
###########################################################################
