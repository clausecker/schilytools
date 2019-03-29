#ident "@(#)Makefile	1.10 19/03/28 "
###########################################################################
tests:		# Make "tests" the default target
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SCCS_BIN_PRE=	ccs/

tests: /tmp/SCCSTEST all-tests all-v6-tests
	@echo "Don't forget to remove /tmp/SCCSTEST in case you don't like to repeat the test"

all-tests: /tmp/SCCSTEST cssc-tests sccs-tests

all-v6-tests: /tmp/SCCSTEST cssc-v6-tests sccs-v6-tests

#
# ksh yields in an exit code of 1 for unset in case that SCCS_V6 has not been set before.
# We thus set SCCS_V6 for "unset"...
#
cssc-tests:
	SCCS_V6=a unset SCCS_V6 && \
	cd cssctests && dir=$${dir-/tmp/SCCSTEST/$(SCCS_BIN_PRE)bin} "$(MAKE)"
	@echo "Watch for FAIL and XFAIL in the output"

sccs-tests:
	SCCS_V6=a unset SCCS_V6 && \
	cd sccstests && dir=$${dir-/tmp/SCCSTEST/$(SCCS_BIN_PRE)bin} "$(MAKE)"
	@echo "Watch for FAIL and XFAIL in the output"

cssc-v6-tests:
	cd cssctests && \
		(dir=$${dir-/tmp/SCCSTEST/$(SCCS_BIN_PRE)bin}; SCCS_V6=; export dir SCCS_V6; \
		(cd ../common; . ./real-thing; eval $$TESTING_SCCS_V6) && "$(MAKE)" )
	@echo "Watch for FAIL and XFAIL in the output"

sccs-v6-tests:
	cd sccstests && \
		(dir=$${dir-/tmp/SCCSTEST/$(SCCS_BIN_PRE)bin}; SCCS_V6=; export dir SCCS_V6; \
		(cd ../common; . ./real-thing; eval $$TESTING_SCCS_V6) && "$(MAKE)" )
	@echo "Watch for FAIL and XFAIL in the output"

bins: /tmp/SCCSTEST

/tmp/SCCSTEST:
	cd ..; "$(MAKE)" clean rmtarget INS_BASE=/tmp/SCCSTEST install; "$(MAKE)" clean rmtarget all

clean:
	$(RM) -rf /tmp/SCCSTEST
