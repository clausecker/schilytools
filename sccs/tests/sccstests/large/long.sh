#! /bin/sh

# Basic tests for SCCS long lines

# Read test core functions
. ../../common/test-common

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
remove $s $g

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

remove $z $s $p $g $output
success
