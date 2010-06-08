#ident @(#)Makefile	1.16 10/05/13 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		scgskeleton
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-DNO_DOIT
CPPOPTS +=	-I../libscg
CPPOPTS +=	-I../libcdrdeflt
CPPOPTS +=	-I../cdrecord
CPPOPTS +=	-I../readcd

CFILES=		skel.c \
		io.c cd_misc.c scsi_cdr.c \
		scsi_scan.c \
		misc.c

#HFILES=		cdrecord.h iodefs.h scsi_scan.h scsimmc.h

LIBS=		-lscgcmd -lrscg -lscg $(LIB_VOLMGT) -lcdrdeflt -ldeflt -lschily $(SCSILIB) $(LIB_SOCKET)
XMK_FILE=	Makefile.man

cd_misc.c scsi_cdr.c scsi_scan.c io.c misc.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
$(ALLTARGETS): cd_misc.c scsi_cdr.c scsi_scan.c io.c misc.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

