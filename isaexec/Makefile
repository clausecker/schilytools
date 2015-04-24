#ident "@(#)Makefile	1.2 10/11/14 "
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		isaexec
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	'-DTEXT_DOMAIN="SUNW_OST_OSCMD"'

SUNPROCOPTOPT=	-fast -xspace -xstrconst $(XARCH_OPT)

CFILES=		isaexec.c

XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

