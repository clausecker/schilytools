#ident @(#)shlschily.mk	1.9 18/10/11 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/pic
SHL_MAJOR=	2
SHL_MINOR=	0
.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	schily
CPPOPTS +=	-Istdio
CPPOPTS +=	-DUSE_SCANSTACK	# Try to scan stack frames
CPPOPTS +=	-DPORT_ONLY	# Add missing funcs line snprintf for porting
include		Targets
LIBS=		$(LIB_INTL) -lc	# Now needs gettext()

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.shl
###########################################################################
