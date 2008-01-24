#ident @(#)libfind_p.mk	1.4 08/01/11 
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
TARGETLIB=	find
COPTS +=	$(COPTGPROF)
#CPPOPTS +=	-Istdio
#CPPOPTS +=	-DUSE_SCANSTACK

#CPPOPTS +=	-D__FIND__
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DUSE_ACL
CPPOPTS +=	-DUSE_XATTR
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	-DSCHILY_PRINT

include		Targets
LIBS=		

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
