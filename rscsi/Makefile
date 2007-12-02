#ident @(#)Makefile	1.5 07/06/13 
###########################################################################
#include		$(MAKE_M_ARCH).def
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		sbin
TARGET=		rscsi
INSMODE=	4711
INSUSR=		root
#CPPOPTS +=	-DUSE_REMOTE
CFILES=		rscsi.c
#HFILES=
LIBS=		-lscg $(LIB_VOLMGT) -ldeflt -lschily $(SCSILIB) $(LIB_SOCKET)
XMK_FILE=	Makefile.dfl Makefile.doc
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
