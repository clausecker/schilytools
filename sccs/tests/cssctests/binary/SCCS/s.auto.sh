h16051
s 00003/00003/00130
d D 1.5 15/06/03 00:06:43 joerg 5 4
c ../common/test-common -> ../../common/test-common
e
s 00012/00002/00121
d D 1.4 14/08/26 20:19:40 joerg 4 3
c ascii/binary Unterschiede bei SCCSv4 <-> SCCSv6 beruecksichtigen
e
s 00001/00001/00122
d D 1.3 11/06/18 16:06:36 joerg 3 2
c x=z.$g -> x=x.$g
e
s 00002/00001/00121
d D 1.2 11/05/30 01:18:01 joerg 2 1
c Falscher Binaertest fuer CSSC korrigiert
e
s 00122/00000/00000
d D 1.1 10/05/11 11:30:00 joerg 1 0
c date and time created 10/05/11 11:30:00 by joerg
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
D 5
. ../common/test-common
. ../common/real-thing
. ../common/config-data
E 5
I 5
. ../../common/test-common
. ../../common/real-thing
. ../../common/config-data
E 5



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

rm -f infile $s
echo_nonl "$@" > infile
if ${vg_admin} -iinfile ${adminflags} $s >/dev/null 2>&1
then
    if ( ${prs} -d:FL: $s 2>/dev/null; echo foo ) | grep encoded >/dev/null 2>&1
    then
        good
    else
        bad $label input did not produce an encoded s-file.
    fi
else
    bad $label ${vg_admin} returned exit status $?.
fi
rm -f infile $s
}

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

remove command.log log  base [sxzp].$g

expect_fail=false
adminflags=""

if $binary_support
then

test_ascii a1 "foo\n" 
test_ascii a2 "foo\nbar\n" 
test_ascii a3 ""                        # an empty input file should be OK.
D 4
test_bin   b4 "\001\n"                  # line starts with control-a
E 4
I 4
if $TESTING_SCCS_V6
then
  test_ascii b4 "\001\n"                # line starts with control-a
else
  test_bin   b4 "\001\n"                # line starts with control-a
fi
E 4
test_ascii a5 "x\001\n"                 # line contains ^A but doesnt start with it.

adminflags=-b
test_bin   b6 "foo\n"                   # Test manual encoding override.
test_bin   b7 "x\000y\n"                # ASCII NUL.
test_bin   b8 "\000"                    # Just the ASCII NUL alone.
test_bin   b9 "\001"                    # Just the ^A alone.
adminflags=

if $TESTING_CSSC
then
    ## Real SCCS fails on these inputs:-
D 4
    test_bin   fb10 "foo"               # no newline at end of file.
E 4
I 4
    if $TESTING_SCCS_V6
    then
      test_ascii fb10 "foo"             # no newline at end of file.
    else
      test_bin   fb10 "foo"             # no newline at end of file.
    fi
E 4
D 2
    test_ascii fa11 "x\000y\n"          # ASCII NUL.
E 2
I 2
    test_bin   fa11 "x\000y\n"          # ASCII NUL.
    test_ascii fa12 "x\001y\n"          # ASCII SOH.
E 2
else
    echo "Some tests skipped (since SCCS fails them but CSSC should pass)"
fi

remove command.log $s $p $g $z $x infile

else
echo "No binary file support -- tests skipped"
fi # binary support. 

success 
E 1
