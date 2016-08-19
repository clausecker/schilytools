#! /bin/sh

# Basic tests for degenerated SCCS comment

# Read test core functions
. ../../common/test-common

g=foo
s=s.$g
p=p.$g
z=z.$g

remove $z $s $p $g

#
# Checking whether SCCS supports to hide extensions in degentrated comments
# that do not look like '^Ac comment'
#
cp s.comment $s
expect_fail=true
docommand cm1 "${val} $s" 0 "" ""
remove $s

echo test | tail +2 > /dev/null 2>/dev/null
if [ "$?" -eq 0 ]; then
	plustwo=+2
else
	plustwo='-n +2'
fi

cp s.comment $s
docommand cm2d "${admin} -db $s" 0 "" ""
if diff s.comment $s > /dev/null
then
	fail "SCCS hidden extensions in degenerated comment are not supported"	
else
	docommand cm2f "${admin} -fb $s" 0 "" ""

	# Flag 'b' may appear as '^Af b' or '^Af b ', so the checksum may vary
	tail $plustwo s.comment	> s.o
	tail $plustwo $s	> s.n
	if diff -w s.o s.n > /dev/null
	then
		echo "SCCS hidden extensions in degenerated comment are supported"
		remove s.o s.n
	else
		fail "SCCS hidden extensions in degenerated comment are not supported"
	fi
fi

remove s.o s.n
remove $z $s $p $g
success
