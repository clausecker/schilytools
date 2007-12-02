#ident @(#)Makefile	1.7 07/04/24 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		scgcheck
CPPOPTS +=	-I../cdrecord
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		scgcheck.c sense.c dmaresid.c \
		scsi_scan.c scsi_cdr.c cd_misc.c modes.c

HFILES=		scgcheck.h version.h
#LIBS=		-lschily $(LIB_SOCKET)
LIBS=		-lrscg -lscg $(LIB_VOLMGT) -lschily $(SCSILIB) $(LIB_SOCKET)
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
