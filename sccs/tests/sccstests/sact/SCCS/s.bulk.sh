hV6,sum=28904
s 00039/00000/00000
d D 1.1 2018/12/04 21:32:06+0100 joerg 1 0
S s 15093
c date and time created 18/12/04 21:32:06 by joerg
e
u
U
f e 0
f y 
G r 0e46e8b693962
G p sccs/tests/sccstests/sact/bulk.sh
t
T
I 1
#! /bin/sh

# Basic tests for SCCS flag -N

# Read test core functions
. ../../common/test-common

g=foo
s=s.$g
p=p.$g
z=z.$g
g2=foo2
s2=s.$g2
p2=p.$g2
z2=z.$g2

remove $z $s $p $g $z2 $s2 $p2 $g2 XXXX a

echo '%M%' > $g
echo '%F%' > $g2

docommand sa_bulk1 "${admin} -N-XXXX -i. $g $g2" 0 "" ""
docommand sa_bulk1b "${get} -NXXXX -e $g $g2" 0 IGNORE ""

docommand sa_bulk2 "${sact} -NXXXX $g $g2" 0 IGNORE ""
docommand sa_bulk3 "${sact} -N XXXX/$g XXXX/$g2" 0 IGNORE ""
docommand sa_bulk4 "${sact} -Ns. XXXX/$s XXXX/$s2" 0 IGNORE ""
docommand sa_bulk5 "${sact} -NXXXX/s. XXXX/$s XXXX/$s2" 0 IGNORE ""

docommand sa_bulk11 "${admin} -NXXXX -fy -n a/b/c/$g a/b/c/$g2" 0 "" ""
docommand sa_bulk11b "${get} -NXXXX -e a/b/c/$g a/b/c/$g2" 0 IGNORE ""

docommand sa_bulk12 "${sact} -NXXXX a/b/c/$g a/b/c/$g2" 0 IGNORE ""
docommand sa_bulk13 "${sact} -N a/b/c/XXXX/$g a/b/c/XXXX/$g2" 0 IGNORE ""
docommand sa_bulk14 "${sact} -Ns. a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""
docommand sa_bulk15 "${sact} -NXXXX/s. a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""

remove $z $s $p $g $z2 $s2 $p2 $g2 XXXX a
success
E 1
