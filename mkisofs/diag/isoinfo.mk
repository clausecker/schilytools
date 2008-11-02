#ident @(#)isoinfo.mk	1.13 08/10/26 
###########################################################################
#@@C@@
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		isoinfo
CPPOPTS +=	-DUSE_LIBSCHILY
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DUSE_SCG
CPPOPTS +=	-I..
CPPOPTS +=	-I../../libscg
CPPOPTS +=	-I../../libscgcmd
CPPOPTS +=	-I../../libcdrdeflt
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-DUSE_ICONV
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"

CFILES=		isoinfo.c \
		scsi.c

LIBS=		-lsiconv -lscgcmd -lrscg -lscg $(LIB_VOLMGT) -lcdrdeflt -ldeflt -lschily \
			$(SCSILIB) $(LIB_SOCKET) $(LIB_ICONV) $(LIB_INTL)

XMK_FILE=	isoinfo_man.mk

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
