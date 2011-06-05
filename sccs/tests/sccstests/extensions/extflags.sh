#! /bin/sh

# Basic tests for SCCS flags in the range 'a'..'z'

# Read test core functions
. ../common/test-common

g=foo
s=s.$g
p=p.$g
z=z.$g

remove $z $s $p $g

#
# Checking whether SCCS supports all flags in the range 'a'..'z'
#
cp s.flags $s
expect_fail=true
docommand fl1 "${val} $s" 0 "" ""
remove $s
cp s.flagz $s
docommand fl2 "${val} $s" 0 "" ""
remove $s

cp s.flags $s
docommand fl3d "${admin} -db -dj -dn $s" 0 "" ""
if diff s.flags $s > /dev/null
then
	fail "Flags in the range 'a'..'y' are not all supported"
else
	docommand fl3f "${admin} -fb -fj -fn $s" 0 "" ""

	# Flag 'b' may appear as '^Af b' or '^Af b ', so the checksum may vary
	tail +2 s.flags	> s.o
	tail +2 $s	> s.n
	if diff -w s.o s.n > /dev/null
	then
		echo "Flags in the range 'a'..'y' are supported"
		remove s.o s.n
	else
		fail "--> admin -db -dj -dn $s / admin -fb -fj -fn $s changed other flags"
	fi

	cp s.flagz $s
	docommand fl4d "${admin} -db -dj -dn $s" 0 "" ""
	docommand fl4f "${admin} -fb -fj -fn $s" 0 "" ""

	# Flag 'b' may appear as '^Af b' or '^Af b ', so the checksum may vary
	tail +2 s.flagz	> s.o
	tail +2 $s	> s.n
	if diff -w s.o s.n > /dev/null
	then
		echo "Flags in the range 'a'..'u','w'..'z' are supported"
		remove s.o s.n
	else
		fail "--> admin -db -dj -dn $s / admin -fb -fj -fn $s changed other flags"
	fi
fi

remove s.o s.n
remove $z $s $p $g
success
