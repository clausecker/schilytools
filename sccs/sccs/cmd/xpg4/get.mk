#ident @(#)get.mk	1.5 18/04/04 
###########################################################################
# Sample makefile for general application programs
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SCCS_HELP_PRE=	ccs/
SCCS_BIN_PRE=	ccs/
#INSDIR=	sccs
INSDIR=		xpg4/bin
TARGET=		get

CPPOPTS +=	-DSUN5_0
CPPOPTS +=	-DXPG4
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-I../../../sgs/inc/common
CPPOPTS +=	-I../../hdr
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DSCCS_HELP_PRE=\"${SCCS_HELP_PRE}\"
CPPOPTS +=	-DSCCS_BIN_PRE=\"${SCCS_BIN_PRE}\"

CFILES=		get.c

LIBS=		-lcomobj -lcassi -lmpw -lgetopt -lschily $(LIB_INTL)
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
