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

docommand lo_bulk1 "${admin} -N-XXXX -i. $g $g2" 0 "" ""

docommand lo_bulk2 "${sccslog} -NXXXX $g $g2" 0 IGNORE ""
docommand lo_bulk3 "${sccslog} -N XXXX/$g XXXX/$g2" 0 IGNORE ""
docommand lo_bulk4 "${sccslog} -Ns. XXXX/$s XXXX/$s2" 0 IGNORE ""
docommand lo_bulk5 "${sccslog} -NXXXX/s. XXXX/$s XXXX/$s2" 0 IGNORE ""

docommand lo_bulk11 "${admin} -NXXXX -fy -n a/b/c/$g a/b/c/$g2" 0 "" ""

docommand lo_bulk12 "${sccslog} -NXXXX a/b/c/$g a/b/c/$g2" 0 IGNORE ""
docommand lo_bulk13 "${sccslog} -N a/b/c/XXXX/$g a/b/c/XXXX/$g2" 0 IGNORE ""
docommand lo_bulk14 "${sccslog} -Ns. a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""
docommand lo_bulk15 "${sccslog} -NXXXX/s. a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""

remove $z $s $p $g $z2 $s2 $p2 $g2 XXXX a
success
