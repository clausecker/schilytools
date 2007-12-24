#ident @(#)Makefile	1.5 07/12/16 
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

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:		$(CFILES) $(HFILES)
		count $r1 

