#! /bin/sh

# Basic tests for extended SCCS flags woth prt

# Read test core functions
. ../common/test-common
. ../common/need-prt

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
