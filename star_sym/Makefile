#ident @(#)star_sym.mk	1.5 07/09/22 
###########################################################################
# Sample makefile for general application programs
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		star_sym
#CPPOPTS +=	-Ispecincl
CPPOPTS +=	-DSTAR_SYM
CPPOPTS +=	-D__STAR__ 
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		star_sym.c restore.c dumpdate.c fetchdir.c
LIBS=		-lfind -lschily $(LIB_ACL_TEST)
#XMK_FILE=	Makefile.man
#XMK_FILE=	tartestman.mk

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################

