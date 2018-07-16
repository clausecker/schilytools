#ident @(#)tartest.mk	1.8 18/07/10 
###########################################################################
# Sample makefile for general application programs
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		tartest
CPPOPTS +=	-I../star
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		tartest.c
LIBS=		-lschily $(LIB_INTL)
#XMK_FILE=	Makefile.man
XMK_FILE=	tartestman.mk

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################

