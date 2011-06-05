h42475
s 00026/00000/00000
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

# sf513800.sh:  Tests relating to SOurceForge bug 513800

# Import common functions & definitions.
. ../common/test-common

g=foo
s=s.$g
p=p.$g

remove $s $p $g

# Extract the test input files. 
for n in 1 2 
do
    filename=sf513800_${n}.uue
    ../../testutils/uu_decode --decode < $filename || miscarry could not uudecode $filename
done

docommand s1 "${vg_sact} $s" 0 IGNORE IGNORE 



remove $s $p $g
success
E 1
