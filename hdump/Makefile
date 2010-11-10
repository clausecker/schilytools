#ident %W% %E% %Q%
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		hdump
HARDLINKS=	od
CPPOPTS +=	-DUSE_LARGEFILES
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-DUSE_NLS
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
#LIBS=		-lunos
LIBS=		-lschily
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1

