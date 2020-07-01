hV6,sum=41551
s 00040/00000/00000
d D 1.1 2018/12/04 21:32:06+0100 joerg 1 0
S s 27657
c date and time created 18/12/04 21:32:06 by joerg
e
u
U
f e 0
f y 
G r 0e46e8b594af6
G p sccs/tests/sccstests/comb/bulk.sh
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

docommand co_bulk1 "${admin} -NXXXX -i. $g $g2" 0 "" ""
docommand co_bulk1b "${delta} -NXXXX -q -f -Xprepend -ycomment $g $g2" 0 IGNORE ""

docommand co_bulk2 "${comb} -NXXXX $g $g2" 0 IGNORE IGNORE
docommand co_bulk3 "${comb} -N XXXX/$g XXXX/$g2" 0 IGNORE IGNORE
docommand co_bulk4 "${comb} -Ns. XXXX/$s XXXX/$s2" 0 IGNORE IGNORE
docommand co_bulk5 "${comb} -NXXXX/s. XXXX/$s XXXX/$s2" 0 IGNORE IGNORE

docommand co_bulk11 "${admin} -NXXXX -fy -n a/b/c/$g a/b/c/$g2" 0 "" ""
docommand co_bulk11a "${get} -NXXXX a/b/c/$g a/b/c/$g2" 0 IGNORE ""
docommand co_bulkq1b "${delta} -NXXXX -q -f -Xprepend -ycomment a/b/c/$g a/b/c/$g2" 0 IGNORE ""

docommand co_bulk12 "${comb} -NXXXX a/b/c/$g a/b/c/$g2" 0 IGNORE IGNORE
docommand co_bulk13 "${comb} -N a/b/c/XXXX/$g a/b/c/XXXX/$g2" 0 IGNORE IGNORE
docommand co_bulk14 "${comb} -Ns. a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE IGNORE
docommand co_bulk15 "${comb} -NXXXX/s. a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE IGNORE

remove $z $s $p $g $z2 $s2 $p2 $g2 XXXX a
success
E 1
