#ident @(#)Makefile	1.6 07/02/04 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		calc
#CPPOPTS +=	-Ispecincl
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		calc.c
#HFILES=		
LIBS=		-lschily
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:		$(CFILES) $(HFILES)
		count $r1 

