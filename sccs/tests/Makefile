#ident "@(#)Makefile	1.6 17/08/04 "
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

all tests: /tmp/SCCSTEST all-tests all-v6-tests
	@echo "Don't forget to remove /tmp/SCCSTEST in case you don't like to repeat the test"

all-tests: cssc-tests sccs-tests

all-v6-tests: cssc-v6-tests sccs-v6-tests

cssc-tests:
	unset SCCS_V6 && \
	cd cssctests && dir=$${dir-/tmp/SCCSTEST/ccs/bin} "$(MAKE)"
	@echo "Watch for FAIL and XFAIL in the output"

sccs-tests:
	unset SCCS_V6 && \
	cd sccstests && dir=$${dir-/tmp/SCCSTEST/ccs/bin} "$(MAKE)"
	@echo "Watch for FAIL and XFAIL in the output"

cssc-v6-tests:
	cd cssctests && \
		(dir=$${dir-/tmp/SCCSTEST/ccs/bin}; SCCS_V6=; export dir SCCS_V6; \
		(cd ../common; . ./real-thing; eval $$TESTING_SCCS_V6) && "$(MAKE)" )
	@echo "Watch for FAIL and XFAIL in the output"

sccs-v6-tests:
	cd sccstests && \
		(dir=$${dir-/tmp/SCCSTEST/ccs/bin}; SCCS_V6=; export dir SCCS_V6; \
		(cd ../common; . ./real-thing; eval $$TESTING_SCCS_V6) && "$(MAKE)" )
	@echo "Watch for FAIL and XFAIL in the output"

bins: /tmp/SCCSTEST

/tmp/SCCSTEST:
	cd ..; "$(MAKE)" clean rmtarget INS_BASE=/tmp/SCCSTEST install; "$(MAKE)" clean rmtarget all

clean:
	$(RM) -rf /tmp/SCCSTEST
