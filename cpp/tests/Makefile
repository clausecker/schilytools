all-tests tests:	test-basic		
	echo Tests passed.

test-basic:
	cd basic && for i in *.sh; do echo Running test basic/$$i; $(SHELL) $$i; done
