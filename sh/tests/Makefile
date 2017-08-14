#ident "@(#)Makefile	1.3 17/08/04 "
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

all tests:
	cd shelltests && "$(MAKE)"
	@echo "Watch for FAIL and XFAIL in the output"
