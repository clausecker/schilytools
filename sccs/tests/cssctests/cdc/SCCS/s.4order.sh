hV6,sum=08035
s 00001/00001/00053
d D 1.3 2015/06/03 00:06:44+0200 joerg 3 2
S s 42158
c ../common/test-common -> ../../common/test-common
e
s 00001/00001/00053
d D 1.2 2011/06/18 16:06:36+0200 joerg 2 1
S s 42019
c x=z.$g -> x=x.$g
e
s 00054/00000/00000
d D 1.1 2010/05/11 11:30:00+0200 joerg 1 0
S s 42021
c date and time created 10/05/11 11:30:00 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ec1b11e
G p sccs/tests/cssctests/cdc/4order.sh
t
T
I 1
#! /bin/sh
# 4order.sh:  Testing for ordering of the components of the comment.

# Import common functions & definitions.
D 3
. ../common/test-common
E 3
I 3
. ../../common/test-common
E 3

g=testfile
s=s.$g
z=z.$g
D 2
x=z.$g
E 2
I 2
x=x.$g
E 2
p=p.$g
files="$s $z $x $p"

remove command.log log log.stdout log.stderr base [sxzp].$g


# Create the input file.
cat > $g <<EOF
%M%: This is a test file containing nothing interesting.
EOF

# Create an SCCS file to work on.
docommand O1 "${admin} -i$g $s" 0 "" ""
remove $g


# Set the MR flag
docommand O2 "${admin} -fvtrue $s" 0 "" ""


# Add another MR.   Check the order is correct.
docommand O3 "${vg_cdc} -r1.1 -yahoo '-mMR2 MR3 MR1' $s" 0 "" "" 
docommand O4 "${prs} -r1.1 -d:MR: $s" 0 "MR2\nMR3\nMR1\n\n" "" 

# Make sure that when we add another comment AND delete 
# an MT, the two parts of the comment end up in the 
# correct order.
docommand O5 "${vg_cdc} -r1.1 -yMyExtraComment '-m!MR3' $s" 0 "" ""

remove comment
${prs} -d:C: -r1.1 $s > comment || fail prs failed unexpectedly

docommand O6 "sed -n 1p <comment" 0 "MyExtraComment\n" ""
docommand O7 "sed -n 2p <comment" 0 "*** LIST OF DELETED MRS ***\n" ""
docommand O8 "sed -n 3p <comment" 0 "MR3\n" ""
docommand O9 "sed -n 4p <comment|egrep \
'^\*\*\* CHANGED \*\*\* [0-9][0-9]/[01][0-9]/[0-3][0-9] [012][0-9]:[0-6][0-9]:[0-6][0-9] [^ ][^ ]*$'" 0 "IGNORE" ""


remove comment


remove command.log passwd $s $p $g $z $x
success
E 1
