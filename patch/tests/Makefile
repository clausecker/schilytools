all-tests:      test-basic \
		test-random
	echo Tests passed.

test-basic:
	cd basic && for i in *.sh; do echo Running test basic/$$i; $(SHELL) $$i; done

test-random:
	cd random && for i in *.sh; do echo Running test random/$$i; $(SHELL) $$i; done
