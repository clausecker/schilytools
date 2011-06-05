h23753
s 00072/00000/00000
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
# 2comment.sh:  Testing for comments

# Import common functions & definitions.
. ../common/test-common

g=testfile
s=s.$g
z=z.$g
x=z.$g
p=p.$g
files="$s $z $x $p"

remove command.log log log.stdout log.stderr base [sxzp].$g


# Create the input file.
cat > $g <<EOF
%M%: This is a test file containing nothing interesting.
EOF

# Create an SCCS file to work on.
docommand C1 "${admin} -i$g $s" 0 "" ""
remove $g

# Now change the (initial) comment.
docommand C2 "${vg_cdc} -r1.1 '-yNewComment
NewComment2' $s" 0 "" "" 

# Extract only the comment.
remove comment 
${prs} -d:C: -r1.1 $s > comment


# Test the first line
docommand C3 "sed -n 1p <comment" 0 "NewComment\n" ""

# Test the second line
docommand C4 "sed -n 2p <comment" 0 "NewComment2\n" ""

# Test the third line
# I can't think of a better match for a username than [^ ][^ ]*
docommand C5 "sed -n 3p <comment|egrep \
'^\*\*\* CHANGED \*\*\* [0-9][0-9]/[01][0-9]/[0-3][0-9] [012][0-9]:[0-6][0-9]:[0-6][0-9] [^ ][^ ]*$'" 0 "IGNORE" ""


# Now change the (already changed) comment.
docommand C6 "${vg_cdc} -r1.1 '-yAnother Comment' $s" 0 "" "" 


# Again, extract only the comment.
remove comment 
${prs} -d:C: -r1.1 $s > comment || fail prs failed unexpectedly


# Test the first line
docommand C7 "sed -n 1p <comment" 0 "Another Comment\n" ""

# Test the second (new "CHANGED") line
docommand C8 "sed -n 2p <comment|egrep \
'^\*\*\* CHANGED \*\*\* [0-9][0-9]/[01][0-9]/[0-3][0-9] [012][0-9]:[0-6][0-9]:[0-6][0-9] [^ ][^ ]*$'" 0 "IGNORE" ""

# Test the third line (used to be 1st line of previous comment)
docommand C9 "sed -n 3p <comment" 0 "NewComment\n" ""

# Test the fifth (old "CHANGED") line
docommand C10 "sed -n 5p <comment|egrep \
'^\*\*\* CHANGED \*\*\* [0-9][0-9]/[01][0-9]/[0-3][0-9] [012][0-9]:[0-6][0-9]:[0-6][0-9] [^ ][^ ]*$'" 0 "IGNORE" ""


remove command.log passwd $s $p $g $z $x
success
E 1
