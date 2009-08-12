#ident "@(#)Makefile	1.1 09/08/11 "
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		mdigest
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		mdigest.c
HFILES=
LIBS=		-lmdigest -lschily
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

IFDEF=	-UJOS 

XRELFILES=	Makefile Makefile.man mdigest.1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.rel
###########################################################################

MAKE_LICENSE=MKCDDL
