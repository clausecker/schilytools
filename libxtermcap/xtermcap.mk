#ident @(#)xtermcap.mk	1.3 20/12/10 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

#.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
#VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	xtermcap
#CPPOPTS +=	-Ispecincl
CPPOPTS +=	-DUSE_PG
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
include		Targets
LIBS=		

XMK_FILE=	Makefile.tc

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
#CC=		echo "	==> COMPILING \"$@\""; cc
###########################################################################

