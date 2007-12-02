#ident @(#)Makefile	1.13 07/02/04 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		sfind
CPPOPTS +=	-D__FIND__
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DUSE_ACL
CPPOPTS +=	-DUSE_XATTR
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	-I../libfind
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		find.c
LIBS=		-lfind -lschily $(LIB_ACL_TEST) $(LIB_INTL)
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:		$(CFILES) $(HFILES)
		count $r1 

xtext:		$(CFILES)
		gxgettext $r1

