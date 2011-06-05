#! /bin/sh

# Tests with only one revision in the SCCS file.

# Import common functions & definitions.
. ../common/test-common


f=1test
remove $f s.$f
: > $f
docommand O1 "$admin -n -i$f s.$f" 0 "" IGNORE
test -r s.$f         || fail admin did not create s.$f
remove $f out

# The get should succeed.
docommand O2 "${vg_get} s.$f" 0 "1.1\n0 lines\n" IGNORE

# With no SCCS file, get should fail.
remove s.$f 
docommand O3 "${vg_get} s.$f" 1 "" IGNORE
remove s.$f $f 

remove command.log
success
