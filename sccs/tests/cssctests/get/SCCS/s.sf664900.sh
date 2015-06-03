h46062
s 00001/00001/00023
d D 1.2 15/06/03 00:06:44 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00024/00000/00000
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
