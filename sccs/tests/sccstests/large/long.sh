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

remove $z $s $p $g $output
success
