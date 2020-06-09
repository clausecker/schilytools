#ident "@(#)libboshcmd_p.mk	1.2 18/08/01 "
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/profiled
SUBINSDIR=	/profiled
INSDIR=		lib
TARGETLIB=	boshcmd

CPPOPTS +=	-DSCHILY_INCLUDES	# Tell the code to use schily include files
CPPOPTS +=	-DUSE_LARGEFILES	# Allow Large Files (> 2 GB)
CPPOPTS +=	-DVFORK			# Use vfork() if possible

CPPOPTS	+=	-I../sh

COPTS +=	$(COPTGPROF)

include		Targets

LIBS=		-lfind -lc
XMK_FILE=	

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
count: $(HFILES) $(CFILES) 
	count $r1

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.rel
###########################################################################
