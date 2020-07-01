hV6,sum=15070
s 00098/00000/00000
d D 1.1 2018/12/04 21:32:06+0100 joerg 1 0
S s 00826
c date and time created 18/12/04 21:32:06 by joerg
e
u
U
f e 0
f y 
G r 0e46e8b6c7a99
G p sccs/tests/sccstests/sccscvt/bulk.sh
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

#
# SCCS_V6 is unset with the normal SCCS v4 tests and set to empty value
# in case we run the same tests in a way that let's all new history files
# to be created as SCCS v6 files.
#
sccs_v6=${SCCS_V6-false} 
: ${sccs_v6:=true}			# true/false depending on state


remove $z $s $p $g $z2 $s2 $p2 $g2 XXXX a

echo '%M%' > $g
echo '%F%' > $g2

docommand cv_bulk1 "${admin} -N-XXXX -i. $g $g2" 0 "" ""
if $sccs_v6; then
	docommand cv_bulk1a "${sccscvt} -NXXXX -V4 $g $g2" 0 "" ""
fi

docommand cv_bulk2a "${sccscvt} -NXXXX -V6 $g $g2" 0 IGNORE ""
${val} -v XXXX/s.foo | grep "SCCS V6" > /dev/null 2>/dev/null || fail "Not in V6 format"
${val} -v XXXX/s.foo2 | grep "SCCS V6" > /dev/null 2>/dev/null || fail "Not in V6 format"
docommand cv_bulk2b "${sccscvt} -NXXXX -V4 $g $g2" 0 IGNORE ""
${val} -v XXXX/s.foo | grep "SCCS V4" > /dev/null 2>/dev/null || fail "Not in V4 format"
${val} -v XXXX/s.foo2 | grep "SCCS V4" > /dev/null 2>/dev/null || fail "Not in V4 format"

docommand cv_bulk3a "${sccscvt} -N -V6 XXXX/$g XXXX/$g2" 0 IGNORE ""
${val} -v XXXX/s.foo | grep "SCCS V6" > /dev/null 2>/dev/null || fail "Not in V6 format"
${val} -v XXXX/s.foo2 | grep "SCCS V6" > /dev/null 2>/dev/null || fail "Not in V6 format"
docommand cv_bulk3b "${sccscvt} -N -V4 XXXX/$g XXXX/$g2" 0 IGNORE ""
${val} -v XXXX/s.foo | grep "SCCS V4" > /dev/null 2>/dev/null || fail "Not in V4 format"
${val} -v XXXX/s.foo2 | grep "SCCS V4" > /dev/null 2>/dev/null || fail "Not in V4 format"

docommand cv_bulk4a "${sccscvt} -Ns. -V6 XXXX/$s XXXX/$s2" 0 IGNORE ""
${val} -v XXXX/s.foo | grep "SCCS V6" > /dev/null 2>/dev/null || fail "Not in V6 format"
${val} -v XXXX/s.foo2 | grep "SCCS V6" > /dev/null 2>/dev/null || fail "Not in V6 format"
docommand cv_bulk4b "${sccscvt} -Ns. -V4 XXXX/$s XXXX/$s2" 0 IGNORE ""
${val} -v XXXX/s.foo | grep "SCCS V4" > /dev/null 2>/dev/null || fail "Not in V4 format"
${val} -v XXXX/s.foo2 | grep "SCCS V4" > /dev/null 2>/dev/null || fail "Not in V4 format"

docommand cv_bulk5a "${sccscvt} -NXXXX/s. -V6 XXXX/$s XXXX/$s2" 0 IGNORE ""
${val} -v XXXX/s.foo | grep "SCCS V6" > /dev/null 2>/dev/null || fail "Not in V6 format"
${val} -v XXXX/s.foo2 | grep "SCCS V6" > /dev/null 2>/dev/null || fail "Not in V6 format"
docommand cv_bulk5b "${sccscvt} -NXXXX/s. -V4 XXXX/$s XXXX/$s2" 0 IGNORE ""
${val} -v XXXX/s.foo | grep "SCCS V4" > /dev/null 2>/dev/null || fail "Not in V4 format"
${val} -v XXXX/s.foo2 | grep "SCCS V4" > /dev/null 2>/dev/null || fail "Not in V4 format"

docommand cv_bulk11 "${admin} -NXXXX -fy -n a/b/c/$g a/b/c/$g2" 0 "" ""
if $sccs_v6; then
	docommand cv_bulk11a "${sccscvt} -NXXXX -V4 a/b/c/$g a/b/c/$g2" 0 "" ""
fi

docommand cv_bulk12a "${sccscvt} -NXXXX -V6 a/b/c/$g a/b/c/$g2" 0 IGNORE ""
${val} -v a/b/c/XXXX/s.foo | grep "SCCS V6" > /dev/null 2>/dev/null || fail "Not in V6 format"
${val} -v a/b/c/XXXX/s.foo2 | grep "SCCS V6" > /dev/null 2>/dev/null || fail "Not in V6 format"
docommand cv_bulk12b "${sccscvt} -NXXXX -V4 a/b/c/$g a/b/c/$g2" 0 IGNORE ""
${val} -v a/b/c/XXXX/s.foo | grep "SCCS V4" > /dev/null 2>/dev/null || fail "Not in V4 format"
${val} -v a/b/c/XXXX/s.foo2 | grep "SCCS V4" > /dev/null 2>/dev/null || fail "Not in V4 format"

docommand cv_bulk13a "${sccscvt} -N -V6 a/b/c/XXXX/$g a/b/c/XXXX/$g2" 0 IGNORE ""
${val} -v a/b/c/XXXX/s.foo | grep "SCCS V6" > /dev/null 2>/dev/null || fail "Not in V6 format"
${val} -v a/b/c/XXXX/s.foo2 | grep "SCCS V6" > /dev/null 2>/dev/null || fail "Not in V6 format"
docommand cv_bulk13b "${sccscvt} -N -V4 a/b/c/XXXX/$g a/b/c/XXXX/$g2" 0 IGNORE ""
${val} -v a/b/c/XXXX/s.foo | grep "SCCS V4" > /dev/null 2>/dev/null || fail "Not in V4 format"
${val} -v a/b/c/XXXX/s.foo2 | grep "SCCS V4" > /dev/null 2>/dev/null || fail "Not in V4 format"

docommand cv_bulk14a "${sccscvt} -Ns. -V6 a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""
${val} -v a/b/c/XXXX/s.foo | grep "SCCS V6" > /dev/null 2>/dev/null || fail "Not in V6 format"
${val} -v a/b/c/XXXX/s.foo2 | grep "SCCS V6" > /dev/null 2>/dev/null || fail "Not in V6 format"
docommand cv_bulk14b "${sccscvt} -Ns. -V4 a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""
${val} -v a/b/c/XXXX/s.foo | grep "SCCS V4" > /dev/null 2>/dev/null || fail "Not in V4 format"
${val} -v a/b/c/XXXX/s.foo2 | grep "SCCS V4" > /dev/null 2>/dev/null || fail "Not in V4 format"

docommand cv_bulk15a "${sccscvt} -NXXXX/s. -V6 a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""
${val} -v a/b/c/XXXX/s.foo | grep "SCCS V6" > /dev/null 2>/dev/null || fail "Not in V6 format"
${val} -v a/b/c/XXXX/s.foo2 | grep "SCCS V6" > /dev/null 2>/dev/null || fail "Not in V6 format"
docommand cv_bulk15b "${sccscvt} -NXXXX/s. -V4 a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""
${val} -v a/b/c/XXXX/s.foo | grep "SCCS V4" > /dev/null 2>/dev/null || fail "Not in V4 format"
${val} -v a/b/c/XXXX/s.foo2 | grep "SCCS V4" > /dev/null 2>/dev/null || fail "Not in V4 format"

remove $z $s $p $g $z2 $s2 $p2 $g2 XXXX a
success
E 1
