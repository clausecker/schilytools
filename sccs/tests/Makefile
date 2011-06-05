#ident "%W% %E% %Q%"
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

all: /tmp/SCCSTEST all-tests
	@echo "Don't forget to remove /tmp/SCCSTEST in case you don't like to repeat the test"

all-tests: cssc-tests sccs-tests

cssc-tests:
	cd cssctests && dir=$${dir-/tmp/SCCSTEST/ccs/bin} $(MAKE)
	@echo "Watch for FAIL and XFAIL in the output"

sccs-tests:
	cd sccstests && dir=$${dir-/tmp/SCCSTEST/ccs/bin} $(MAKE)
	@echo "Watch for FAIL and XFAIL in the output"

bins: /tmp/SCCSTEST

/tmp/SCCSTEST:
	cd ..; $(MAKE) clean rmtarget INS_BASE=/tmp/SCCSTEST install clean rmtarget

clean:
	$(RM) -rf /tmp/SCCSTEST
