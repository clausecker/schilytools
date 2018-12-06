h21086
s 00067/00000/00000
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

docommand de_bulk1 "${admin} -N-XXXX -i. $g $g2" 0 "" ""

docommand de_bulk2a "${get} -NXXXX -e $g $g2" 0 IGNORE ""
echo a >> $g
echo b >> $g2
docommand de_bulk2 "${delta} -NXXXX -ycomment $g $g2" 0 IGNORE ""

docommand de_bulk3a "${get} -N -e XXXX/$g XXXX/$g2" 0 IGNORE ""
echo a2 >> XXXX/$g
echo b2 >> XXXX/$g2
docommand de_bulk3 "${delta} -N -ycomment2 XXXX/$g XXXX/$g2" 0 IGNORE ""

docommand de_bulk4a "${get} -Ns. -e XXXX/$s XXXX/$s2" 0 IGNORE ""
echo a3 >> XXXX/$g
echo b3 >> XXXX/$g2
docommand de_bulk4 "${delta} -Ns. -ycomment3 XXXX/$s XXXX/$s2" 0 IGNORE ""

docommand de_bulk5a "${get} -NXXXX/s. -e XXXX/$s XXXX/$s2" 0 IGNORE ""
echo a4 >> $g
echo b4 >> $g2
docommand de_bulk5 "${delta} -NXXXX/s. -ycomment4 XXXX/$s XXXX/$s2" 0 IGNORE ""

docommand de_bulk11 "${admin} -NXXXX -fy -n a/b/c/$g a/b/c/$g2" 0 "" ""

docommand de_bulk12a "${get} -NXXXX -e a/b/c/$g a/b/c/$g2" 0 IGNORE ""
echo a >> a/b/c/$g
echo b >> a/b/c/$g2
docommand de_bulk12 "${delta} -NXXXX -ycomment a/b/c/$g a/b/c/$g2" 0 IGNORE ""

docommand de_bulk13a "${get} -N -e a/b/c/XXXX/$g a/b/c/XXXX/$g2" 0 IGNORE ""
echo a2 >> a/b/c/XXXX/$g
echo b2 >> a/b/c/XXXX/$g2
docommand de_bulk13 "${delta} -N -ycomment2 a/b/c/XXXX/$g a/b/c/XXXX/$g2" 0 IGNORE ""

docommand de_bulk14a "${get} -Ns. -e a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""
echo a3 >> a/b/c/XXXX/$g
echo b3 >> a/b/c/XXXX/$g2
docommand de_bulk14 "${delta} -Ns. -ycomment3 a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""

docommand de_bulk15a "${get} -NXXXX/s. -e a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""
echo a4 >> a/b/c/$g
echo b4 >> a/b/c/$g2
docommand de_bulk15 "${delta} -NXXXX/s. -ycomment4 a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""

remove $z $s $p $g $z2 $s2 $p2 $g2 XXXX a
success
E 1