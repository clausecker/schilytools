h39577
s 00058/00000/00000
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
# optorder.sh:  Testing for option ordering.
#   "get s.foo -Gbar" and "get -Gbar s.foo" should be equivalent.

# Import common functions & definitions.
. ../common/test-common

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
