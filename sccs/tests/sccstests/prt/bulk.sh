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

docommand pt_bulk1 "${admin} -N-XXXX -i. $g $g2" 0 "" ""

docommand pt_bulk2 "${prt} -NXXXX $g $g2" 0 IGNORE ""
docommand pt_bulk3 "${prt} -N XXXX/$g XXXX/$g2" 0 IGNORE ""
docommand pt_bulk4 "${prt} -Ns. XXXX/$s XXXX/$s2" 0 IGNORE ""
docommand pt_bulk5 "${prt} -NXXXX/s. XXXX/$s XXXX/$s2" 0 IGNORE ""

docommand pt_bulk11 "${admin} -NXXXX -fy -n a/b/c/$g a/b/c/$g2" 0 "" ""

docommand pt_bulk12 "${prt} -NXXXX a/b/c/$g a/b/c/$g2" 0 IGNORE ""
docommand pt_bulk13 "${prt} -N a/b/c/XXXX/$g a/b/c/XXXX/$g2" 0 IGNORE ""
docommand pt_bulk14 "${prt} -Ns. a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""
docommand pt_bulk15 "${prt} -NXXXX/s. a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""

remove $z $s $p $g $z2 $s2 $p2 $g2 XXXX a
success
