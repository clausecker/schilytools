#ident @(#)vc.mk	1.3 11/04/30 
###########################################################################
# Sample makefile for general application programs
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

#INSDIR=	sccs
INSDIR=		ccs/bin
TARGET=		vc

CPPOPTS +=	-DSUN5_0
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-I../../../sgs/inc/common
CPPOPTS +=	-I../../hdr
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"

CFILES=		vc.c

LIBS=		-lcomobj -lcassi -lmpw -lschily $(LIB_INTL)
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
