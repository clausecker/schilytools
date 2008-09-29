#ident @(#)readcd.mk	1.13 08/09/10 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		readcd
CPPOPTS +=	-I../cdrecord
CPPOPTS +=	-I../libscg
CPPOPTS +=	-I../libedc
CPPOPTS +=	-DCLONE_WRITE
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		readcd.c cd_misc.c io.c \
		scsi_scan.c \
		movesect.c \
		defaults.c \
		scsi_cdr.c \
		scsi_mmc.c \
		modes.c misc.c
HFILES=		
#LIBS=		-lschily $(LIB_SOCKET)
LIBS=		-lrscg -lscg $(LIB_VOLMGT) -ledc_ecc_dec -ledc_ecc -ldeflt -lschily $(SCSILIB) $(LIB_SOCKET)
XMK_FILE=	Mreadcd.man

cd_misc.c scsi_scan.c scsi_cdr.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
$(ALLTARGETS): cd_misc.c scsi_scan.c scsi_cdr.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

