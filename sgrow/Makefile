#ident @(#)Makefile	1.6 10/05/12 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		sgrow
#CPPOPTS +=	
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		sgrow.c
#HFILES=
#LIBS=		-lunos
LIBS=		-lschily
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:		$(CFILES) $(HFILES)
		count $r1 

