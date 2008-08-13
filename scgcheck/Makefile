#ident @(#)Makefile	1.9 08/08/01 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		scgcheck
CPPOPTS +=	-I../libscg
CPPOPTS +=	-I../cdrecord
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		scgcheck.c sense.c dmaresid.c \
		scsi_scan.c scsi_cdr.c cd_misc.c modes.c

HFILES=		scgcheck.h version.h
#LIBS=		-lschily $(LIB_SOCKET)
LIBS=		-lrscg -lscg $(LIB_VOLMGT) -lschily $(SCSILIB) $(LIB_SOCKET)
XMK_FILE=	Makefile.man

scsi_scan.c scsi_cdr.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
all: scsi_scan.c scsi_cdr.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
