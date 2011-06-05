h30576
s 00018/00000/00000
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

# sactbasic.sh:  Basic tests for sact

# Import common functions & definitions.
. ../common/test-common

g=foo
s=s.$g
p=p.$g

remove $s $p $g

# It is invalid to supply no arguments to sact.
docommand sb1 "${vg_sact}" 1 IGNORE IGNORE 

remove $s $p $g
success
E 1
