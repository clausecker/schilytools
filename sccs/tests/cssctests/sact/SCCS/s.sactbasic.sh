hV6,sum=47997
s 00001/00001/00017
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 22427
c ../common/test-common -> ../../common/test-common
e
s 00018/00000/00000
d D 1.1 2010/05/03 03:11:28+0200 joerg 1 0
S s 22288
c date and time created 10/05/03 03:11:28 by joerg
e
u
U
f e 0
f y 
G r 0e46e8eef01bf
G p sccs/tests/cssctests/sact/sactbasic.sh
t
T
I 1
#! /bin/sh

# sactbasic.sh:  Basic tests for sact

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

g=foo
s=s.$g
p=p.$g

remove $s $p $g

# It is invalid to supply no arguments to sact.
docommand sb1 "${vg_sact}" 1 IGNORE IGNORE 

remove $s $p $g
success
E 1
