#ident @(#)Makefile	1.2 08/01/09 Copyright 2000 J. Schilling
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		termcap
CPPOPTS +=	-DUSE_LARGEFILES

CFILES=		cap.c

HFILES=		
LIBS=		-lxtermcap -lschily
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################

count:		$(CFILES) $(HFILES)
		count $r1

