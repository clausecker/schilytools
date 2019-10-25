#ident @(#)shlgetopt.mk	1.2 19/10/23 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/pic
INSDIR=		lib
TARGETLIB=	getopt
CPPOPTS +=	-DUSE_SCANSTACK
CPPOPTS +=	-DDO_GETOPT_LONGONLY	# Support getopt(.. "?900?(long)")
CPPOPTS +=	-DDO_GETOPT_SDASH_LONG	# Support -long also
CPPOPTS +=	-DDO_GETOPT_PLUS	# Support +o also
CPPOPTS +=	-DTEXT_DOMAIN='"SUNW_OST_OSLIB"'	# dgettext
include		Targets
LIBS=		

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.shl
###########################################################################
#CC=		echo "	==> COMPILING \"$@\""; cc
###########################################################################
