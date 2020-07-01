hV6,sum=56783
s 00001/00001/00057
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 31446
c ../common/test-common -> ../../common/test-common
e
s 00058/00000/00000
d D 1.1 2010/05/11 11:30:00+0200 joerg 1 0
S s 31307
c date and time created 10/05/11 11:30:00 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ecee480
G p sccs/tests/cssctests/get/optorder.sh
t
T
I 1
#! /bin/sh
# optorder.sh:  Testing for option ordering.
#   "get s.foo -Gbar" and "get -Gbar s.foo" should be equivalent.

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

remove command.log log log.stdout log.stderr passwd blah
mkdir test 2>/dev/null


g=base
s=s.$g
gotten=testG-$g

remove ${gotten} $g $s


# Create the input files.
echo foo > $g


#
# Create an SCCS file to work on.
# We generally ignore stderr output since we produce "Warning: no id keywords"
# more often than "real" SCCS.
#
docommand O1 "${admin} -i${g} ${s}" 0 "" IGNORE
remove $g

docommand O2 "${vg_get} -G${gotten} ${s}" 0 IGNORE IGNORE

# Make sure the gotten file was given the right name
echo_nonl O3...
if test -f ${gotten}
then
    echo passed
else
    fail "O3 gotten file ${gotten} was not created"
fi
remove $gotten

# Same again but with the other order
docommand O4 "${vg_get} ${s} -G${gotten}" 0 IGNORE IGNORE

# Make sure the gotten file was given the right name
echo_nonl O5...
if test -f ${gotten}
then
    echo passed
else
    fail "O5 gotten file ${gotten} was not created"
fi
remove $gotten


remove ${gotten} $g $s command.log
success
E 1
