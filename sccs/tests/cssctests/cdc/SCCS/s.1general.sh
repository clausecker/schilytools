h27965
s 00001/00001/00042
d D 1.3 15/06/03 00:06:43 joerg 3 2
c ../common/test-common -> ../../common/test-common
e
s 00001/00001/00042
d D 1.2 11/06/18 16:06:36 joerg 2 1
c x=z.$g -> x=x.$g
e
s 00043/00000/00000
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
# general.sh:  Testing for general aspects of cdc.

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
docommand G1 "${admin} -i$g $s" 0 "" ""
remove $g

# Test general behaviour works.
docommand G2 "${vg_cdc} -r1.1 '-yNewComment
NewComment2' $s" 0 "" "" 

# Make sure a SID is required
docommand G3 "${vg_cdc} -yNewComment2 $s" 1 "" IGNORE


# Make sure an SCCS file is required
docommand G4 "${vg_cdc} -yNewComment2" 1 "" IGNORE

# Make sure a complete lack or args is diagnosed.
docommand G5 "${vg_cdc}" 1 "" IGNORE 



remove command.log passwd $s $p $g $z $x
success
E 1
