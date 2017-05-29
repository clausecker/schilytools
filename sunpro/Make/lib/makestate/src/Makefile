#ident @(#)Makefile	1.4 17/05/28 
###########################################################################
SRCROOT=	../../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/pic
#.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
#VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	makestate
CPPOPTS +=	-DSCHILY_INCLUDES	# To use schily include files
CFILES=		ld_file.c lock.c
LIBS=		$(LIB_ELF) $(LIB_C)

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.shl
###########################################################################
#CC=		echo "	==> COMPILING \"$@\""; cc
###########################################################################
