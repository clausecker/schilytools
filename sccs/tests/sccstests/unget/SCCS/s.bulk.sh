h57597
s 00051/00000/00000
d D 1.1 18/12/04 21:32:06 joerg 1 0
c date and time created 18/12/04 21:32:06 by joerg
e
u
U
f e 0
f y 
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

docommand un_bulk1 "${admin} -N-XXXX -i. $g $g2" 0 "" ""

docommand un_bulk2a "${get} -NXXXX -e $g $g2" 0 IGNORE ""
docommand un_bulk2 "${unget} -NXXXX $g $g2" 0 IGNORE ""

docommand un_bulk3a "${get} -N -e XXXX/$g XXXX/$g2" 0 IGNORE ""
docommand un_bulk3 "${unget} -N  XXXX/$g XXXX/$g2" 0 IGNORE ""

docommand un_bulk4a "${get} -Ns. -e XXXX/$s XXXX/$s2" 0 IGNORE ""
docommand un_bulk4 "${unget} -Ns. XXXX/$s XXXX/$s2" 0 IGNORE ""

docommand un_bulk5a "${get} -NXXXX/s. -e XXXX/$s XXXX/$s2" 0 IGNORE ""
docommand un_bulk5 "${unget} -NXXXX/s. XXXX/$s XXXX/$s2" 0 IGNORE ""

docommand un_bulk11 "${admin} -NXXXX -fy -n a/b/c/$g a/b/c/$g2" 0 "" ""

docommand un_bulk12a "${get} -NXXXX -e a/b/c/$g a/b/c/$g2" 0 IGNORE ""
docommand un_bulk12 "${unget} -NXXXX a/b/c/$g a/b/c/$g2" 0 IGNORE ""

docommand un_bulk13a "${get} -N -e a/b/c/XXXX/$g a/b/c/XXXX/$g2" 0 IGNORE ""
docommand un_bulk13 "${unget} -N a/b/c/XXXX/$g a/b/c/XXXX/$g2" 0 IGNORE ""

docommand un_bulk14a "${get} -Ns. -e a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""
docommand un_bulk14 "${unget} -Ns. a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""

docommand un_bulk15a "${get} -NXXXX/s. -e a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""
docommand un_bulk15 "${unget} -NXXXX/s. a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""

remove $z $s $p $g $z2 $s2 $p2 $g2 XXXX a
success
E 1
