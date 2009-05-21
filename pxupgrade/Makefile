#ident @(#)Makefile	1.5 08/08/01 Copyr 1995 J. Schilling
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		pxupgrade
#CPPOPTS +=	-DFIFO -DDRV_DVD -DUSE_PG -DAUINFO
#CPPOPTS +=	-DASPI
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-I../libscg

CFILES=		pxupgrade.c pcmd.c
HFILES=		scsicmds.h

#LIBS=		-lschily $(LIB_SOCKET)
LIBS=		-lscg $(LIB_VOLMGT) -ldeflt -lschily $(SCSILIB)
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
