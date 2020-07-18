#ident @(#)libdbgmalloc.mk	1.3 20/07/08 
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
	@echo "	==> MAKING SYMLINKS in ."; sh ./MKLINKS
$(SRCROOT)/$(RULESDIR)/rules.lib: alloc.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
#CC=		echo "	==> COMPILING \"$@\""; cc
###########################################################################
