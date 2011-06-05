h57946
s 00001/00001/00175
d D 1.2 11/05/30 01:17:27 joerg 2 1
c if ( ${admin} -V 2>&1 ; echo umsp )  | grep CSSC >/dev/null -> if $TESTING_CSSC
e
s 00176/00000/00000
d D 1.1 10/04/29 02:05:14 joerg 1 0
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh
# auto.sh:  Tests for "admin"'s detection of binary files.

# Import common functions & definitions.
. ../common/test-common
. ../common/real-thing
. ../common/config-data

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

g=testfile
s=s.$g
z=z.$g
x=z.$g
p=p.$g
files="$s $z $x $p"

remove $files long-text-file infile ctrl-A-file no-newline ctrl-A-end
remove command.log log  base [sxzp].$g errmsg $g

expect_fail=false
adminflags=""

remove long-text-file
../../testutils/yammer 1000 "this is a text file" > long-text-file
#../../testutils/yes "this is a text file" | nl | head -1000 >long-text-file
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
test_bin s2 ctrl-A-file

# Create a file which we only discover needs encoding after
# we have read loads of it.
remove infile ; cat long-text-file ctrl-A-file > infile
test_bin s3 infile

# Another long file but binary because it lacks a newline at the end.
test_bin s4 ctrl-A-end

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
    
    test_bin s5 no-newline		# Real SCCS fails this one.
    
    remove infile ; cat long-text-file no-newline > infile
    test_bin s6 infile

    use_stdin=true
    test_bin i1 ctrl-A-file
    test_bin i2 infile
    remove infile ; cat long-text-file ctrl-A-file > infile
    test_bin i3 ctrl-A-end
    test_bin i4 no-newline

    remove infile ; cat long-text-file no-newline > infile
    test_bin i5 infile
else
    echo "Not running tests on CSSC; Some tests have been been omitted"
fi

remove $files long-text-file infile ctrl-A-file no-newline ctrl-A-end
remove command.log log  base errmsg $g
success
E 1
