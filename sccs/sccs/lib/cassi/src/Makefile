#ident @(#)Makefile	1.3 18/04/04 
###########################################################################
# Sample makefile for non-shared libraries
###########################################################################
SRCROOT=	../../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

PREINSDIR=	$(SCCS_BIN_PRE)
SCCS_HELP_PRE=	ccs/
SCCS_BIN_PRE=	ccs/
INSDIR=		lib
TARGETLIB=	cassi
CPPOPTS +=	-DSUN5_0
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-I../../../../sgs/inc/common
CPPOPTS +=	-I../../../hdr
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	-DSCCS_HELP_PRE=\"${SCCS_HELP_PRE}\"
CPPOPTS +=	-DSCCS_BIN_PRE=\"${SCCS_BIN_PRE}\"

include		Targets

LIBS=
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
