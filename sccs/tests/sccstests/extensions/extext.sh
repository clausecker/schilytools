#! /bin/sh

# Basic tests for SCCS extensions bast flag section

# Read test core functions
. ../../common/test-common

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

cp s.ext $s
docommand ext2d "${admin} -db $s" 0 "" ""
if diff s.ext $s > /dev/null
then
	fail "SCCS Extensions past flag section not supported"
else
	docommand ext2f "${admin} -fb $s" 0 "" ""

	# Flag 'b' may appear as '^Af b' or '^Af b ', so the checksum may vary
	tail +2 s.ext	> s.o
	tail +2 $s	> s.n
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
