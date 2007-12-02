#ident @(#)libsiconv.mk	1.1 07/05/19 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		lib
TARGETLIB=	siconv
#CPPOPTS +=	-Istdio
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-DUSE_ICONV
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"

include		Targets
LIBS=

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
