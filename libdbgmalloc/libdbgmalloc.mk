#ident @(#)libdbgmalloc.mk	1.1 09/10/18 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

#.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
#VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	dbgmalloc
#CPPOPTS +=	-Ispecincl
CPPOPTS +=	-DUSE_SCANSTACK
CPPOPTS +=	-DD_MALLOC
include		Targets
LIBS=		

alloc.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
#CC=		echo "	==> COMPILING \"$@\""; cc
###########################################################################
