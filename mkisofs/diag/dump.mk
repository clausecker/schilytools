#ident @(#)dump.mk	1.10 08/10/26 
###########################################################################
#@@C@@
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		devdump
CPPOPTS +=	-DUSE_LIBSCHILY
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DUSE_SCG
CPPOPTS +=	-I..
CPPOPTS +=	-I../../libscg
CPPOPTS +=	-I../../libscgcmd
CPPOPTS +=	-I../../libcdrdeflt
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		dump.c \
		scsi.c

LIBS=		-lscgcmd -lrscg -lscg $(LIB_VOLMGT) -lcdrdeflt -ldeflt -lschily $(SCSILIB) $(LIB_SOCKET)
XMK_FILE=	devdump_man.mk

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
