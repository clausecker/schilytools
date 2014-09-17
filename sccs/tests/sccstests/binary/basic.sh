#! /bin/sh

# Basic tests for SCCS text/binary files

# Read test core functions
. ../common/test-common
. ../common/real-thing

g=foo
s=s.$g
p=p.$g
z=z.$g
output=get.output

remove $z $s $p $g

#
# The "classical" SCCSv4 history file format does not support ^A (\001) at the
# beginning of a line but SCCSv6 has no problem with supporting this as a text
# file.
#
echo		'%M%' > $g	|| miscarry "Could not create $g"
echo_nonl	'\001D\n' >> $g	|| miscarry "Could not append a line to $g"
docommand bb1 "${admin} -n -i$g $s" IGNORE IGNORE IGNORE
if test -r $s
then
	echo "$s created with ^A at start of line"
	docommand bb2 "${prs} -d:FL: $s > $output" 0 "" ""
	if grep encoded $output > /dev/null
	then
		if $TESTING_SCCS_V6
		then
			fail "$s created uu-encoded with ^A at start of line even though SCCS v6"
		else
			echo "PASS $s created encoded"
		fi
	else
		if $TESTING_SCCS_V6
		then
			echo "PASS $s created un-encoded"
		else
			fail "$s created un-encoded even though ^A at start of line"
		fi
	fi
else
	if test $cmd_exit -eq 0
	then
		fail "$s not created but exit code is $cmd_exit"
	else
		echo "$s not created with ^A at start of line"
	fi
fi

remove $z $s $p $g $output
success
