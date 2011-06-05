#SHELL=/bin/bash

all-tests:      test-initial \
                test-rmdel \
                test-admin test-delta test-get test-prs test-prt test-unget \
                test-cdc  test-sact test-val \
                test-large test-sccsdiff test-binary test-bsd-sccs test-what \
                test-year-2000
	echo Tests passed.

test-initial:
	cd initial && for i in *.sh; do echo Running test initial/$$i; $(SHELL) $$i; done

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

test-bsd-sccs:
	cd bsd-sccs && for i in *.sh; do echo Running test bsd-sccs/$$i; $(SHELL) $$i; done

#
# what/whatbasic.sh may fail on HP-UX because of the low ARG_MAX limit
# Allow to continue in this case
#
test-what:
	cd what && for i in *.sh; do echo Running test what/$$i; $(SHELL) $$i || echo "whatbasic.sh may fail on HP-UX because of the low ARG_MAX limit"; done

test-year-2000:
	cd year-2000 && for i in *.sh; do echo Running test year-2000/$$i; $(SHELL) $$i; done

