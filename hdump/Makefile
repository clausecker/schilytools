#ident @(#)Makefile	1.10 10/12/23 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		hdump
HARDLINKS=	od
#HARDLINKS=	../xpg4/bin/od
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-DUSE_NLS
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DTEXT_DOMAIN=\"SCHILY_UTILS\"
#
# Selectively increase the opimisation for hdump for better performance
#
# The code has been tested for correctness with this level of optimisation
# If your GCC creates defective code, you found a GCC bug that should
# be reported to the GCC people. As a workaround, you may remove the next
# lines to fall back to the standard optimisation level.
#
_XARCH_OPT=	$(OARCH:%cc64=$(SUNPROCOPT64))
XARCH_OPT=	$(_XARCH_OPT:%cc=$(XARCH_GEN))

SUNPROCOPTOPT=	-fast $(XARCH_OPT)
GCCOPTOPT=	-O3  -fexpensive-optimizations
#
CFILES=		hdump.c
#HFILES=
LIBS=		-lschily $(LIB_INTL)
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

