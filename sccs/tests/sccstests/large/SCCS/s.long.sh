h38922
s 00024/00000/00029
d D 1.3 18/12/02 10:05:24 joerg 3 2
c Neuer Test mit noch laengeren Zeilen und nun auch mit delta
e
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
I 3
remove $s $g
E 3

I 3
cp long2 $g
expect_fail=true
docommand l3 "${admin} -n -i$g $s" 0 "" IGNORE
remove $g
docommand l4 "${get} -p $s > $output" 0 IGNORE IGNORE
if cmp long2 $output
then
	:
else
	xfail "Very long lines are not supported"
fi
docommand l5 "${get} -e $s" 0 IGNORE IGNORE
cat long2 >> $g
cp $g $output
docommand l6 "${delta} -yComment $s" 0 IGNORE IGNORE
docommand l7 "${get} $s" 0 IGNORE IGNORE
if cmp $g $output
then
	echo "PASS Very long lines are supported"
else
	xfail "Very long lines are not supported"
fi

E 3
remove $z $s $p $g $output
success
E 1
