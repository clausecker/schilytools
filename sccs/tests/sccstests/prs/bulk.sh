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

docommand ps_bulk1 "${admin} -N-XXXX -i. $g $g2" 0 "" ""

docommand ps_bulk2 "${prs} -NXXXX $g $g2" 0 IGNORE ""
docommand ps_bulk3 "${prs} -N XXXX/$g XXXX/$g2" 0 IGNORE ""
docommand ps_bulk4 "${prs} -Ns. XXXX/$s XXXX/$s2" 0 IGNORE ""
docommand ps_bulk5 "${prs} -NXXXX/s. XXXX/$s XXXX/$s2" 0 IGNORE ""

docommand ps_bulk11 "${admin} -NXXXX -fy -n a/b/c/$g a/b/c/$g2" 0 "" ""

docommand ps_bulk12 "${prs} -NXXXX a/b/c/$g a/b/c/$g2" 0 IGNORE ""
docommand ps_bulk13 "${prs} -N a/b/c/XXXX/$g a/b/c/XXXX/$g2" 0 IGNORE ""
docommand ps_bulk14 "${prs} -Ns. a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""
docommand ps_bulk15 "${prs} -NXXXX/s. a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""

remove $z $s $p $g $z2 $s2 $p2 $g2 XXXX a
success
