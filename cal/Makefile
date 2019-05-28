#ident "@(#)Makefile	1.3 19/05/26  Copyright (c) 2019 J. Schilling"
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
					# Calendar
TARGET=		cal

CPPOPTS +=	-DVSHNAME='"bosh"'	# Shell name variant (bosh/osh/pbosh)

CPPOPTS +=	-DSCHILY_INCLUDES	# Tell the code to use schily include files
CPPOPTS +=	-DUSE_NLS		# Enable NLS support in include/schily/*.h
#CPPOPTS +=	-DNO_LOCALE		# Don't use setlocale()

CPPOPTS +=	-DTEXT_DOMAIN='"SUNW_OST_OSCMD"'	# gettext()

CFILES=		cal.c

LIBS=		-lschily $(LIB_INTL)
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
include		$(SRCROOT)/$(RULESDIR)/rules.tst
###########################################################################
count: $(HFILES) $(CFILES) 
	count $r1


