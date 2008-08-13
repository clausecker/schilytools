#ident @(#)shlrscg.mk	1.4 08/08/03 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/pic
#.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
#VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	rscg
CPPOPTS +=	-I../libscg
CPPOPTS +=	-DUSE_PG
CPPOPTS +=	-DUSE_RCMD_RSH
CPPOPTS +=	-DSCHILY_PRINT

#include		Targets
CFILES=		scsi-remote.c
LIBS=		-lscg -lschily -lc

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.shl
###########################################################################
#CC=		echo "	==> COMPILING \"$@\""; cc
###########################################################################
