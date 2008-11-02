#ident @(#)Makefile	1.12 08/10/26 
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

CFILES=		skel.c \
		io.c cd_misc.c scsi_cdr.c \
		scsi_scan.c \
		misc.c

HFILES=		cdrecord.h iodefs.h scsi_scan.h scsimmc.h

LIBS=		-lscgcmd -lrscg -lscg $(LIB_VOLMGT) -lcdrdeflt -ldeflt -lschily $(SCSILIB) $(LIB_SOCKET)

skel.c: iodefs.h
iodefs.h scsi_scan.c io.c scsi_cdr.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
$(ALLTARGETS): iodefs.h scsi_scan.c io.c scsi_cdr.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

