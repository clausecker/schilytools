#! /bin/sh

# Basic tests for SCCS text/binary files

# Read test core functions
. ../common/test-common

g=foo
s=s.$g
p=p.$g
z=z.$g
output=get.output

remove $z $s $p $g

echo		'%M%' > $g	|| miscarry "Could not create $g"
echo_nonl	'\001D\n' >> $g	|| miscarry "Could not append a line to $g"
docommand bb1 "${admin} -n -i$g $s" IGNORE IGNORE IGNORE
if test -r $s
then
	echo "$s created with ^A at start of line"
	docommand bb2 "${prs} -d:FL: $s > $output" 0 "" ""
	if grep encoded $output > /dev/null
	then
		echo "PASS $s created encoded"
	else
		fail "$s created unencoded even though ^A at start of line"
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
