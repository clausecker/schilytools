hV6,sum=52533
s 00001/00001/00023
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 27559
c ../common/test-common -> ../../common/test-common
e
s 00024/00000/00000
d D 1.1 2010/04/29 02:05:14+0200 joerg 1 0
S s 27420
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ed4846f
G p sccs/tests/cssctests/get/sf664900.sh
t
T
I 1
#! /bin/sh
# sf664900.sh: tests for SourceForge bug number 664900 

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2


g=foo.txt
s=s.$g
x=x.$g 
z=z.$g
p=p.$g


remove $g $s $x $z $p
touch $g

docommand t1 "${admin} -i$g -r1.1.1.1 -n $s" 0 IGNORE IGNORE 
remove foo 

docommand t2 "${vg_get} $s" 1 IGNORE IGNORE

remove $g $s $x $z $p
success
E 1
