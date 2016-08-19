h44721
s 00009/00002/00044
d D 1.3 16/08/14 23:16:32 joerg 3 2
c tail +2 -> tail $plustwo und $plustwo wird automatish angepasst
e
s 00001/00001/00045
d D 1.2 15/06/03 00:06:45 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00046/00000/00000
d D 1.1 11/05/29 21:08:43 joerg 1 0
c date and time created 11/05/29 21:08:43 by joerg
e
u
U
f e 0
t
T
I 1
#! /bin/sh

# Basic tests for SCCS extensions bast flag section

# Read test core functions
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

g=foo
s=s.$g
p=p.$g
z=z.$g

remove $z $s $p $g

#
# Checking whether SCCS supports extensions between flag section
# and comment section
#
cp s.ext $s
expect_fail=true
docommand ext1 "${val} $s" 0 "" ""
remove $s

I 3
echo test | tail +2 > /dev/null 2>/dev/null
if [ "$?" -eq 0 ]; then
	plustwo=+2
else
	plustwo='-n +2'
fi

E 3
cp s.ext $s
docommand ext2d "${admin} -db $s" 0 "" ""
if diff s.ext $s > /dev/null
then
	fail "SCCS Extensions past flag section not supported"
else
	docommand ext2f "${admin} -fb $s" 0 "" ""

	# Flag 'b' may appear as '^Af b' or '^Af b ', so the checksum may vary
D 3
	tail +2 s.ext	> s.o
	tail +2 $s	> s.n
E 3
I 3
	tail $plustwo s.ext	> s.o
	tail $plustwo $s	> s.n
E 3
	if diff -w s.o s.n > /dev/null
	then
		echo "SCCS Extensions past flag section are supported"
		remove s.o s.n
	else
		fail "SCCS Extensions past flag section not supported"
	fi
fi

remove s.o s.n
remove $z $s $p $g
success
E 1
