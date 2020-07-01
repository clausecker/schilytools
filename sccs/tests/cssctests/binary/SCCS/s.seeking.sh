hV6,sum=37656
s 00003/00003/00241
d D 1.6 2015/06/03 00:06:43+0200 joerg 6 5
S s 58793
c ../common/test-common -> ../../common/test-common
e
s 00002/00002/00242
d D 1.5 2015/06/01 23:55:23+0200 joerg 5 4
S s 58376
c ../../testutils/ -> ${SRCROOT}/tests/testutils/
e
s 00078/00010/00166
d D 1.4 2014/08/26 20:19:40+0200 joerg 4 3
S s 55938
c ascii/binary Unterschiede bei SCCSv4 <-> SCCSv6 beruecksichtigen
e
s 00001/00001/00175
d D 1.3 2011/06/18 16:06:36+0200 joerg 3 2
S s 35286
c x=z.$g -> x=x.$g
e
s 00001/00001/00175
d D 1.2 2011/05/30 01:17:27+0200 joerg 2 1
S s 35288
c if ( ${admin} -V 2>&1 ; echo umsp )  | grep CSSC >/dev/null -> if $TESTING_CSSC
e
s 00176/00000/00000
d D 1.1 2010/04/29 02:05:14+0200 joerg 1 0
S s 38614
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ebcd31f
G p sccs/tests/cssctests/binary/seeking.sh
t
T
I 1
#! /bin/sh
# auto.sh:  Tests for "admin"'s detection of binary files.

# Import common functions & definitions.
D 6
. ../common/test-common
. ../common/real-thing
. ../common/config-data
E 6
I 6
. ../../common/test-common
. ../../common/real-thing
. ../../common/config-data
E 6

if $binary_support
then
    true
else
    echo "Skipping these tests -- no binary file support."
    exit 0
fi 



good() {
    if $expect_fail ; then
	echo UNEXPECTED PASS
    else
	echo passed
    fi
}

bad() {
    if $expect_fail ; then
	echo failed -- but we expected that.
    else
	fail "$@"
    fi
}


# test_bin: 
# Usage:   test_bin LABEL <contents>
#
# create a flie containing the specified argument and check
# that it is encoded as a binary file.  
test_bin() {
label=$1
echo_nonl ${label}...
shift

infile=$1
shift


if ${use_stdin}
then
    cmd="cat ${infile} | ${vg_admin} -i ${adminflags} $s"
else
    cmd="${vg_admin} -i${infile} ${adminflags} $s"
fi

rm -f $s
remove errmsg
if ( eval "${cmd}" ) >/dev/null 2>errmsg
then
    if ( ${prs} -d:FL: $s 2>/dev/null; echo foo ) | grep encoded >/dev/null 2>&1
    then
	remove $g errmsg
	if ${get} -p ${s} 2>errmsg >${g}
	then
	    remove errmsg
	    if diff ${infile} ${g} >diffs
	    then
		remove diffs
		good
	    else
		# We failed to get the data back correctly from the 
		# SCCS file.
		bad "$label data lost (see file 'diffs') in ${cmd}".
	    fi
	else
	    bad $label "${get} -p $s failed: `cat errmsg`"
	fi
    else
	bad $label input did not produce an encoded s-file.
    fi
else
    cat errmsg ; remove errmsg
    bad $label ${admin} returned exit status $?.
fi
rm -f $s
}

I 4
# test_ascii: 
#
# As for test_bin, but the resulting SCCS file must NOT be encoded.
# 
test_ascii() {
label=$1
echo_nonl ${label}...
shift

rm -f infile $s
echo_nonl "$@" > infile
if ${vg_admin} -iinfile ${adminflags} $s >/dev/null 2>&1
then
    if ( ${prt} -f $s 2>/dev/null ; echo foo ) | grep encoded >/dev/null 2>&1
    then
        bad $label input produced an encoded s-file and should not have.
    else
        good
    fi
else
    bad $label ${vg_admin} returned exit status $?.
fi
rm -f infile $s
}


E 4
g=testfile
s=s.$g
z=z.$g
D 3
x=z.$g
E 3
I 3
x=x.$g
E 3
p=p.$g
files="$s $z $x $p"

remove $files long-text-file infile ctrl-A-file no-newline ctrl-A-end
remove command.log log  base [sxzp].$g errmsg $g

expect_fail=false
adminflags=""

remove long-text-file
D 5
../../testutils/yammer 1000 "this is a text file" > long-text-file
#../../testutils/yes "this is a text file" | nl | head -1000 >long-text-file
E 5
I 5
${SRCROOT}/tests/testutils/yammer 1000 "this is a text file" > long-text-file
#${SRCROOT}/tests/testutils/yes "this is a text file" | nl | head -1000 >long-text-file
E 5
if test -s long-text-file
then
    true 
else
    miscarry could not create long-text-file.
fi

remove no-newline 
echo_nonl "no newline here" > no-newline
remove ctrl-A-file
echo_nonl \
    "the next line of this file starts with " \
    "a ctrl-A,\n\001Which SCCS does not like" > ctrl-A-file
remove ctrl-A-end
echo_nonl \
    "This file ends with a ctrl-A.\n\001" > ctrl-A-end


# Make sure that we correctly decide to encode the files, 
# and that we don't lose data.

# First make sure that forcing binary mode works.
use_stdin=false
adminflags=-b
test_bin s1 long-text-file
adminflags=


# Now try various nearly-text files.
D 4
test_bin s2 ctrl-A-file
E 4
I 4
if $TESTING_SCCS_V6
then
	test_ascii s2 ctrl-A-file
else
	test_bin s2 ctrl-A-file
fi
E 4

# Create a file which we only discover needs encoding after
# we have read loads of it.
remove infile ; cat long-text-file ctrl-A-file > infile
D 4
test_bin s3 infile
E 4
I 4
if $TESTING_SCCS_V6
then
	test_ascii s3 infile
else
	test_bin s3 infile
fi
E 4

# Another long file but binary because it lacks a newline at the end.
D 4
test_bin s4 ctrl-A-end
E 4
I 4
if $TESTING_SCCS_V6
then
	test_ascii s4 ctrl-A-end
else
	test_bin s4 ctrl-A-end
fi
E 4

use_stdin=true


## Same tests as before, but with the "-i" file on a pipe.



D 2
if ( ${admin} -V 2>&1 ; echo umsp )  | grep CSSC >/dev/null
E 2
I 2
if $TESTING_CSSC
E 2
then
    # Do the tests that SCCS does not pass.
    use_stdin=false
    
D 4
    test_bin s5 no-newline		# Real SCCS fails this one.
E 4
I 4
	if $TESTING_SCCS_V6
	then
	    test_ascii s5 no-newline	# Real SCCS fails this one.
	else
	    test_bin s5 no-newline	# Real SCCS fails this one.
	fi
E 4
    
    remove infile ; cat long-text-file no-newline > infile
D 4
    test_bin s6 infile
E 4
I 4
	if $TESTING_SCCS_V6
	then
	    test_ascii s6 infile
	else
	    test_bin s6 infile
	fi
E 4

    use_stdin=true
D 4
    test_bin i1 ctrl-A-file
    test_bin i2 infile
E 4
I 4
	if $TESTING_SCCS_V6
	then
	    test_ascii i1 ctrl-A-file
	    test_ascii i2 infile
	else
	    test_bin i1 ctrl-A-file
	    test_bin i2 infile
	fi
E 4
    remove infile ; cat long-text-file ctrl-A-file > infile
D 4
    test_bin i3 ctrl-A-end
    test_bin i4 no-newline
E 4
I 4
	if $TESTING_SCCS_V6
	then
	    test_ascii i3 ctrl-A-end
	    test_ascii i4 no-newline
	else
	    test_bin i3 ctrl-A-end
	    test_bin i4 no-newline
	fi
E 4

    remove infile ; cat long-text-file no-newline > infile
D 4
    test_bin i5 infile
E 4
I 4
	if $TESTING_SCCS_V6
	then
	    test_ascii i5 infile
	else
	    test_bin i5 infile
	fi
E 4
else
    echo "Not running tests on CSSC; Some tests have been been omitted"
fi

remove $files long-text-file infile ctrl-A-file no-newline ctrl-A-end
remove command.log log  base errmsg $g
success
E 1
