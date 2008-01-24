#ident "@(#)librscg_p.mk	1.4 08/01/11 "
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/profiled
SUBINSDIR=	/profiled
#.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
#VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	rscg
#CPPOPTS +=	-Ispecincl
CPPOPTS +=	-DUSE_PG
CPPOPTS +=	-DUSE_RCMD_RSH
CPPOPTS +=	-DSCHILY_PRINT
COPTS +=	$(COPTGPROF)

#include		Targets
CFILES=		scsi-remote.c
LIBS=		

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
#CC=		echo "	==> COMPILING \"$@\""; cc
###########################################################################
