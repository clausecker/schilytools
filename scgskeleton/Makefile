#ident @(#)Makefile	1.15 09/08/15 
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

cd_misc.c scsi_cdr.c scsi_scan.c io.c misc.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
$(ALLTARGETS): cd_misc.c scsi_cdr.c scsi_scan.c io.c misc.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

