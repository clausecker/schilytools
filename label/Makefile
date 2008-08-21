#ident @(#)label.mk	1.4 08/08/15 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

.SEARCHLIST:	. $(ARCHDIR) dat $(ARCHDIR)
VPATH=		.:dat:$(ARCHDIR)

INSDIR=		bin
TARGET=		label
CPPOPTS +=	-I../sformat
CPPOPTS +=	-I../libscg
CPPOPTS +=	-DFMT
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		label.c makelabel.c io.c \
		check_part.c

HFILES=		fmt.h
HFILES_TAG=	scsireg.h

LIBS=		-lschily

makelabel.c io.c check_part.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
all: makelabel.c io.c check_part.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count: $(CFILES) $(HFILES)
	count $r1

