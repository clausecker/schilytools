#ident @(#)Makefile	1.5 17/04/22 
###########################################################################
SRCROOT=	../../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

#.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
#VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	bsd
CPPOPTS +=	-I../../../../include
CPPOPTS +=	-I../../../include
#CPPOPTS +=	-compat=4
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-D_FILE_OFFSET_BITS=64 -DSUN5_0
CPPOPTS +=	-DSUNOS4_AND_AFTER
CPPOPTS +=	-DPMAKE
CPPOPTS +=	-DREDIRECT_ERR

C++FILES=	bsd.cc

LIBS=		

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
#CC=		echo "	==> COMPILING \"$@\""; cc
###########################################################################
