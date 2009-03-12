#ident @(#)Makefile	1.10 09/01/14 
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
CPPOPTS +=	-I../libcdrdeflt
CPPOPTS +=	-I../libscg

CFILES=		skel.c \
		io.c cd_misc.c scsi_cdr.c \
		scsi_scan.c \
		misc.c


HFILES=		
LIBS=		-lscgcmd -lrscg -lscg $(LIB_VOLMGT) -lcdrdeflt -ldeflt -lschily $(SCSILIB) $(LIB_SOCKET)
XMK_FILE=	Makefile.man

scsi_scan.c io.c scsi_cdr.c misc.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
$(ALLTARGETS): scsi_scan.c io.c scsi_cdr.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

