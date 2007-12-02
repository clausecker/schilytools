#ident @(#)tartest.mk	1.6 07/02/17 
###########################################################################
# Sample makefile for general application programs
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		tartest
#CPPOPTS +=	-Ispecincl
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		tartest.c
LIBS=		-lschily
#XMK_FILE=	Makefile.man
XMK_FILE=	tartestman.mk

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################

IFDEF=	-UEVALVERS
XRELFILES=	Makefile tartestman.mk tartest.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.rel
###########################################################################

MAKE_LICENSE=MKCDDL
