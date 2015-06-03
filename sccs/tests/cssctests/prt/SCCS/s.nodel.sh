h47043
s 00002/00002/00025
d D 1.3 15/06/03 00:06:44 joerg 3 2
c ../common/test-common -> ../../common/test-common
e
s 00001/00001/00026
d D 1.2 15/06/01 23:55:23 joerg 2 1
c ../../testutils/ -> ${SRCROOT}/tests/testutils/
e
s 00027/00000/00000
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

# default.sh:  Tests for prt no involving several deltas.

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

# XXX: the IGNORE in the followig lines is because of the warning message we
#      get about the excluded deltas feature not being fully tested.

do_output d1 "${vg_prt} -u $s" 0 expected/nodel.-u IGNORE
do_output d2 "${vg_prt} -f $s" 0 expected/nodel.-f IGNORE
do_output d3 "${vg_prt} -t $s" 0 expected/nodel.-t IGNORE
do_output d4 "${vg_prt} -b $s" 0 expected/nodel.-b IGNORE

do_output d5 "${vg_prt} -u -f $s" 0 expected/nodel.-u-f IGNORE
do_output d6 "${vg_prt} -t -b $s" 0 expected/nodel.-t-b IGNORE
do_output d7 "${vg_prt} -u -f -t -b $s" 0 expected/nodel.-u-f-t-b IGNORE

remove $s
success
E 1
