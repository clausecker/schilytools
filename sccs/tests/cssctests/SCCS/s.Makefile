hV6,sum=61011
s 00005/00001/00060
d D 1.2 2011/05/31 21:15:51+0200 joerg 2 1
S s 30611
c Kein Abbruch mehr bei Problen mit what/whatbasic.sh wgen HP-UX ARG_MAX
e
s 00061/00000/00000
d D 1.1 2011/05/27 18:55:58+0200 joerg 1 0
S s 15121
c date and time created 11/05/27 18:55:58 by joerg
e
u
U
f e 0
G r 0e46e8eb275a9
G p sccs/tests/cssctests/Makefile
t
T
I 1
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

I 2
#
# what/whatbasic.sh may fail on HP-UX because of the low ARG_MAX limit
# Allow to continue in this case
#
E 2
test-what:
D 2
	cd what && for i in *.sh; do echo Running test what/$$i; $(SHELL) $$i; done
E 2
I 2
	cd what && for i in *.sh; do echo Running test what/$$i; $(SHELL) $$i || echo "whatbasic.sh may fail on HP-UX because of the low ARG_MAX limit"; done
E 2

test-year-2000:
	cd year-2000 && for i in *.sh; do echo Running test year-2000/$$i; $(SHELL) $$i; done

E 1
