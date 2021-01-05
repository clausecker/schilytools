#ident @(#)shlxtermcap.mk	1.2 20/12/10 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/pic
#.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
#VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	xtermcap
#CPPOPTS +=	-Ispecincl
CPPOPTS +=	-DUSE_PG
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
include		Targets
LIBS=		-lschily -lc

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.shl
###########################################################################
#CC=		echo "	==> COMPILING \"$@\""; cc
###########################################################################
