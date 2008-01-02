#ident @(#)diffh.mk	1.5 08/01/02 
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		ccs/lib
TARGET=		diffh
#SYMLINKS=	../../bin/sccs

CPPOPTS +=	-DSUN5_0
#CPPOPTS +=	-D_FILE_OFFSET_BITS=64
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-I../../../sgs/inc/common
CPPOPTS +=	-I../../hdr
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-DTEXT_DOMAIN=\"SUNW_OST_OSCMD\"
CPPOPTS +=	-D_TS_ERRNO
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"

CFILES=		diffh.c

#HFILES=		make.h

LIBS=		-lgetopt -lschily $(LIB_INTL)

#XMK_FILE=	Makefile.def Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
###########################################################################
