hV6,sum=51030
s 00094/00000/00000
d D 1.1 2018/12/04 21:32:06+0100 joerg 1 0
S s 37229
c date and time created 18/12/04 21:32:06 by joerg
e
u
U
f e 0
f y 
G r 0e46e8b5f664c
G p sccs/tests/sccstests/get/bulk.sh
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

docommand ge_bulk1 "${admin} -N-XXXX -i. $g $g2" 0 "" ""

#
# Create foo in current dir.
#
docommand ge_bulk2 "${get} -NXXXX $g $g2" 0 IGNORE ""
if [ ! -r $g ] || [ ! -r $g2 ] ; then
	fail "SCCS get did not create $g or $g2"
fi
remove foo* XXXX/foo*
#
# Create XXXX/foo.
#
docommand ge_bulk3 "${get} -N XXXX/$g XXXX/$g2" 0 IGNORE ""
if [ ! -r XXXX/$g ] || [ ! -r XXXX/$g2 ] ; then
	fail "SCCS get did not create XXXX/$g or XXXX/$g2"
fi
remove foo* XXXX/foo*
#
# Create XXXX/foo.
#
docommand ge_bulk4 "${get} -Ns. XXXX/$s XXXX/$s2" 0 IGNORE ""
if [ ! -r XXXX/$g ] || [ ! -r XXXX/$g2 ] ; then
	fail "SCCS get did not create XXXX/$g or XXXX/$g2"
fi
remove foo* XXXX/foo*
#
# Create foo in current dir.
#
docommand ge_bulk5 "${get} -NXXXX/s. XXXX/$s XXXX/$s2" 0 IGNORE ""
if [ ! -r $g ] || [ ! -r $g2 ] ; then
	fail "SCCS get did not create $g or $g2"
fi

docommand ge_bulk11 "${admin} -NXXXX -fy -n a/b/c/$g a/b/c/$g2" 0 "" ""
if [ ! -r a/b/c/XXXX/$s ] || [ ! -r a/b/c/XXXX/$s2 ] ; then
	fail "SCCS admin did not create a/b/c/XXXX/$s or a/b/c/XXXX/$s2"
fi

#
# Create foo in a/b/c.
#
docommand ge_bulk12 "${get} -NXXXX a/b/c/$g a/b/c/$g2" 0 IGNORE ""
if [ ! -r a/b/c/$g ] || [ ! -r a/b/c/$g2 ] ; then
	fail "SCCS get did not create a/b/c/$g or a/b/c/$g2"
fi
remove a/b/c/$g a/b/c/$g2
#
# Create a/b/c/XXXX/foo.
#
docommand ge_bulk13 "${get} -N a/b/c/XXXX/$g a/b/c/XXXX/$g2" 0 IGNORE ""
if [ ! -r a/b/c/XXXX/$g ] || [ ! -r a/b/c/XXXX/$g2 ] ; then
	fail "SCCS get did not create a/b/c/XXXX/$g or a/b/c/XXXX/$g2"
fi
remove a/b/c/XXXX/$g a/b/c/XXXX/$g2
#
# Create XXXX/foo.
#
docommand ge_bulk14 "${get} -Ns. a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""
if [ ! -r a/b/c/XXXX/$g ] || [ ! -r a/b/c/XXXX/$g2 ] ; then
	fail "SCCS get did not create a/b/c/XXXX/$g or a/b/c/XXXX/$g2"
fi
remove a/b/c/XXXX/$g a/b/c/XXXX/$g2
#
# Create foo in a/b/c.
#
docommand ge_bulk15 "${get} -NXXXX/s. a/b/c/XXXX/$s a/b/c/XXXX/$s2" 0 IGNORE ""
if [ ! -r a/b/c/$g ] || [ ! -r a/b/c/$g2 ] ; then
	fail "SCCS get did not create a/b/c/$g or a/b/c/$g2"
fi

remove $z $s $p $g $z2 $s2 $p2 $g2 XXXX a
success
E 1
