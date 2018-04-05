#ident "@(#)sccscvt.mk	1.4 18/04/04 "
###########################################################################
# Sample makefile for general application programs
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

PREINSDIR=	$(SCCS_BIN_PRE)
#SCCS_BIN_PRE=	sccs/
SCCS_HELP_PRE=	ccs/
SCCS_BIN_PRE=	ccs/
INSDIR=		bin
TARGET=		sccscvt

CPPOPTS +=	-DSUN5_0
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-I../../../sgs/inc/common
CPPOPTS +=	-I../../hdr
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DSCCS_HELP_PRE=\"${SCCS_HELP_PRE}\"
CPPOPTS +=	-DSCCS_BIN_PRE=\"${SCCS_BIN_PRE}\"
CPPOPTS +=	-DSCCS_FATALHELP		# auto call to help

CFILES=		sccscvt.c

LIBS=		-lcomobj -lcassi -lmpw -lschily $(LIB_INTL)
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
