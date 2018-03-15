all-tests tests:	test-artype \
			test-basic \
		
	echo Tests passed.

test-artype:
	cd artype && for i in *.sh; do echo Running test artype/$$i; $(SHELL) $$i; done
test-basic:
	cd basic && for i in *.sh; do echo Running test basic/$$i; $(SHELL) $$i; done
