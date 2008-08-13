#ident %W% %E% %Q%
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

HFILES=		fmt.h scsireg.h

LIBS=		-lschily

makelabel.c io.c check_part.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
all: makelabel.c io.c check_part.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
