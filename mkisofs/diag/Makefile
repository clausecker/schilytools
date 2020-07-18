#ident "@(#)Makefile	1.11 20/07/08 "
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
	@echo "	==> MAKING SYMLINKS in ."; sh ./MKLINKS
$(ALLTARGETS): scsi.c
$(SRCROOT)/$(RULESDIR)/rules.mks: scsi.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.mks
###########################################################################

CFILES: dump.c isodebug.c isodump.c isoinfo.c isovfy.c

xtxt:	$(CFILES)
	gxgettext --no-wrap -k_ -k__ $(CFILES)
