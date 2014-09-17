#ident @(#)sccs.mk	1.7 14/08/09 
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		ccs/bin
TARGET=		sccs
#SYMLINKS=	../../bin/sccs

CPPOPTS +=	-DSUN5_0
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-I../../../sgs/inc/common
CPPOPTS +=	-I../../hdr
CPPOPTS +=	-DUSE_RECURSIVE
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DSCCS_FATALHELP		# auto call to help

CFILES=		sccs.c

#HFILES=		make.h

LIBS=		-lcomobj -lcassi -lmpw -lgetopt -lschily $(LIB_INTL)
LIBS=		-lcomobj -lcassi -lmpw -lgetopt -lfind -lschily $(LIB_ACL_TEST) $(LIB_INTL)

XMK_FILE=	sccs_sym.mk
#XMK_FILE=	Makefile.def Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
###########################################################################
