#ident @(#)Mctags	1.3 20/10/31 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		vctags
SYMLINKS +=	ctags

CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DVED_STATS
CPPOPTS +=	-DFASTPOS
#CPPOPTS +=	-DFASTPOS -DCHECKPOS
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		ctags.c

HFILES=

LIBS=
XMK_FILE=	Mctags.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################

count:		$(CFILES) $(HFILES)
		count $r1

