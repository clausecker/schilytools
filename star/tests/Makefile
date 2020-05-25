#ident "@(#)Makefile	1.6 20/05/24 "
###########################################################################
tests:		# Make "tests" the default target
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

all-tests tests:	test-artype \
			test-basic \
			test-compress \
			test-pax \
			test-port \
			test-incr \
		
	echo Tests passed.

test-artype:
	cd artype && for i in *.sh; do echo Running test artype/$$i; $(SHELL) $$i; done
test-basic:
	cd basic && for i in *.sh; do echo Running test basic/$$i; $(SHELL) $$i; done
test-compress:
	cd compress && for i in *.sh; do echo Running test compress/$$i; $(SHELL) $$i; done
test-pax:
	cd pax && for i in *.sh; do echo Running test pax/$$i; $(SHELL) $$i; done
test-port:
	cd port && for i in *.sh; do echo Running test port/$$i; $(SHELL) $$i; done
test-incr:
	cd incremental && for i in *.sh; do echo Running test incremental/$$i; $(SHELL) $$i; done
