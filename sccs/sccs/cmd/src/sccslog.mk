#ident @(#)sccslog.mk	1.6 07/12/11 
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		ccs/bin
TARGET=		sccslog
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-I../../hdr
CFILES=		sccslog.c
HFILES=		
LIBS=		-lschily
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
