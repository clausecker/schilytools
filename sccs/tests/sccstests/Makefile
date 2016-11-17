#SHELL=/bin/bash
#SHELL=/bin/sh

all-tests:      test-initial \
		test-format \
		test-extensions \
                test-rmdel \
                test-admin test-delta test-get test-prs test-prt \
		test-random test-unget \
                test-cdc test-sact test-val \
                test-large test-sccsdiff test-binary test-sccs test-what \
		test-help test-sccslog \
                test-year-2000
	echo Tests passed.

test-initial:
	cd initial && for i in *.sh; do echo Running test initial/$$i; $(SHELL) $$i; done

test-format:
	cd format && for i in *.sh; do echo Running test format/$$i; $(SHELL) $$i; done

test-extensions:
	cd extensions && for i in *.sh; do echo Running test extensions/$$i; $(SHELL) $$i; done

test-rmdel:
	cd rmdel && for i in *.sh; do echo Running test rmdel/$$i; $(SHELL) $$i; done

test-admin:
	cd admin && for i in *.sh; do echo Running test admin/$$i; $(SHELL) $$i; done

test-delta:
	cd delta && for i in *.sh; do echo Running test delta/$$i; $(SHELL) $$i; done

test-get:
	cd get && for i in *.sh; do echo Running test get/$$i; $(SHELL) $$i; done

test-prs:
	cd prs && for i in *.sh; do echo Running test prs/$$i; $(SHELL) $$i; done

test-prt:
	cd prt && for i in *.sh; do echo Running test prt/$$i; $(SHELL) $$i; done

test-random:
	cd random && for i in *.sh; do echo Running test random/$$i; $(SHELL) $$i; done

test-unget:
	cd unget && for i in *.sh; do echo Running test unget/$$i; $(SHELL) $$i; done

test-cdc:
	cd cdc && for i in *.sh; do echo Running test cdc/$$i; $(SHELL) $$i; done

test-sact:
	cd sact && for i in *.sh; do echo Running test sact/$$i; $(SHELL) $$i; done

test-val:
	cd val && for i in *.sh; do echo Running test val/$$i; $(SHELL) $$i; done

test-large:
	cd large && for i in *.sh; do echo Running test large/$$i; $(SHELL) $$i; done

test-sccsdiff:
	cd sccsdiff && for i in *.sh; do echo Running test sccsdiff/$$i; $(SHELL) $$i; done

test-binary:
	cd binary && for i in *.sh; do echo Running test binary/$$i; $(SHELL) $$i; done

test-sccs:
	cd sccs && for i in *.sh; do echo Running test sccs/$$i; $(SHELL) $$i; done

test-what:
	cd what && for i in *.sh; do echo Running test what/$$i; $(SHELL) $$i; done

test-help:
	cd help && for i in *.sh; do echo Running test help/$$i; $(SHELL) $$i; done

test-sccslog:
	cd sccslog && for i in *.sh; do echo Running test sccslog/$$i; $(SHELL) $$i; done

test-year-2000:
	cd year-2000 && for i in *.sh; do echo Running test year-2000/$$i; $(SHELL) $$i; done

