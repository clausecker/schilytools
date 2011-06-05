#! /bin/sh
# n-option.sh:  Testing for the -n option of "delta"

# Import common functions & definitions.
. ../common/test-common

g=foo
s=s.$g

remove $s $g p.$g z.$g


# Create an SCCS file.
docommand d1 "${admin} -n $s"    0 "" IGNORE

# Check the file out for editing.
docommand d2 "${get} -e $s"      0 IGNORE IGNORE

# Check the file back in with delta, using the -n option.
docommand d3 "${vg_delta} -n -y $s" 0 IGNORE IGNORE

# Make sure the g-file hasn't been deleted, and is still writable.
docommand d4 "test -w $g" 0 "" IGNORE



remove $s $g
success
