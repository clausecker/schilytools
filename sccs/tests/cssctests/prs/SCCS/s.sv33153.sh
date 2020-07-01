hV6,sum=06448
s 00001/00001/00021
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 47041
c ../common/test-common -> ../../common/test-common
e
s 00022/00000/00000
d D 1.1 2011/04/30 16:33:45+0200 joerg 1 0
S s 46902
c date and time created 11/04/30 16:33:45 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ee54e70
G p sccs/tests/cssctests/prs/sv33153.sh
t
T
I 1
#! /bin/sh

# Tests for Savannah bug #33153 ("prs" includes "AUTO NULL DELTAS").


# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

s=s.sv33153
cleanup () {
    remove command.log
}
cleanup
# Deltas 2.1, 3.1, 4.1, 5.1 are "AUTO NULL DELTA"s and all 
# have the same timestamp.  Nevertheless, "prs -l -r4.1" and
# "prs -e -r4.1" should start/stop based on the SID itself, not
# on the timestamp of the delta.
docommand nd1 "${prs} -l -r4.1 -d:I: $s" 0 "6.1\n5.1\n4.1\n" ""
docommand nd2 "${prs} -e -r4.1 -d:I: $s" 0 "4.1\n3.1\n2.1\n1.1\n" ""
docommand nd2 "${prs}    -r4.1 -d:I: $s" 0 "4.1\n" ""

cleanup
E 1
