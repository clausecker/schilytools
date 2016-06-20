all-tests:	test-alias \
		test-arith \
		test-break	\
		test-cmdsub	\
		test-echo	\
		test-errflg	\
		test-error	\
		test-exit	\
		test-getopts	\
		test-glob	\
		test-here	\
		test-ifs	\
		test-misc	\
		test-parameter	\
		test-quoting	\
		test-return	\
		test-set	\
		test-test
	echo Tests passed.

test-alias:
	cd alias && for i in *.sh; do echo Running test alias/$$i; $(SHELL) $$i; done

test-arith:
	cd arith && for i in *.sh; do echo Running test arith/$$i; $(SHELL) $$i; done

test-break:
	cd break && for i in *.sh; do echo Running test break/$$i; $(SHELL) $$i; done

test-cmdsub:
	cd cmdsub && for i in *.sh; do echo Running test cmdsub/$$i; $(SHELL) $$i; done

test-errflg:
	cd errflg && for i in *.sh; do echo Running test errflg/$$i; $(SHELL) $$i; done

test-echo:
	cd echo && for i in *.sh; do echo Running test echo/$$i; $(SHELL) $$i; done

test-error:
	cd error && for i in *.sh; do echo Running test error/$$i; $(SHELL) $$i; done

test-exit:
	cd exit && for i in *.sh; do echo Running test exit/$$i; $(SHELL) $$i; done

test-getopts:
	cd getopts && for i in *.sh; do echo Running test getopts/$$i; $(SHELL) $$i; done

test-glob:
	cd glob && for i in *.sh; do echo Running test glob/$$i; $(SHELL) $$i; done

test-here:
	cd here && for i in *.sh; do echo Running test here/$$i; $(SHELL) $$i; done

test-ifs:
	cd ifs && for i in *.sh; do echo Running test ifs/$$i; $(SHELL) $$i; done

test-misc:
	cd misc && for i in *.sh; do echo Running test misc/$$i; $(SHELL) $$i; done

test-parameter:
	cd parameter && for i in *.sh; do echo Running test parameter/$$i; $(SHELL) $$i; done

test-quoting:
	cd quoting && for i in *.sh; do echo Running test quoting/$$i; $(SHELL) $$i; done

test-return:
	cd return && for i in *.sh; do echo Running test return/$$i; $(SHELL) $$i; done

test-set:
	cd set && for i in *.sh; do echo Running test set/$$i; $(SHELL) $$i; done

test-test:
	cd test && for i in *.sh; do echo Running test test/$$i; $(SHELL) $$i; done
