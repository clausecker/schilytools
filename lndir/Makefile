#	@(#)Makefile	1.2 11/08/05 Copyr 2005 J. Schilling
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		lndir
CPPOPTS +=	-DUSE_LARGEFILES
CFILES=		lndir.c
HFILES=
LIBS=		-lschily
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
