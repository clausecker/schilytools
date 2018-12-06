h56053
s 00004/00001/00083
d D 1.5 18/12/04 00:50:21 joerg 5 4
c comb neu
e
s 00004/00001/00080
d D 1.4 18/12/04 00:40:29 joerg 4 3
c sccscvt neu
e
s 00006/00002/00075
d D 1.3 16/11/13 23:07:18 joerg 3 2
c test-random neu
e
s 00004/00000/00073
d D 1.2 16/06/20 01:39:02 joerg 2 1
c test-format neu
e
s 00073/00000/00000
d D 1.1 11/05/30 00:05:02 joerg 1 0
c date and time created 11/05/30 00:05:02 by joerg
e
u
U
f e 0
t
T
I 1
#SHELL=/bin/bash
#SHELL=/bin/sh

all-tests:      test-initial \
I 2
		test-format \
E 2
		test-extensions \
                test-rmdel \
D 3
                test-admin test-delta test-get test-prs test-prt test-unget \
                test-cdc  test-sact test-val \
E 3
I 3
                test-admin test-delta test-get test-prs test-prt \
		test-random test-unget \
D 5
                test-cdc test-sact test-val \
E 5
I 5
                test-cdc test-comb test-sact test-val \
E 5
E 3
                test-large test-sccsdiff test-binary test-sccs test-what \
D 4
		test-help test-sccslog \
E 4
I 4
		test-help test-sccscvt test-sccslog \
E 4
                test-year-2000
	echo Tests passed.

test-initial:
	cd initial && for i in *.sh; do echo Running test initial/$$i; $(SHELL) $$i; done

I 2
test-format:
	cd format && for i in *.sh; do echo Running test format/$$i; $(SHELL) $$i; done

E 2
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

I 3
test-random:
	cd random && for i in *.sh; do echo Running test random/$$i; $(SHELL) $$i; done

E 3
test-unget:
	cd unget && for i in *.sh; do echo Running test unget/$$i; $(SHELL) $$i; done

test-cdc:
	cd cdc && for i in *.sh; do echo Running test cdc/$$i; $(SHELL) $$i; done

I 5
test-comb:
	cd comb && for i in *.sh; do echo Running test comb/$$i; $(SHELL) $$i; done

E 5
test-sact:
	cd sact && for i in *.sh; do echo Running test sact/$$i; $(SHELL) $$i; done

test-val:
	cd val && for i in *.sh; do echo Running test val/$$i; $(SHELL) $$i; done

test-large:
	cd large && for i in *.sh; do echo Running test large/$$i; $(SHELL) $$i; done

I 4
test-sccscvt:
	cd sccscvt && for i in *.sh; do echo Running test sccscvt/$$i; $(SHELL) $$i; done

E 4
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

E 1
