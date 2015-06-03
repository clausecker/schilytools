h53212
s 00001/00001/00028
d D 1.2 15/06/03 00:06:45 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00029/00000/00000
d D 1.1 11/05/29 20:20:45 joerg 1 0
c date and time created 11/05/29 20:20:45 by joerg
e
u
U
f e 0
t
T
I 1
#! /bin/sh

# Basic tests for SCCS long lines

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
output=get.output

remove $z $s $p $g

cp long $g
expect_fail=true
docommand l1 "${admin} -n -i$g $s" 0 "" IGNORE
remove $g
docommand l2 "${get} $s" 0 IGNORE IGNORE
if cmp long $g
then
	echo "PASS Long lines are supported"
else
	fail "Long lines are not supported"
fi

remove $z $s $p $g $output
success
E 1
