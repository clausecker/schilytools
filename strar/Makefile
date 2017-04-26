#	@(#)Makefile	1.2 17/04/21 Copyr 2017 J. Schilling
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		strar
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		strar.c
HFILES=
LIBS=		-lstreamar -lfind -lschily $(LIB_INTL)
#
#	Wenn -lfind, dann auch  $(LIB_INTL)
#
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

