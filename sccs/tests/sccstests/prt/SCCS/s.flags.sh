hV6,sum=19058
s 00002/00002/00066
d D 1.2 2015/06/03 00:06:45+0200 joerg 2 1
S s 58229
c ../common/test-common -> ../../common/test-common
e
s 00068/00000/00000
d D 1.1 2011/05/29 20:19:37+0200 joerg 1 0
S s 57951
c date and time created 11/05/29 20:19:37 by joerg
e
u
U
f e 0
G r 0e46e8b659333
G p sccs/tests/sccstests/prt/flags.sh
t
T
I 1
#! /bin/sh

# Basic tests for extended SCCS flags woth prt

# Read test core functions
D 2
. ../common/test-common
. ../common/need-prt
E 2
I 2
. ../../common/test-common
. ../../common/need-prt
E 2

g=foo
s=s.$g
p=p.$g
z=z.$g
output=got.output
error=got.error

remove $z $s $p $g

if test .$HAVE_PRT = .false
then
	fail "No prt command available"
else
	expect_fail=true
	docommand fl1 "${prt} -f s.flags > $output" 0 "" ""
	if grep encoded $output > /dev/null
	then
		echo "SUCCESS prt prints \"encoded\" flag"
		if grep floor $output > /dev/null
		then
			echo "SUCCESS prt prints \"floor\" flag"
			if grep 'encoded.*floor' $output > /dev/null
			then
				fail "prt prints no newline past \"encoded\" flag"
			else
				echo "SUCCESS prt correctly prints \"encoded\" flag including new line"
			fi
		fi
	else
		fail "prt does not print \"encoded\" flag"
		if grep floor $output > /dev/null
			echo "SUCCESS prt prints \"floor\" flag"
		then
			fail "prt does not print \"floor\" flag"
		fi
	fi

	docommand fl2 "${prt} -f s.flagz > $output" 0 "" ""
	if grep "keywd scan lines" $output > /dev/null
	then
			echo "SUCCESS prt prints \"keywd scan lines\" flag"
	else
			fail "prt does not print \"keywd scan lines\" flag"
	fi
	if grep extensions $output > /dev/null
	then
			echo "SUCCESS prt prints \"extensions\" flag"
	else
			fail "prt does not print \"extensions\" flag"
	fi
	if grep "expand keywds" $output > /dev/null
	then
			echo "SUCCESS prt prints \"expand keywds\" flag"
	else
			fail "prt does not print \"expand keywds\" flag"
	fi
fi

remove $z $s $p $g $output $error
success
E 1
