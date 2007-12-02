#ident @(#)label.mk	1.2 07/02/04 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

.SEARCHLIST:	. $(ARCHDIR) dat $(ARCHDIR)
VPATH=		.:dat:$(ARCHDIR)

INSDIR=		bin
TARGET=		label
CPPOPTS +=	-DFMT
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		label.c makelabel.c io.c \
		check_part.c

HFILES=		fmt.h scsireg.h

#LIBS=		-lunos
LIBS=		-lschily

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
