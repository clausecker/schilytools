#ident @(#)bdiff.mk	1.6 18/04/04 
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

PREINSDIR=	$(SCCS_BIN_PRE)
SCCS_HELP_PRE=	ccs/
SCCS_BIN_PRE=	ccs/
INSDIR=		bin
TARGET=		bdiff

CPPOPTS +=	-DSUN5_0
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-I../../../sgs/inc/common
CPPOPTS +=	-I../../hdr
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DSCCS_HELP_PRE=\"${SCCS_HELP_PRE}\"
CPPOPTS +=	-DSCCS_BIN_PRE=\"${SCCS_BIN_PRE}\"

CFILES=		bdiff.c

#HFILES=		make.h

LIBS=		-lgetopt -lschily $(LIB_INTL)
#LIBS=		-lcomobj -lcassi -lmpw -lgetopt -lschily

#XMK_FILE=	Makefile.def Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
###########################################################################
