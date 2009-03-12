#ident @(#)sccs.mk	1.5 09/01/10 
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		xpg4/bin
TARGET=		sccs
#SYMLINKS=	../../bin/sccs

CPPOPTS +=	-DSUN5_0
CPPOPTS +=	-DXPG4
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-I../../../sgs/inc/common
CPPOPTS +=	-I../../hdr
CPPOPTS +=	-DUSE_RECURSIVE
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"

CFILES=		sccs.c

#HFILES=		make.h

LIBS=		-lcomobj -lcassi -lmpw -lgetopt -lschily $(LIB_INTL)
LIBS=		-lcomobj -lcassi -lmpw -lgetopt -lfind -lschily $(LIB_ACL_TEST) $(LIB_INTL)

#XMK_FILE=	Makefile.def Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
###########################################################################
