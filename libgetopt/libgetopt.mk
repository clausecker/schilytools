
#ident @(#)libgetopt.mk	1.3 15/12/13 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		lib
TARGETLIB=	getopt
CPPOPTS +=	-DUSE_SCANSTACK
CPPOPTS +=	-DDO_GETOPT_LONGONLY	# Support getopt(.. "?900?(long)")
CPPOPTS +=	-DDO_GETOPT_SDASH_LONG	# Support -long also
include		Targets
LIBS=		

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
#CC=		echo "	==> COMPILING \"$@\""; cc
###########################################################################
