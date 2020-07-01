hV6,sum=15339
s 00083/00000/00000
d D 1.1 2018/12/04 21:32:06+0100 joerg 1 0
S s 01397
c date and time created 18/12/04 21:32:06 by joerg
e
u
U
f e 0
f y 
G r 0e46e8b560a83
G p sccs/tests/sccstests/admin/bulk.sh
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
echo '%M%' > $g2

docommand ad_bulk1 "${admin} -NXXXX -n $g $g2" 0 "" ""
remove XXXX
docommand ad_bulk2 "${admin} -NXXXX -i. $g $g2" 0 "" ""
docommand ad_bulk3 "${admin} -NXXXX -fb $g $g2" 0 "" ""
docommand ad_bulk4 "${prs}  -d:FL: XXXX/$s XXXX/$s2" 0 "branch\n\nbranch\n\n" ""
docommand ad_bulk5 "${admin} -NXXXX -db $g $g2" 0 "" ""
docommand ad_bulk6 "${prs}  -d:FL: XXXX/$s XXXX/$s2" 0 "\n\n" ""
remove XXXX

docommand ad_bulk11 "${admin} -N -n $g $g2" 0 "" ""
remove $s $s2
docommand ad_bulk12 "${admin} -N -i. $g $g2" 0 "" ""
docommand ad_bulk13 "${admin} -N -fb $g $g2" 0 "" ""
docommand ad_bulk14 "${prs}  -d:FL: $s $s2" 0 "branch\n\nbranch\n\n" ""
docommand ad_bulk15 "${admin} -N -db $g $g2" 0 "" ""
docommand ad_bulk16 "${prs}  -d:FL: $s $s2" 0 "\n\n" ""
remove $s $s2

docommand ad_bulk21 "${admin} -Ns. -n $s $s2" 0 "" ""
remove $s $s2
docommand ad_bulk22 "${admin} -Ns. -i. $s $s2" 0 "" ""
docommand ad_bulk23 "${admin} -Ns. -fb $s $s2" 0 "" ""
docommand ad_bulk24 "${prs}  -d:FL: $s $s2" 0 "branch\n\nbranch\n\n" ""
docommand ad_bulk25 "${admin} -Ns. -db $s $s2" 0 "" ""
docommand ad_bulk26 "${prs}  -d:FL: $s $s2" 0 "\n\n" ""
remove $s $s2

docommand ad_bulk31 "${admin} -NXXXX/s. -n XXXX/$s XXXX/$s2" 0 "" ""
remove XXXX
docommand ad_bulk32 "${admin} -NXXXX/s. -i. XXXX/$s XXXX/$s2" 0 "" ""
docommand ad_bulk33 "${admin} -NXXXX/s. -fb XXXX/$s XXXX/$s2" 0 "" ""
docommand ad_bulk34 "${prs}  -d:FL: XXXX/$s XXXX/$s2" 0 "branch\n\nbranch\n\n" ""
docommand ad_bulk35 "${admin} -NXXXX/s. -db XXXX/$s XXXX/$s2" 0 "" ""
docommand ad_bulk36 "${prs}  -d:FL: XXXX/$s XXXX/$s2" 0 "\n\n" ""

docommand ad_bulk51 "${admin} -NXXXX -fy -n a/b/c/$g a/b/c/$g2" 0 "" ""
if [ ! -r a/b/c/XXXX/$s ] || [ ! -r a/b/c/XXXX/$s2 ] ; then
	fail "SCCS admin did not create a/b/c/XXXX/$s or a/b/c/XXXX/$s2"
fi
remove a

docommand ad_bulk52 "${admin} -N -fy -n a/b/c/$g a/b/c/$g2" 0 "" ""
if [ ! -r a/b/c/$s ] || [ ! -r a/b/c/$s2 ] ; then
	fail "SCCS admin did not create a/b/c/$s or a/b/c/$s2"
fi
remove a

docommand ad_bulk53 "${admin} -Ns. -fy -n a/b/c/$s a/b/c/$s2" 0 "" ""
if [ ! -r a/b/c/$s ] || [ ! -r a/b/c/$s2 ] ; then
	fail "SCCS admin did not create a/b/c/$s or a/b/c/$s2"
fi
remove a

docommand ad_bulk54 "${admin} -NXXXX/s. -fy -n a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 "" ""
if [ ! -r a/b/c/XXXX/$s ] || [ ! -r a/b/c/XXXX/$s2 ] ; then
	fail "SCCS admin did not create a/b/c/XXXX/$s or a/b/c/XXXX/$s2"
fi
remove a


remove $z $s $p $g $z2 $s2 $p2 $g2 XXXX a
success
E 1
