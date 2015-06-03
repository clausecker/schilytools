h02724
s 00002/00002/00016
d D 1.3 15/06/03 00:06:44 joerg 3 2
c ../common/test-common -> ../../common/test-common
e
s 00001/00001/00017
d D 1.2 15/06/01 23:55:23 joerg 2 1
c ../../testutils/ -> ${SRCROOT}/tests/testutils/
e
s 00018/00000/00000
d D 1.1 10/04/29 02:05:14 joerg 1 0
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh

# default.sh:  Test the default behaviour of prt.

# Import common functions & definitions.
D 3
. ../common/test-common
. ../common/need-prt
E 3
I 3
. ../../common/test-common
. ../../common/need-prt
E 3

s=s.testfile

remove $s
D 2
../../testutils/uu_decode --decode < testfile.uue || miscarry could not uudecode testfile.uue.
E 2
I 2
${SRCROOT}/tests/testutils/uu_decode --decode < testfile.uue || miscarry could not uudecode testfile.uue.
E 2


do_output d1 "${vg_prt} $s" 0 expected/default.1 IGNORE

remove $s
success
E 1
