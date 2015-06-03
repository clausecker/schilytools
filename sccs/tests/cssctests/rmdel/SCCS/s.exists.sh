h37506
s 00001/00001/00012
d D 1.2 15/06/03 00:06:44 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00013/00000/00000
d D 1.1 10/05/03 03:11:28 joerg 1 0
c date and time created 10/05/03 03:11:28 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh
# exists.sh:  What if the input file doesn't exist?

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

s=s.testfile

remove $s
docommand e1 "${vg_rmdel} -r1.1 $s" 1 "IGNORE" "IGNORE"
remove $s

success
E 1
