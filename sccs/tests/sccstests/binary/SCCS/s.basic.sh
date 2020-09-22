hV6,sum=19735
s 00016/00010/00213
d D 1.6 2020/09/15 23:53:45.018976319+0200 joerg 6 5
S s 18679
c Nun werden alle 3 diff Varianten von delta(1) getestet
e
s 00049/00000/00174
d D 1.5 2020/09/15 23:17:50.516650326+0200 joerg 5 4
S s 03465
c Tests ob admin -fy auch bei Binaerfiles wirkt
c Neues Binaerfile mit letzter Zeile ohne NL und ^A am Anfang
e
s 00119/00000/00055
d D 1.4 2020/09/13 23:45:16.604080566+0200 joerg 4 3
S s 54395
c Neue Tests fuer Nul Bytes und mit cmp der "gotten" Dateien gegen das Original
e
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
I 4
g2=foo-out
E 4
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
I 4
	docommand bb1a "${get} -p -k -s $s > $g2" 0 "" ""
	if cmp -s $g $g2 > /dev/null
	then
		:
	else
		fail "gotten files differs"
	fi
E 4
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
I 4
remove $z $s $p $g $output
E 4

I 4
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
E 4
remove $z $s $p $g $output
I 4

#
D 6
# The "classical" SCCSv4 history file format does not support NUL (\000) in the
# file, but SCCSv6 has no problem with supporting this as a "text file".
E 6
I 6
# We need to check delta(1) in un-encoded binary mode with all three
# diff options.
E 6
#
I 6
for dtype in "" "-b" "-d"; do
#
# The "classical" SCCSv4 history file format does not support NUL (\000) in the
# file, but SCCSv6 has no problem with supporting this as a "text file".
#
E 6
echo		'%M%' > $g	|| miscarry "Could not create $g"
echo_nonl	'\000D\n' >> $g	|| miscarry "Could not append a line to $g"
D 6
docommand bb5 "${admin} -n -i$g $s" IGNORE IGNORE IGNORE
E 6
I 6
docommand bb5$dtype "${admin} -n -i$g $s" IGNORE IGNORE IGNORE
E 6
if test -r $s
then
	echo "$s created with '\\\\000' in the file"
D 6
	docommand bb5a "${get} -p -k -s $s > $g2" 0 "" ""
E 6
I 6
	docommand bb5a$dtype "${get} -p -k -s $s > $g2" 0 "" ""
E 6
	if cmp -s $g $g2 > /dev/null
	then
		:
	else
		fail "gotten files differs"
	fi
D 6
	docommand bb6 "${prs} -d:FL: $s > $output" 0 "" ""
E 6
I 6
	docommand bb6$dtype "${prs} -d:FL: $s > $output" 0 "" ""
E 6
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

D 6
docommand bb7 "${get} -e -s $s" 0 "" ""
E 6
I 6
docommand bb7$dtype "${get} -e -s $s" 0 "" ""
E 6
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
D 6
docommand bb8 "${delta} -ya $s" 0 IGNORE IGNORE
docommand bb9 "${get} -k -s $s" 0 "" ""
E 6
I 6
docommand bb8$dtype "${delta} $dtype -ya $s" 0 IGNORE IGNORE
docommand bb9$dtype "${get} -k -s $s" 0 "" ""
E 6
if cmp -s $g $g2 > /dev/null
then
	:
else
	fail "gotten files differs"
fi
I 5
D 6
docommand bb10 "${admin} -fy $s" 0 "" ""
E 6
I 6
docommand bb10$dtype "${admin} -fy $s" 0 "" ""
E 6
remove $g
D 6
docommand bb11 "${get} -s $s" 0 "" ""
E 6
I 6
docommand bb11$dtype "${get} -s $s" 0 "" ""
E 6
if cmp -s $g $g2 > /dev/null
then
	:
else
	fail "gotten files differs"
fi
remove $z $s $p $g $g2 $output
I 6
done
E 6
E 5

I 5
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

E 5
remove $z $s $p $g $g2 $output
E 4
success
E 1
