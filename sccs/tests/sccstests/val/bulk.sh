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

docommand va_bulk1 "${admin} -N-XXXX -i. $g $g2" 0 "" ""

docommand va_bulk2 "${val} -NXXXX $g $g2" 0 IGNORE ""
docommand va_bulk3 "${val} -N XXXX/$g XXXX/$g2" 0 IGNORE ""
docommand va_bulk4 "${val} -Ns. XXXX/$s XXXX/$s2" 0 IGNORE ""
docommand va_bulk5 "${val} -NXXXX/s. XXXX/$s XXXX/$s2" 0 IGNORE ""

docommand va_bulk11 "${admin} -NXXXX -fy -n a/b/c/$g a/b/c/$g2" 0 "" ""

docommand va_bulk12 "${val} -NXXXX a/b/c/$g a/b/c/$g2" 0 IGNORE ""
docommand va_bulk13 "${val} -N a/b/c/XXXX/$g a/b/c/XXXX/$g2" 0 IGNORE ""
docommand va_bulk14 "${val} -Ns. a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""
docommand va_bulk15 "${val} -NXXXX/s. a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""

remove $z $s $p $g $z2 $s2 $p2 $g2 XXXX a
success
