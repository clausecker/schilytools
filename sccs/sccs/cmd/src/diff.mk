#ident @(#)diff.mk	1.5 07/05/13 
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		ccs/bin
TARGET=		diff
#SYMLINKS=	../../bin/sccs

CPPOPTS +=	-DSUN5_0
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-I../../../sgs/inc/common
CPPOPTS +=	-I../../hdr
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	-DTEXT_DOMAIN=\"SUNW_OST_OSCMD\"
CPPOPTS +=	-D_TS_ERRNO
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"

CFILES=		diff.c

HFILES=		diff.h

LIBS=		-lgetopt -lschily
#LIBS=		-lcomobj -lcassi -lmpw -lgetopt -lschily

#XMK_FILE=	Makefile.def Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
###########################################################################
