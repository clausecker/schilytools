#ident @(#)Makefile	1.6 08/08/01 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		btcflash
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-I../cdrecord
CPPOPTS +=	-I../libscg

CFILES=		skel.c \
		io.c cd_misc.c scsi_cdr.c \
		scsi_scan.c \
		modes.c misc.c \
		defaults.c

HFILES=		
LIBS=		-lrscg -lscg $(LIB_VOLMGT) -ldeflt -lschily $(SCSILIB) $(LIB_SOCKET)
XMK_FILE=	Makefile.man

scsi_scan.c io.c scsi_cdr.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
all: scsi_scan.c io.c scsi_cdr.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

