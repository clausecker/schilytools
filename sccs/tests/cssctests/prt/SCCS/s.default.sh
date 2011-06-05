h38163
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
. ../common/test-common
. ../common/need-prt

s=s.testfile

remove $s
../../testutils/uu_decode --decode < testfile.uue || miscarry could not uudecode testfile.uue.


do_output d1 "${vg_prt} $s" 0 expected/default.1 IGNORE

remove $s
success
E 1
