#! /bin/sh
# general.sh:  Testing for general aspects of cdc.

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
