hV6,sum=18579
s 00024/00000/00067
d D 1.2 2018/12/17 21:58:48+0100 joerg 2 1
S s 63016
c Neuer test fuer delta -N+XXXX
e
s 00067/00000/00000
d D 1.1 2018/12/04 21:32:06+0100 joerg 1 0
S s 12782
c date and time created 18/12/04 21:32:06 by joerg
e
u
U
f e 0
f y 
G r 0e46e8b5ac5db
G p sccs/tests/sccstests/delta/bulk.sh
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

I 2
remove a/b/c/$g a/b/c/$g2

#
# Test whether delta -N+XXXX does a correct implicit get(1)
#
docommand de_bulk20 "${get} -NXXXX  -e a/b/c/$g a/b/c/$g2" 0 IGNORE ""
echo '%M%' >> a/b/c/$g
echo '%M%' >> a/b/c/$g2
docommand de_bulk21 "${admin} -NXXXX -dy a/b/c/$g a/b/c/$g2" 0 IGNORE ""
docommand de_bulk22 "${delta} -N+XXXX -ycomment5 a/b/c/$g a/b/c/$g2" 0 IGNORE ""
if wtest -w a/b/c/$g || wtest -w a/b/c/$g2; then
	fail "a/b/c/$g a/b/c/$g2 writable"
fi
if grep "foo" a/b/c/$g > /dev/null 2>/dev/null; then
	:
else
	fail "a/b/c/$g keywords not expanded"
fi
if grep "foo2" a/b/c/$g2 > /dev/null 2>/dev/null; then
	:
else
	fail "a/b/c/$g2 keywords not expanded"
fi

E 2
remove $z $s $p $g $z2 $s2 $p2 $g2 XXXX a
success
E 1
