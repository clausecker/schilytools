#ident "@(#)libboshcmd.mk	1.1 18/01/10 "
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		lib
TARGETLIB=	boshcmd

CPPOPTS +=	-DUSE_LARGEFILES	# Allow Large Files (> 2 GB)
CPPOPTS +=	-DVFORK			# Use vfork() if possible

CPPOPTS	+=	-I../sh

include		Targets

LIBS=		-lfind -lc
XMK_FILE=	

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
count: $(HFILES) $(CFILES) 
	count $r1


