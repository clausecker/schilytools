#ident "@(#)Makefile	1.17 15/06/02 "
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		spatch
SYMLINKS=	opatch
#CPPOPTS +=	-DNO_FLEXNAMES		#define if you are on a V7fs
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DTEXT_DOMAIN=\"SCHILY_UTILS\"

CFILES=		patch.c inp.c pch.c util.c
HFILES=		common.h inp.h pch.h util.h

LIBS=		-lschily $(LIB_INTL)
XMK_FILE=	patch_sym.mk Makefile.man opatch.mk1 patch.mk1 sccspatch.mk1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

