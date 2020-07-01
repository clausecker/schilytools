hV6,sum=47150
s 00002/00002/00013
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 20361
c ../common/test-common -> ../../common/test-common
e
s 00015/00000/00000
d D 1.1 2010/04/29 02:05:14+0200 joerg 1 0
S s 20083
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ee83703
G p sccs/tests/cssctests/prt/exists.sh
t
T
I 1
#! /bin/sh

# exists.sh:  What if the input file doesn't exist?

# Import common functions & definitions.
D 2
. ../common/test-common
. ../common/need-prt
E 2
I 2
. ../../common/test-common
. ../../common/need-prt
E 2

s=s.testfile

remove $s
docommand e1 "${vg_prt} $s" 1 "IGNORE" "IGNORE"
remove $s

success
E 1
