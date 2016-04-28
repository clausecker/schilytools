all-tests:	test-cmdsub	\
		test-errflg	\
		test-exit	\
		test-getopts	\
		test-here	\
		test-ifs	\
		test-parameter	\
		test-quoting	\
		test-return
	echo Tests passed.

test-cmdsub:
	cd cmdsub && for i in *.sh; do echo Running test cmdsub/$$i; $(SHELL) $$i; done

test-errflg:
	cd errflg && for i in *.sh; do echo Running test errflg/$$i; $(SHELL) $$i; done

test-exit:
	cd exit && for i in *.sh; do echo Running test exit/$$i; $(SHELL) $$i; done

test-getopts:
	cd getopts && for i in *.sh; do echo Running test getopts/$$i; $(SHELL) $$i; done

test-here:
	cd here && for i in *.sh; do echo Running test here/$$i; $(SHELL) $$i; done

test-ifs:
	cd ifs && for i in *.sh; do echo Running test ifs/$$i; $(SHELL) $$i; done

test-parameter:
	cd parameter && for i in *.sh; do echo Running test parameter/$$i; $(SHELL) $$i; done

test-quoting:
	cd quoting && for i in *.sh; do echo Running test quoting/$$i; $(SHELL) $$i; done

test-return:
	cd return && for i in *.sh; do echo Running test return/$$i; $(SHELL) $$i; done
