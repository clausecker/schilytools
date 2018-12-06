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

docommand rm_bulk1 "${admin} -N-XXXX -i. $g $g2" 0 "" ""

docommand rm_bulk2a "${get} -NXXXX -e $g $g2" 0 IGNORE ""
echo a >> $g
echo b >> $g2
docommand rm_bulk2b "${delta} -NXXXX -ycomment $g $g2" 0 IGNORE ""
docommand rm_bulk2 "${rmdel} -NXXXX -d -r1.2 $g $g2" 0 IGNORE ""

docommand rm_bulk3a "${get} -NXXXX -e $g $g2" 0 IGNORE ""
echo a >> $g
echo b >> $g2
docommand rm_bulk3b "${delta} -NXXXX -ycomment $g $g2" 0 IGNORE ""
docommand rm_bulk3 "${rmdel} -N -d -r1.2 -ycomment2 XXXX/$g XXXX/$g2" 0 IGNORE ""

docommand rm_bulk4a "${get} -NXXXX -e $g $g2" 0 IGNORE ""
echo a >> $g
echo b >> $g2
docommand rm_bulk4b "${delta} -NXXXX -ycomment $g $g2" 0 IGNORE ""
docommand rm_bulk4 "${rmdel} -Ns. -d -r1.2 -ycomment3 XXXX/$s XXXX/$s2" 0 IGNORE ""

docommand rm_bulk4a "${get} -NXXXX -e $g $g2" 0 IGNORE ""
echo a >> $g
echo b >> $g2
docommand rm_bulk4b "${delta} -NXXXX -ycomment $g $g2" 0 IGNORE ""
docommand rm_bulk5 "${rmdel} -NXXXX/s. -d -r1.2 -ycomment4 XXXX/$s XXXX/$s2" 0 IGNORE ""

docommand rm_bulk11 "${admin} -NXXXX -fy -n a/b/c/$g a/b/c/$g2" 0 "" ""

docommand rm_bulk12a "${get} -NXXXX -e a/b/c/$g a/b/c/$g2" 0 IGNORE ""
echo a >> a/b/c/$g
echo b >> a/b/c/$g2
docommand rm_bulk12b "${delta} -NXXXX -ycomment a/b/c/$g a/b/c/$g2" 0 IGNORE ""
docommand rm_bulk12 "${rmdel} -NXXXX -d -r1.2 a/b/c/$g a/b/c/$g2" 0 IGNORE ""

docommand rm_bulk13a "${get} -NXXXX -e a/b/c/$g a/b/c/$g2" 0 IGNORE ""
echo a >> a/b/c/$g
echo b >> a/b/c/$g2
docommand rm_bulk13b "${delta} -NXXXX -ycomment a/b/c/$g a/b/c/$g2" 0 IGNORE ""
docommand rm_bulk13 "${rmdel} -N -d -r1.2 a/b/c/XXXX/$g a/b/c/XXXX/$g2" 0 IGNORE ""

docommand rm_bulk14a "${get} -NXXXX -e a/b/c/$g a/b/c/$g2" 0 IGNORE ""
echo a >> a/b/c/$g
echo b >> a/b/c/$g2
docommand rm_bulk14b "${delta} -NXXXX -ycomment a/b/c/$g a/b/c/$g2" 0 IGNORE ""
docommand rm_bulk14 "${rmdel} -Ns. -d -r1.2 a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""

docommand rm_bulk15a "${get} -NXXXX -e a/b/c/$g a/b/c/$g2" 0 IGNORE ""
echo a >> a/b/c/$g
echo b >> a/b/c/$g2
docommand rm_bulk15b "${delta} -NXXXX -ycomment a/b/c/$g a/b/c/$g2" 0 IGNORE ""
docommand rm_bulk15 "${rmdel} -NXXXX/s. -d -r1.2 a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""

remove $z $s $p $g $z2 $s2 $p2 $g2 XXXX a
success
