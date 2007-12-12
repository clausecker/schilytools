#ident @(#)Makefile	1.2 07/01/07 
###########################################################################
# Sample makefile for non-shared libraries
###########################################################################
SRCROOT=	../../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		ccs/lib
TARGETLIB=	mpw
CPPOPTS +=	-DSUN5_0
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-I../../../../sgs/inc/common
CPPOPTS +=	-I../../../hdr
CPPOPTS +=	-DUSE_NLS

include		Targets

LIBS=
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
