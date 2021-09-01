#ident @(#)Makefile   1.12 21/08/18 Copyr 1990 J. Schilling
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		calltree
#CPPOPTS +=	-Ispecincl
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-DUSE_NLS		# Include locale support
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DINS_RBASE=\"${INS_RBASE}\"
CPPOPTS +=	-DTEXT_DOMAIN=\"SCHILY_utils\"

CFILES=		calltree.c clex.c keyw.c string.c sym.c
HFILES=		clex.h strsubs.h sym.h version.h
LIBS=		-lschily $(LIB_INTL)
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:		$(CFILES) $(HFILES)
		count $r1 

bintree:	$(CFILES)
		calltree -bg $(CPPOPTS) $r1 

