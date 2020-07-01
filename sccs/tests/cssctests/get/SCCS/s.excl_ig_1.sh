hV6,sum=51923
s 00001/00001/00049
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 26588
c ../common/test-common -> ../../common/test-common
e
s 00050/00000/00000
d D 1.1 2010/04/29 02:05:14+0200 joerg 1 0
S s 26449
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ecf6780
G p sccs/tests/cssctests/get/excl_ig_1.sh
t
T
I 1
#! /bin/sh
# excl_ig_1.sh:  Tests for exclusions and ignores.

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2


g=incl_excl_1
s=s.$g
x=x.$g 
z=z.$g
p=p.$g
remove $g $s $x $z $p

cp s.incl_excl_1.input $s

docommand xg1 "${vg_get} -r1.1 -p $s"      0 "" IGNORE

docommand xg2 "${vg_get} -r1.2 -p $s"      0 \
    "This line was added in version 1.2
" IGNORE

docommand xg3 "${vg_get} -r1.3 -p $s"      0 \
    "This line was added in version 1.2
This line was added in version 1.3
" IGNORE

docommand xg4 "${vg_get} -r1.4 -p $s"      0 \
    "This line was added in version 1.2
This line was added in version 1.3
This line was added in version 1.4
" IGNORE

docommand xg5 "${vg_get} -r1.5 -p $s"      0 \
    "This line was added in version 1.2
This line was added in version 1.3
This line was added in version 1.4
This line was added in version 1.5
" IGNORE

# Revision 1.6 ignores SID 1.2.
docommand xg6 "${vg_get} -r1.6 -p $s"      0 \
"This line was added in version 1.3
This line was added in version 1.4
This line was added in version 1.5
This line was added in version 1.6; that version also ignores 1.2\n" \
IGNORE

remove $g $s $x $z $p
success
E 1
