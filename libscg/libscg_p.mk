#ident "@(#)libscg_p.mk	1.4 08/01/11 "
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
TARGETLIB=	scg
#CPPOPTS +=	-Ispecincl
CPPOPTS +=	-DUSE_PG
CPPOPTS +=	-DSCHILY_PRINT
COPTS +=	$(COPTGPROF)

include		Targets
LIBS=		

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
#CC=		echo "	==> COMPILING \"$@\""; cc
###########################################################################
