#ident "@(#)Makefile	1.4 19/03/27 "
###########################################################################
tests:		# Make "tests" the default target
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

tests:
	cd shelltests && "$(MAKE)"
	@echo "Watch for FAIL and XFAIL in the output"
