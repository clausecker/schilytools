#ident "@(#)Makefile	1.2 16/06/16 "
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

all tests:
	cd shelltests && $(MAKE)
	@echo "Watch for FAIL and XFAIL in the output"
