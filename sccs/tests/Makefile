#ident "@(#)Makefile	1.14 20/08/25 "
###########################################################################
tests:		# Make "tests" the default target
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SCCS_BIN_PRE=	ccs/

tests: /tmp/SCCSTEST /tmp/sccstests all-tests all-v6-tests
	@echo "Don't forget to remove /tmp/SCCSTEST in case you don't like to repeat the test"
	@echo "Don't forget to remove /tmp/sccstests in case you don't like to repeat the test"

all-tests: /tmp/SCCSTEST cssc-tests sccs-tests

all-v6-tests: /tmp/SCCSTEST cssc-v6-tests sccs-v6-tests

#
# ksh yields in an exit code of 1 for unset in case that SCCS_V6 has not been set before.
# We thus set SCCS_V6 for "unset"...
#
# Do the same with SCCS_NMODE to make sure SCCS is checked with the implemented
# default. If we need it, we set it explicitly in a related test.
#
cssc-tests:
	SCCS_V6=a unset SCCS_V6 && \
	SCCS_NMODE=a unset SCCS_NMODE && \
	cd /tmp/sccstests/sccs/tests && \
	cd cssctests && dir=$${dir-/tmp/SCCSTEST/$(SCCS_BIN_PRE)bin} "$(MAKE)"
	@echo "Watch for FAIL and XFAIL in the output"

sccs-tests:
	SCCS_V6=a unset SCCS_V6 && \
	SCCS_NMODE=a unset SCCS_NMODE && \
	cd /tmp/sccstests/sccs/tests && \
	cd sccstests && dir=$${dir-/tmp/SCCSTEST/$(SCCS_BIN_PRE)bin} "$(MAKE)"
	@echo "Watch for FAIL and XFAIL in the output"

cssc-v6-tests:
	SCCS_NMODE=a unset SCCS_NMODE && \
	cd /tmp/sccstests/sccs/tests && \
	cd cssctests && \
		(dir=$${dir-/tmp/SCCSTEST/$(SCCS_BIN_PRE)bin}; SCCS_V6=; export dir SCCS_V6; \
		(cd ../common; . ./real-thing; eval $$TESTING_SCCS_V6) && "$(MAKE)" )
	@echo "Watch for FAIL and XFAIL in the output"

sccs-v6-tests:
	SCCS_NMODE=a unset SCCS_NMODE && \
	cd /tmp/sccstests/sccs/tests && \
	cd sccstests && \
		(dir=$${dir-/tmp/SCCSTEST/$(SCCS_BIN_PRE)bin}; SCCS_V6=; export dir SCCS_V6; \
		(cd ../common; . ./real-thing; eval $$TESTING_SCCS_V6) && "$(MAKE)" )
	@echo "Watch for FAIL and XFAIL in the output"

bins: /tmp/SCCSTEST

/tmp/SCCSTEST: /tmp/SCCSTEST/bin

/tmp/SCCSTEST/bin:
	cd ..; "$(MAKE)" clean rmtarget INS_BASE=/tmp/SCCSTEST install; "$(MAKE)" clean rmtarget all

copy: /tmp/sccstests

/tmp/sccstests: /tmp/sccstests/sccs

/tmp/sccstests/sccs:
	star -version > /dev/null || (echo "Please install star"; exit 1)
	mkdir -p /tmp/sccstests/sccs && star -no-fsync -C /tmp/sccstests -xp -bz < $(SRCROOT)/makefiles.tar.bz2
	star -no-fsync -copy -C ../../ incs tests /tmp/sccstests/
	mkdir -p /tmp/sccstests/sccs/tests/ && star -no-fsync -copy -C . Makefile README common cssctests sccstests /tmp/sccstests/sccs/tests/

clean:
	$(RM) -rf /tmp/SCCSTEST
