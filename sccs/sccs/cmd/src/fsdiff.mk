#ident @(#)fsdiff.mk	1.1 20/08/30 
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

PREINSDIR=	$(SCCS_BIN_PRE)
#SCCS_BIN_PRE=	sccs/
SCCS_HELP_PRE=	ccs/
SCCS_BIN_PRE=	ccs/
INSDIR=		bin
TARGET=		fsdiff
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		fsdiff.c
HFILES=
#LIBS=		-lunos
LIBS=		-lschily $(LIB_INTL)
#XMK_FILE=	Makefile.man fdiff.mk1 fsdiff.mk1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:		$(CFILES) $(HFILES)
		count $r1 
