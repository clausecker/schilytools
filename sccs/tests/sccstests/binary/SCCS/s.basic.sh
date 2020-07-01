hV6,sum=13066
s 00002/00002/00053
d D 1.3 2015/06/03 00:06:45+0200 joerg 3 2
S s 29441
c ../common/test-common -> ../../common/test-common
e
s 00018/00002/00037
d D 1.2 2014/08/26 14:05:24+0200 joerg 2 1
S s 29163
c ^A Behandlung am Anfang der Zeile haengt nun davon ab ob ein SCCSv6 history File vorliegt
e
s 00039/00000/00000
d D 1.1 2011/05/29 20:20:24+0200 joerg 1 0
S s 63912
c date and time created 11/05/29 20:20:24 by joerg
e
u
U
f e 0
f y 
G r 0e46e8b57ae31
G p sccs/tests/sccstests/binary/basic.sh
t
T
I 1
#! /bin/sh

# Basic tests for SCCS text/binary files

# Read test core functions
D 3
. ../common/test-common
I 2
. ../common/real-thing
E 3
I 3
. ../../common/test-common
. ../../common/real-thing
E 3
E 2

g=foo
s=s.$g
p=p.$g
z=z.$g
output=get.output

remove $z $s $p $g

I 2
#
# The "classical" SCCSv4 history file format does not support ^A (\001) at the
# beginning of a line but SCCSv6 has no problem with supporting this as a text
# file.
#
E 2
echo		'%M%' > $g	|| miscarry "Could not create $g"
echo_nonl	'\001D\n' >> $g	|| miscarry "Could not append a line to $g"
docommand bb1 "${admin} -n -i$g $s" IGNORE IGNORE IGNORE
if test -r $s
then
	echo "$s created with ^A at start of line"
	docommand bb2 "${prs} -d:FL: $s > $output" 0 "" ""
	if grep encoded $output > /dev/null
	then
D 2
		echo "PASS $s created encoded"
E 2
I 2
		if $TESTING_SCCS_V6
		then
			fail "$s created uu-encoded with ^A at start of line even though SCCS v6"
		else
			echo "PASS $s created encoded"
		fi
E 2
	else
D 2
		fail "$s created unencoded even though ^A at start of line"
E 2
I 2
		if $TESTING_SCCS_V6
		then
			echo "PASS $s created un-encoded"
		else
			fail "$s created un-encoded even though ^A at start of line"
		fi
E 2
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
E 1
