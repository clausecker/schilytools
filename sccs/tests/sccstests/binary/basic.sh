#! /bin/sh

# Basic tests for SCCS text/binary files

# Read test core functions
. ../../common/test-common
. ../../common/real-thing

g=foo
g2=foo-out
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
	docommand bb1a "${get} -p -k -s $s > $g2" 0 "" ""
	if cmp -s $g $g2 > /dev/null
	then
		:
	else
		fail "gotten files differs"
	fi
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

#
# The "classical" SCCSv4 history file format does not support files with no \n
# at the end of the last line but SCCSv6 has no problem with supporting this as a text
# file.
#
echo		'%M%' > $g	|| miscarry "Could not create $g"
echo_nonl	'xxxD' >> $g	|| miscarry "Could not append a line to $g"
docommand bb3 "${admin} -n -i$g $s" IGNORE IGNORE IGNORE
if test -r $s
then
	echo "$s created with no '\\\\n' at the end of last line"
	docommand bb3a "${get} -p -k -s $s > $g2" 0 "" ""
	if cmp -s $g $g2 > /dev/null
	then
		:
	else
		fail "gotten files differs"
	fi
	docommand bb4 "${prs} -d:FL: $s > $output" 0 "" ""
	if grep encoded $output > /dev/null
	then
		if $TESTING_SCCS_V6
		then
			fail "$s created uu-encoded with no '\\\\n' at the end of last line even though SCCS v6"
		else
			echo "PASS $s created encoded"
		fi
	else
		if $TESTING_SCCS_V6
		then
			echo "PASS $s created un-encoded"
		else
			fail "$s created un-encoded even though no '\\\\n' at the end of last line"
		fi
	fi
else
	if test $cmd_exit -eq 0
	then
		fail "$s not created but exit code is $cmd_exit"
	else
		echo "$s not created with no '\\\\n' at the end of last line"
	fi
fi
remove $z $s $p $g $output

#
# We need to check delta(1) in un-encoded binary mode with all three
# diff options.
#
for dtype in "" "-b" "-d"; do
#
# The "classical" SCCSv4 history file format does not support NUL (\000) in the
# file, but SCCSv6 has no problem with supporting this as a "text file".
#
echo		'%M%' > $g	|| miscarry "Could not create $g"
echo_nonl	'\000D\n' >> $g	|| miscarry "Could not append a line to $g"
docommand bb5$dtype "${admin} -n -i$g $s" IGNORE IGNORE IGNORE
if test -r $s
then
	echo "$s created with '\\\\000' in the file"
	docommand bb5a$dtype "${get} -p -k -s $s > $g2" 0 "" ""
	if cmp -s $g $g2 > /dev/null
	then
		:
	else
		fail "gotten files differs"
	fi
	docommand bb6$dtype "${prs} -d:FL: $s > $output" 0 "" ""
	if grep encoded $output > /dev/null
	then
		if $TESTING_SCCS_V6
		then
			fail "$s created uu-encoded with '\\\\000' in the the file even though SCCS v6"
		else
			echo "PASS $s created encoded"
		fi
	else
		if $TESTING_SCCS_V6
		then
			echo "PASS $s created un-encoded"
		else
			fail "$s created un-encoded even though '\\\\000' in the file"
		fi
	fi
else
	if test $cmd_exit -eq 0
	then
		fail "$s not created but exit code is $cmd_exit"
	else
		echo "$s not created with '\\\\000' in the file"
	fi
fi
remove $g $output

docommand bb7$dtype "${get} -e -s $s" 0 "" ""
if cmp -s $g $g2 > /dev/null
then
	:
else
	fail "gotten files differs"
fi
remove $g2 $output

cat /bin/sh* >> $g
echo_nonl	'\000end' >> $g	|| miscarry "Could not append a line to $g"
cp $g $g2
docommand bb8$dtype "${delta} $dtype -ya $s" 0 IGNORE IGNORE
docommand bb9$dtype "${get} -k -s $s" 0 "" ""
if cmp -s $g $g2 > /dev/null
then
	:
else
	fail "gotten files differs"
fi
docommand bb10$dtype "${admin} -fy $s" 0 "" ""
remove $g
docommand bb11$dtype "${get} -s $s" 0 "" ""
if cmp -s $g $g2 > /dev/null
then
	:
else
	fail "gotten files differs"
fi
remove $z $s $p $g $g2 $output
done

#
# tar.tar.7z is a file where the last line starts with ^A and
# does not end in a newline.
#
docommand bb12 "${admin} -itar.tar.7z $s" 0 "" IGNORE
docommand bb13 "${get} -p -k -s $s > $g" 0 "" ""
if cmp -s tar.tar.7z $g > /dev/null
then
	:
else
	fail "gotten files differs"
fi
docommand bb14 "${admin} -fy $s" 0 "" ""
docommand bb14b "${prs} -d:FL: $s > $output" 0 "" ""
if grep encoded $output > /dev/null
then
	if $TESTING_SCCS_V6
	then
		fail "$s created uu-encoded with '\\\\000' in the the file even though SCCS v6"
	else
		echo "PASS $s created encoded"
	fi
else
	if $TESTING_SCCS_V6
	then
		echo "PASS $s created un-encoded"
	else
		fail "$s created un-encoded even though '\\\\000' in the file"
	fi
fi
remove $g
docommand bb15 "${get} -s $s" 0 "" ""
if cmp -s tar.tar.7z $g > /dev/null
then
	:
else
	fail "gotten files differs"
fi

remove $z $s $p $g $g2 $output
success
