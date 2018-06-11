#	@(#)Makefile	1.4 18/05/26 Copyr 2017-2018 J. Schilling
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		strar
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT		# Use *printf() routines from libschily
CPPOPTS +=	-DUSE_NLS		# Include locale support
CPPOPTS +=	-DTEXT_DOMAIN=\"SCHILY_utils\"

CFILES=		strar.c
HFILES=
LIBS=		-lstreamar -lfind -lschily $(LIB_ICONV) $(LIB_INTL)
#
#	Wenn -lfind, dann auch  $(LIB_INTL)
#
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

