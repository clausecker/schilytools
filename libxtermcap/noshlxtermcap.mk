#ident "@(#)noshlxtermcap.mk	1.2 20/06/01 "
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

all:
	@echo "	==> MAKING no shared libs on this platform (LINKMODE=${LNKMODE})"

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.dyn
###########################################################################
